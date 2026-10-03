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

enum class Fixture {
    Music, Silence, Invalid, Onset, OppositeOnset,
    Rise, OppositeRise, CooldownRise, SubFloorRise
};

void writeFixture(const QString& path, Fixture kind, int duration = 3) {
    QByteArray bytes;
    bytes.resize(duration * 44100 * 2 * int(sizeof(float)));
    for (int frame = 0; frame < duration * 44100; ++frame) {
        const double seconds = double(frame) / 44100;
        // Unequal amplitudes verify that weak roles do not normalize to full.
        float value = kind == Fixture::Silence ? 0 : float(0.35 * std::sin(2 * 3.141592653589793 * 90 * seconds)
            + 0.012 * std::sin(2 * 3.141592653589793 * 3000 * seconds));
        // The silent lead-in warms onset detection before a clear bass attack.
        if ((kind == Fixture::Onset || kind == Fixture::OppositeOnset)
            && (seconds < 0.5 || seconds >= 0.75)) value = 0;
        // Unit-test mixtures only: a quiet bed followed by a broad level rise.
        if (kind == Fixture::Rise || kind == Fixture::OppositeRise
            || kind == Fixture::CooldownRise || kind == Fixture::SubFloorRise) {
            bool raised = seconds >= 2;
            if (kind == Fixture::CooldownRise)
                raised = (seconds >= 2 && seconds < 3.5)
                    || (seconds >= 5 && seconds < 6.5)
                    || (seconds >= 10 && seconds < 11.5);
            const double gain = (raised ? 1.0 : 0.04)
                * (kind == Fixture::SubFloorRise ? 0.003 : 1.0);
            value = float(gain * (0.35 * std::sin(2 * 3.141592653589793 * 90 * seconds)
                + 0.18 * std::sin(2 * 3.141592653589793 * 550 * seconds)
                + 0.08 * std::sin(2 * 3.141592653589793 * 3000 * seconds)));
        }
        if (kind == Fixture::Invalid && frame == 1000)
            value = std::numeric_limits<float>::quiet_NaN();
        for (int channel = 0; channel < 2; ++channel) {
            const bool invert = kind == Fixture::OppositeOnset || kind == Fixture::OppositeRise;
            const float sample = invert && channel == 1 ? -value : value;
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
    require(std::abs(a.surge - b.surge) < 1e-10, message);
}

void bounded(const Journey::Audio& value) {
    for (double band : value.bands)
        require(std::isfinite(band) && band >= 0 && band <= 1, "bands stay finite and bounded");
    require(std::isfinite(value.bass) && value.bass >= 0 && value.bass <= 1, "bounded bass");
    require(std::isfinite(value.accent) && value.accent >= 0 && value.accent <= 1, "bounded accent");
    require(std::isfinite(value.surge) && value.surge >= 0 && value.surge <= 1, "bounded surge");
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
    writeFixture(silence, Fixture::Silence, 16);
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
    middle.surge = (left.surge + right.surge) * 0.5;
    equal(full.at(start), middle, "interpolate between analysis frames");
    require(left.bands[1] > left.bands[4] * 4, "preserve absolute balance between roles");
    require(left.bands[1] > 0.1 && left.bands[1] < 0.95, "strong role has useful headroom");
    for (int frame = 0; frame <= 180; ++frame) {
        const auto value = full.at(double(frame) / 60);
        bounded(value);
        require(value.surge == 0, "steady startup level does not invent a surge");
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
    const QString rise = directory.filePath(QStringLiteral("rise.f32"));
    const QString oppositeRise = directory.filePath(QStringLiteral("opposite-rise.f32"));
    const QString cooldownRise = directory.filePath(QStringLiteral("cooldown-rise.f32"));
    const QString subFloorRise = directory.filePath(QStringLiteral("sub-floor-rise.f32"));
    writeFixture(rise, Fixture::Rise, 16);
    writeFixture(oppositeRise, Fixture::OppositeRise, 16);
    writeFixture(cooldownRise, Fixture::CooldownRise, 16);
    writeFixture(subFloorRise, Fixture::SubFloorRise, 16);
    Journey::AudioTimeline rising, risingOpposite, risingOffset, cooldown, cooldownOffset, subFloor;
    require(rising.load(rise, 0, 16, error), "load broad rise");
    require(risingOpposite.load(oppositeRise, 0, 16, error), "load opposite-phase broad rise");
    constexpr double surgeStart = 2.5 + 1.0 / 120;
    require(risingOffset.load(rise, surgeStart, 10, error), "load offset during surge");
    require(cooldown.load(cooldownRise, 0, 16, error), "load repeated rises");
    require(cooldownOffset.load(cooldownRise, 6.2, 7, error), "load offset with cooldown history");
    require(subFloor.load(subFloorRise, 0, 16, error), "load rise below energy floor");
    const auto surgeLeft = rising.at(2.5), surgeRight = rising.at(2.5 + 1.0 / 60);
    require(surgeLeft.surge > 0.2, "meaningful rise produces an obvious surge");
    require(std::abs(surgeLeft.surge - surgeRight.surge) > 1e-6, "surge interpolation checks changing values");
    require(std::abs(rising.at(surgeStart).surge - (surgeLeft.surge + surgeRight.surge) * 0.5)
        < 1e-10, "surge interpolates between analysis frames");
    double peak = 0, latePeak = 0, previous = 0, allowedSecondPeak = 0;
    for (int frame = 0; frame <= 16 * 120; ++frame) {
        const double time = double(frame) / 120;
        const auto value = rising.at(time);
        bounded(value);
        equal(value, risingOpposite.at(time), "surge and roles survive opposite-phase stereo");
        require(subFloor.at(time).surge == 0, "near-silence rise stays below absolute energy floor");
        require(std::abs(value.surge - previous) < 0.035, "surge attack and release avoid flashes");
        if (time < 1) require(value.surge == 0, "surge warmup is silent");
        if (time >= 2 && time < 4) peak = std::max(peak, value.surge);
        if (time >= 8) latePeak = std::max(latePeak, value.surge);
        if (time >= 3)
            require(value.surge <= previous + 1e-10, "sustained level cannot retrigger surge");
        if (time <= 9)
            equal(risingOffset.at(time), rising.at(time + surgeStart), "offset preserves active surge history");
        if (time <= 6)
            equal(cooldownOffset.at(time), cooldown.at(time + 6.2), "offset preserves cooldown and rearming history");
        const auto repeated = cooldown.at(time);
        bounded(repeated);
        if (time >= 5 && time <= 6.5)
            require(repeated.surge <= cooldown.at(time - 1.0 / 120).surge + 1e-10,
                "rise inside six-second cooldown cannot retrigger");
        if (time >= 10 && time < 12) allowedSecondPeak = std::max(allowedSecondPeak, repeated.surge);
        previous = value.surge;
    }
    require(peak > 0.2, "broad rise reaches meaningful surge strength");
    require(latePeak < 0.07, "surge releases while the raised level is sustained");
    require(allowedSecondPeak > 0.2, "fresh rise after cooldown can trigger again");
    require(quiet.load(silence, 0.25, 1, error), "load silence");
    equal(quiet.at(0.321), {}, "silence produces zero response");
    require(quiet.load(silence, 0, 16, error), "load long silence");
    for (int frame = 0; frame <= 16 * 60; ++frame)
        equal(quiet.at(double(frame) / 60), {}, "silence remains zero beyond cooldowns");
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
    std::puts("Journey audio: offset, interpolation, balance, stereo phase, surge, silence and invalid input passed.");
    return 0;
}
