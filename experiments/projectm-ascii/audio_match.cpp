#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
std::vector<float> readFloats(const char* path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return {};
    const std::streamsize bytes = input.tellg();
    if (bytes <= 0 || bytes % static_cast<std::streamsize>(sizeof(float)) != 0) {
        return {};
    }
    input.seekg(0);
    std::vector<float> values(
        static_cast<std::size_t>(bytes) / sizeof(float));
    input.read(reinterpret_cast<char*>(values.data()), bytes);
    return input ? values : std::vector<float>{};
}

std::vector<float> difference(const std::vector<float>& values) {
    if (values.size() < 2) return {};
    std::vector<float> result(values.size() - 1);
    for (std::size_t index = 1; index < values.size(); ++index) {
        result[index - 1] = values[index] - values[index - 1];
    }
    return result;
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "usage: audio-match CAPTURE_F32_MONO REFERENCE_F32_MONO "
                     "[MINIMUM_CORRELATION]\n";
        return 2;
    }
    const float minimum = argc == 4 ? std::strtof(argv[3], nullptr) : 0.70f;
    if (!(minimum >= 0.0f && minimum <= 1.0f)) {
        std::cerr << "audio-match: correlation must be between zero and one\n";
        return 2;
    }
    const std::vector<float> captured = difference(readFloats(argv[1]));
    const std::vector<float> reference = difference(readFloats(argv[2]));
    if (captured.size() < reference.size() || reference.size() < 100) {
        std::cerr << "audio-match: capture or reference is too short\n";
        return 2;
    }

    double referencePower = 0.0;
    for (const float value : reference) referencePower += value * value;
    if (referencePower < 1e-8) {
        std::cerr << "audio-match: reference opening has insufficient signal\n";
        return 2;
    }

    float best = -std::numeric_limits<float>::infinity();
    std::size_t bestLag = 0;
    const std::size_t maximumLag = captured.size() - reference.size();
    for (std::size_t lag = 0; lag <= maximumLag; ++lag) {
        double dot = 0.0;
        double capturedPower = 0.0;
        for (std::size_t index = 0; index < reference.size(); ++index) {
            const float value = captured[lag + index];
            dot += value * reference[index];
            capturedPower += value * value;
        }
        if (capturedPower < 1e-8) continue;
        const float correlation = static_cast<float>(
            std::abs(dot) / std::sqrt(capturedPower * referencePower));
        if (correlation > best) {
            best = correlation;
            bestLag = lag;
        }
    }

    if (!std::isfinite(best)) best = 0.0f;
    std::cout << "opening_audio correlation=" << best
              << " lag_samples=" << bestLag << "\n";
    if (best < minimum) {
        std::cerr << "audio-match: captured opening does not match the approved audio\n";
        return 1;
    }
}
