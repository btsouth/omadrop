#include "pipewire_capture.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

PipeWireCapture::~PipeWireCapture() {
    stop();
}

bool PipeWireCapture::start(const std::string& targetSink) {
    stop();
    int audioPipe[2];
    if (pipe2(audioPipe, O_CLOEXEC) != 0) return false;
    int execPipe[2];
    if (pipe2(execPipe, O_CLOEXEC) != 0) {
        close(audioPipe[0]);
        close(audioPipe[1]);
        return false;
    }
    const pid_t child = fork();
    if (child == 0) {
        close(execPipe[0]);
        dup2(audioPipe[1], STDOUT_FILENO);
        const int nullFd = open("/dev/null", O_WRONLY);
        if (nullFd >= 0) dup2(nullFd, STDERR_FILENO);
        close(audioPipe[0]);
        close(audioPipe[1]);
        const char* command = std::getenv("OMADROP_PW_RECORD_COMMAND");
        if (!command || !*command) command = "pw-record";
        if (targetSink.empty()) {
            execlp(command, "pw-record", "--raw", "--rate", "44100",
                   "--channels", "2", "--format", "f32", "--latency", "20ms",
                   "-P", "{ stream.capture.sink=true }", "-",
                   static_cast<char*>(nullptr));
        } else {
            execlp(command, "pw-record", "--raw", "--rate", "44100",
                   "--channels", "2", "--format", "f32", "--latency", "20ms",
                   "-P", "{ stream.capture.sink=true }", "--target",
                   targetSink.c_str(), "-", static_cast<char*>(nullptr));
        }
        const int execError = errno;
        static_cast<void>(write(execPipe[1], &execError, sizeof(execError)));
        _exit(127);
    }
    close(audioPipe[1]);
    close(execPipe[1]);
    if (child < 0) {
        close(audioPipe[0]);
        close(execPipe[0]);
        return false;
    }

    int execError = 0;
    ssize_t execBytes = -1;
    do {
        execBytes = ::read(execPipe[0], &execError, sizeof(execError));
    } while (execBytes < 0 && errno == EINTR);
    close(execPipe[0]);
    if (execBytes != 0) {
        close(audioPipe[0]);
        if (execBytes < 0) kill(child, SIGKILL);
        waitpid(child, nullptr, 0);
        return false;
    }

    process_ = child;
    descriptor_ = audioPipe[0];
    fcntl(descriptor_, F_SETFL, fcntl(descriptor_, F_GETFL) | O_NONBLOCK);
    return true;
}

void PipeWireCapture::stop() {
    if (process_ > 0) {
        kill(process_, SIGTERM);
        waitpid(process_, nullptr, 0);
        process_ = -1;
    }
    if (descriptor_ >= 0) {
        close(descriptor_);
        descriptor_ = -1;
    }
}

std::size_t PipeWireCapture::read(float* samples, std::size_t capacity) {
    if (descriptor_ < 0 || !samples || capacity == 0) return 0;
    const ssize_t bytes = ::read(
        descriptor_, samples, capacity * sizeof(float));
    if (bytes <= 0) return 0;
    return static_cast<std::size_t>(bytes) / sizeof(float);
}
