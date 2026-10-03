#include "../src/audio.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

enum class Fixture { Music, Silence, Invalid, Onset, OppositeOnset };

void writeFixture(const QString& path, Fixture kind) {
    QByteArray bytes;
    bytes.resize(3 * 44100 * 2 * int(sizeof(float)));
    for (int frame = 0; frame < 3 * 44100; ++frame) {
        const double seconds = double(frame) / 44100;
        // Unequal amplitudes verify that weak roles do not normalize to full.
        float value = kind == Fixture::Silence ? 0 : float(0.35 * std::sin(2 * 3.141592653589793 * 90 * seconds)
            + 0.012 * std::sin(2 * 3.141592653589793 * 3000 * seconds));
        // The silent lead-in warms onset detection before a clear bass attack.
        if ((kind == Fixture::Onset || kind == Fixture::OppositeOnset)
            && (seconds < 0.5 || seconds >= 0.75)) value = 0;
        if (kind == Fixture::Invalid && frame == 1000)
            value = std::numeric_limits<float>::quiet_NaN();
        for (int channel = 0; channel < 2; ++channel) {
            const float sample = kind == Fixture::OppositeOnset && channel == 1 ? -value : value;
            quint32 bits;
            std::memcpy(&bits, &sample, sizeof(sample));
            qToLittleEndian(bits, bytes.data() + (frame * 2 + channel) * sizeof(float));
        }
    }
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "create fixture");
    require(file.write(bytes) == bytes.size(), "write fixture");
}

void equal(const Journey::Audio& a, const Journey::Audio& b, const char* message) {
    for (std::size_t i = 0; i < a.bands.size(); ++i)
        require(std::abs(a.bands[i] - b.bands[i]) < 1e-10, message);
    require(std::abs(a.bass - b.bass) < 1e-10, message);
    require(std::abs(a.accent - b.accent) < 1e-10, message);
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    require(directory.isValid(), "temporary directory");
    const QString music = directory.filePath(QStringLiteral("music.f32"));
    const QString silence = directory.filePath(QStringLiteral("silence.f32"));
    const QString invalid = directory.filePath(QStringLiteral("invalid.f32"));
    const QString onset = directory.filePath(QStringLiteral("onset.f32"));
    const QString opposite = directory.filePath(QStringLiteral("opposite-onset.f32"));
    writeFixture(music, Fixture::Music);
    writeFixture(silence, Fixture::Silence);
    writeFixture(invalid, Fixture::Invalid);
    writeFixture(onset, Fixture::Onset);
    writeFixture(opposite, Fixture::OppositeOnset);
    QString error;
    Journey::AudioTimeline full, offset, quiet;
    require(full.load(music, 0, 3, error), "load full fixture");
    constexpr double start = 0.5 + 1.0 / 120;
    require(offset.load(music, start, 0.8, error), "load offset fixture");
    equal(offset.at(0), full.at(start), "offset warms from the beginning");
    equal(offset.at(0.123), full.at(start + 0.123), "exact offset movie lookup");
    const auto left = full.at(0.5), right = full.at(0.5 + 1.0 / 60);
    Journey::Audio middle;
    for (std::size_t i = 0; i < middle.bands.size(); ++i)
        middle.bands[i] = (left.bands[i] + right.bands[i]) * 0.5;
    middle.bass = (left.bass + right.bass) * 0.5;
    middle.accent = (left.accent + right.accent) * 0.5;
    equal(full.at(start), middle, "interpolate between analysis frames");
    require(left.bands[1] > left.bands[4] * 4, "preserve absolute balance between roles");
    require(left.bands[1] > 0.1 && left.bands[1] < 0.95, "strong role has useful headroom");
    for (int frame = 0; frame <= 180; ++frame) {
        const auto value = full.at(double(frame) / 60);
        for (double band : value.bands)
            require(std::isfinite(band) && band >= 0 && band <= 1, "bands stay finite and bounded");
        require(std::isfinite(value.bass) && value.bass >= 0 && value.bass <= 1, "bounded bass");
        require(std::isfinite(value.accent) && value.accent >= 0 && value.accent <= 1, "bounded accent");
    }
    Journey::AudioTimeline identicalChannels, oppositeChannels;
    require(identicalChannels.load(onset, 0, 3, error), "load identical-channel onset");
    require(oppositeChannels.load(opposite, 0, 3, error), "load opposite-phase onset");
    double onsetBass = 0, onsetAccent = 0;
    for (int frame = 0; frame <= 360; ++frame) {
        const double time = double(frame) / 120;
        const auto value = oppositeChannels.at(time);
        equal(value, identicalChannels.at(time), "all features survive opposite-phase stereo");
        if (time >= 0.5 && time <= 0.75) {
            onsetBass = std::max(onsetBass, value.bass);
            onsetAccent = std::max(onsetAccent, value.accent);
        }
    }
    require(onsetBass > 0.1, "opposite-phase onset has meaningful bass");
    require(onsetAccent > 0.1, "opposite-phase onset has meaningful accent");
    require(quiet.load(silence, 0.25, 1, error), "load silence");
    equal(quiet.at(0.321), {}, "silence produces zero response");
    equal(full.at(std::numeric_limits<double>::quiet_NaN()), {}, "nonfinite lookup is silent");
    require(!quiet.load(music, 2, 2, error) && error.contains(QStringLiteral("too short")), "short audio is rejected");
    equal(quiet.at(0), {}, "failed reload clears old features");
    require(!quiet.load(invalid, 0, 1, error) && error.contains(QStringLiteral("Invalid float32")), "NaN audio is rejected");
    require(!quiet.load(directory.filePath(QStringLiteral("missing")), 0, 1, error) && !error.isEmpty(), "missing audio fails usefully");
    require(!quiet.load(music, -1, 1, error), "negative offset rejected");
    require(!quiet.load(music, 0, std::numeric_limits<double>::infinity(), error), "nonfinite duration rejected");
    QFile malformed(directory.filePath(QStringLiteral("malformed.f32")));
    require(malformed.open(QIODevice::WriteOnly), "create malformed fixture");
    malformed.write("123", 3);
    malformed.close();
    require(!quiet.load(malformed.fileName(), 0, 1, error) && error.contains(QStringLiteral("complete stereo")), "partial stereo frame rejected");
    std::puts("Journey audio: offset, interpolation, balance, stereo phase, silence and invalid input passed.");
    return 0;
}
