#include "mpris_poller.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>

MprisPoller::MprisPoller(std::filesystem::path helper)
    : helper_(std::move(helper)) {}

MprisPoller::~MprisPoller() {
    stop();
}

bool MprisPoller::start(bool skipArt, std::uint64_t nowMs) {
    if (running()) return false;
    int pipeFds[2];
    if (pipe2(pipeFds, O_CLOEXEC | O_NONBLOCK) != 0) return false;
    const pid_t pid = fork();
    if (pid == 0) {
        dup2(pipeFds[1], STDOUT_FILENO);
        const int nullFd = open("/dev/null", O_WRONLY);
        if (nullFd >= 0) dup2(nullFd, STDERR_FILENO);
        close(pipeFds[0]);
        close(pipeFds[1]);
        if (skipArt) {
            execl(helper_.c_str(), helper_.c_str(), "--no-art",
                  static_cast<char*>(nullptr));
        } else {
            execl(helper_.c_str(), helper_.c_str(),
                  static_cast<char*>(nullptr));
        }
        _exit(127);
    }
    close(pipeFds[1]);
    if (pid < 0) {
        close(pipeFds[0]);
        return false;
    }
    childPid_ = pid;
    outputFd_ = pipeFds[0];
    output_.clear();
    startedAtMs_ = nowMs;
    return true;
}

std::optional<MprisPollResult> MprisPoller::update() {
    if (!running()) return std::nullopt;
    std::array<char, 2048> buffer{};
    const auto readOutput = [&] {
        ssize_t bytes = 0;
        while ((bytes = read(outputFd_, buffer.data(), buffer.size())) > 0) {
            output_.append(buffer.data(), static_cast<std::size_t>(bytes));
        }
        return bytes == 0 || errno == EAGAIN || errno == EWOULDBLOCK;
    };
    if (!readOutput()) {
        MprisPollResult result;
        result.error = "could not read MPRIS helper output";
        result.startedAtMs = startedAtMs_;
        stop();
        return result;
    }

    int status = 0;
    if (waitpid(childPid_, &status, WNOHANG) != childPid_) {
        return std::nullopt;
    }
    // The child can exit between the first nonblocking read and waitpid. Once
    // it is reaped, all remaining pipe bytes are available and must be drained
    // before the descriptor closes.
    if (!readOutput()) {
        MprisPollResult result;
        result.error = "could not finish reading MPRIS helper output";
        result.startedAtMs = startedAtMs_;
        close(outputFd_);
        outputFd_ = -1;
        childPid_ = -1;
        output_.clear();
        return result;
    }
    close(outputFd_);
    outputFd_ = -1;
    childPid_ = -1;
    while (!output_.empty()
           && (output_.back() == '\n' || output_.back() == '\r')) {
        output_.pop_back();
    }

    MprisPollResult result;
    result.startedAtMs = startedAtMs_;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        result.error = "MPRIS helper failed";
        output_.clear();
        return result;
    }
    if (!output_.empty()) {
        result.state = parseMprisState(output_, result.error);
    }
    output_.clear();
    return result;
}

void MprisPoller::stop() {
    if (childPid_ > 0) {
        kill(childPid_, SIGTERM);
        while (waitpid(childPid_, nullptr, 0) < 0 && errno == EINTR) {}
    }
    if (outputFd_ >= 0) close(outputFd_);
    childPid_ = -1;
    outputFd_ = -1;
    output_.clear();
}
