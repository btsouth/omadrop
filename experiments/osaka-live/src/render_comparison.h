#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <vector>

namespace Journey {
struct RenderComparison {
    bool sameSize;
    std::size_t changed = 0;
    int maximum = 0;
    bool exact() const { return sameSize && changed == 0; }
    // NVIDIA half-float MSAA rounding is history-dependent with bounded
    // rendering, even between repeated uncached draws. Approved limit applies
    // on every GPU, per pose; larger differences remain failures. Evidence:
    // /home/bts/Projects/_evidence/omadrop/igpu-speedups-2026-10-04/round3/report.md
    // /home/bts/Projects/_evidence/omadrop/igpu-speedups-2026-10-04/round4/full-clear-verify-1.log
    bool acceptable() const { return sameSize && changed <= 64 && maximum <= 1; }
};
inline RenderComparison compareRender(const std::vector<unsigned char>& before,
                                      const std::vector<unsigned char>& after) {
    RenderComparison result{before.size() == after.size()};
    for(std::size_t i = 0; i < std::min(before.size(), after.size()); ++i) {
        result.changed += before[i] != after[i];
        result.maximum = std::max(result.maximum, std::abs(int(before[i]) - int(after[i])));
    }
    return result;
}
}
