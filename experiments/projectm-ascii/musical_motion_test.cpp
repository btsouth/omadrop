#include "musical_motion.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    // Same elapsed time and target must produce the same trajectory at every
    // display rate. This catches per-frame lerps and unstable Euler springs.
    auto sample = [](int hz) {
        MusicalSpring spring;
        for (int i = 0; i < hz / 5; ++i) spring.update(1.0f, 18.0f, 1.0f / hz);
        return spring;
    };
    const auto reference = sample(60);
    for (int hz : {120, 165, 240}) {
        const auto current = sample(hz);
        assert(std::abs(reference.position - current.position) < 0.00001f);
        assert(std::abs(reference.velocity - current.velocity) < 0.0001f);
    }
    MusicalSpring spring;
    float previous = 0;
    for (int i = 0; i < 1000; ++i) {
        const float position = spring.update(1.0f, 85.0f, 1.0f / 165.0f);
        assert(std::isfinite(position));
        assert(position >= previous - 0.000001f && position <= 1.000001f);
        previous = position;
    }
    // A change of target changes acceleration, not position or velocity at
    // zero elapsed time. Instant shape jumps were the original defect.
    const float before = spring.position, velocity = spring.velocity;
    spring.update(0.0f, 85.0f, 0.0f);
    assert(spring.position == before && spring.velocity == velocity);
    for (int i = 0; i < 165; ++i) spring.update(0.0f, 85.0f, 1.0f / 165.0f);
    assert(std::abs(spring.position) < 0.000001f);
    assert(MusicalMotion::impactStrength(1.2f, 1.0f)
           > 2.5f * MusicalMotion::impactStrength(0.45f, 1.0f));
    assert(MusicalMotion::impactStrength(10.0f, 1.5f) == 1.0f);
    MusicalMotion motion;
    MusicFrame music;
    music.kick = 1.0f;
    music.bandLevel[0] = 1.0f;
    for (int i = 0; i < 30; ++i) motion.update(music, 1.0f / 165.0f);
    const auto state = motion.update(music, 1.0f / 165.0f);
    assert(state.impact[0] > 0.7f && state.impact[1] == 0.0f && state.impact[2] == 0.0f);
    assert(state.bands[0] > 0.9f && state.bands[1] == 0.0f);
    // A one-packet percussion event must become a short traveling gesture,
    // not a near-instant flash. Check its peak, adjacent-frame jump and tail
    // at the actual high-refresh presentation rate.
    MusicalMotion impulse;
    float last = 0.0f, peak = 0.0f, largestJump = 0.0f;
    for (int frame = 0; frame < 165; ++frame) {
        MusicFrame packet;
        if (frame < 3) packet.kick = 1.0f;
        const float current = impulse.update(packet, 1.0f / 165.0f).impact[0];
        peak = std::max(peak, current);
        largestJump = std::max(largestJump, std::abs(current-last));
        if (frame >= 83) assert(current < 0.0001f);
        last = current;
    }
    assert(peak > 0.10f && peak < 0.25f);
    assert(largestJump < 0.04f);
    motion.reset();
    assert(motion.update({}, 0).impact[0] == 0);
    std::cout << "frame-rate independent motion, continuous trajectories, role isolation, and headroom passed\n";
}
