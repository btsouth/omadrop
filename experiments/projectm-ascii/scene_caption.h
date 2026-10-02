#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <cstddef>

// Observe presentation state without changing the scene or cover clocks.
class SceneCaption {
public:
    bool update(std::size_t scene, bool transitioning, bool coverComplete,
                bool windowShown) {
        if (transitioning) pending_ = true;
        if (!windowShown || transitioning || !coverComplete) return false;
        if (!activeScene_ || *activeScene_ != scene || pending_) {
            activeScene_ = scene;
            pending_ = false;
            return true;
        }
        return false;
    }

    static float opacity(std::uint64_t age) {
        if (age < 400) return age / 400.0f;
        if (age < 3400) return 1.0f;
        if (age < 4200) return (4200 - age) / 800.0f;
        return 0.0f;
    }

private:
    std::optional<std::size_t> activeScene_;
    bool pending_ = false;
};
