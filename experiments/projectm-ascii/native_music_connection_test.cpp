#include "native_renderer.h"
#include <SDL.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

// A prototype contract, independent of the old sparse-motion scorecard.
// Compare identical clock/feedback histories so motion cannot impersonate audio.
namespace {
constexpr int width = 640, height = 360;
using Image = std::vector<float>;
void require(bool okay, const std::string& message) {
    if (!okay) throw std::runtime_error(message);
}
Image read(NativeRenderer& renderer) {
    Image pixels(width * height * 4);
    glBindTexture(GL_TEXTURE_2D, renderer.texture(NativeSceneKind::InkCurrent));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
    for (float value : pixels)
        require(std::isfinite(value) && value >= 0.0f && value <= 1.001f,
                "non-finite or out-of-range pixel");
    return pixels;
}
float difference(const Image& a, const Image& b) {
    double sum = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (i % 4 != 3) sum += std::abs(a[i] - b[i]);
    return sum / (a.size() * 0.75);
}
void save(const Image& a, const std::filesystem::path& file) {
    std::ofstream out(file, std::ios::binary);
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        for (int x = 0; x < width; ++x)
            for (int c = 0; c < 3; ++c) {
                const auto value = static_cast<unsigned char>(255.0f * std::clamp(
                    a[(y * width + x) * 4 + c], 0.0f, 1.0f));
                out.write(reinterpret_cast<const char*>(&value), 1);
            }
}
}
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* window = SDL_CreateWindow("Music connection probe", 0, 0,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window) return 1;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (!context) return 1;
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return 1;
    int status = 0;
    {
        NativeRenderer renderer;
        std::string error;
        try {
            // The preview must hold through musical boundaries while keeping
            // deliberate next-scene requests available.
            NativeSceneDirector held;
            held.selectScene(NativeSceneKind::InkCurrent);
            MusicFrame driving;
            driving.energyFast = driving.energySlow = 0.8f;
            driving.bpm = 120.0f;
            for (int i = 0; i < 6000; ++i) {
                driving.section = i % 120 == 0 ? 1.0f : 0.0f;
                const auto& state = held.update(driving, 1.0f / 60.0f, false);
                require(!state.transitioning && state.currentScene == NativeSceneKind::InkCurrent,
                        "held preview changed scene automatically");
            }
            held.requestNext();
            for (int i = 0; i < 300; ++i) held.update(driving, 1.0f / 60.0f, false);
            require(!held.state().transitioning
                    && held.state().currentScene == NativeSceneKind::GlassChoir,
                    "held preview ignored manual next");
            std::cout << "preview hold and manual next passed\n";
            require(renderer.initialize(argv[1], error), error);
            std::filesystem::create_directories(argv[2]);
            NativeSceneState scene;
            scene.currentScene = NativeSceneKind::InkCurrent;
            scene.development = 0.8f;
            scene.drive = 0.5f;
            MusicFrame quiet;
            auto render = [&](const MusicFrame& frame, float dt,
                              NativeRenderPolicy policy = {}) {
                // Pin the scene clock while allowing physical response to
                // develop. Zero elapsed simulation time cannot move a spring.
                if (dt == 0.0f) {
                    renderer.synchronizeFlowTime(2.0f);
                    dt = 1.0f / 60.0f;
                }
                require(renderer.render(frame, scene, width, height,
                    {0.46f, 0.72f, 1.0f}, 0, 1.0f, dt, error, policy), error);
            };
            auto baseline = [&]() {
                renderer.reset();
                renderer.synchronizeFlowTime(2.0f);
                for (int i = 0; i < 12; ++i) render(quiet, 0.0f);
                return read(renderer);
            };
            const Image reference = baseline();
            save(reference, std::filesystem::path(argv[2]) / "quiet.ppm");
            std::array<Image, 6> responses;
            const std::array<const char*, 6> names{
                "kick", "snare", "hat", "bass", "mid", "treble"};
            for (int role = 0; role < 6; ++role) {
                baseline();
                MusicFrame music;
                if (role == 0) music.kick = 0.8f;
                if (role == 1) music.snare = 0.8f;
                if (role == 2) music.hat = 0.8f;
                if (role >= 3) {
                    const int band = (role - 3) * 2;
                    music.bandLevel[band] = music.bandLevel[band + 1] = 1.0f;
                    for (int i = (role - 3) * 10; i < std::min(32, (role - 2) * 10); ++i)
                        music.spectrumLevel[i] = 1.0f;
                }
                render(music, 0.0f);
                responses[role] = read(renderer);
                const float immediate = difference(reference, responses[role]);
                // Attacks remain immediate. Sustained bands may begin gently
                // while their physical response develops; the settled response
                // below must still cross the original 0.006 image threshold.
                const float minimum = role >= 3 ? 0.0002f : role == 2 ? 0.003f : 0.006f;
                require(immediate > minimum,
                        std::string(names[role]) + " response is too weak");
                if (role >= 3) {
                    for (int i = 0; i < 60; ++i) render(music, 0.0f);
                    require(difference(reference, read(renderer)) > 0.006f,
                            "sustained response vanished");
                }
                for (int i = 0; i < 30; ++i) render(quiet, 0.0f);
                const float recovery = difference(reference, read(renderer));
                require(recovery < 0.001f, "response did not recover");
                save(responses[role], std::filesystem::path(argv[2]) /
                     (std::string(names[role]) + ".ppm"));
                std::cout << names[role] << " immediate=" << immediate
                          << " recovery=" << recovery << '\n';
            }
            for (int a = 0; a < 6; ++a)
                for (int b = a + 1; b < 6; ++b)
                    require(difference(responses[a], responses[b]) > 0.003f,
                            "musical roles produce indistinguishable frames");
            auto motion = [&](MusicFrame music, NativeRenderPolicy policy) {
                baseline();
                for (int i = 0; i < 30; ++i) render(music, 1.0f / 60.0f, policy);
                Image previous = read(renderer);
                double total = 0;
                for (int i = 0; i < 90; ++i) {
                    render(music, 1.0f / 60.0f, policy);
                    auto current = read(renderer);
                    total += difference(previous, current);
                    previous = std::move(current);
                }
                return total / 90;
            };
            MusicFrame music;
            music.energySlow = music.energyFast = 0.6f;
            music.harmonic = 0.6f;
            music.bandLevel.fill(0.65f);
            music.spectrumLevel.fill(0.65f);
            // Holding the sound must hold the picture even while scene time
            // and inferred rhythm advance. Only changing measurements may move it.
            const double silence = motion({}, {});
            const double heldMotion = motion(music, {});
            std::cout << "motion silence=" << silence << " held=" << heldMotion << '\n';
            require(silence < 0.00001, "silence invents movement");
            require(heldMotion < 0.00005, "held sound invents movement");
            baseline();
            for (int i = 0; i < 120; ++i) render(music, 1.0f / 60.0f);
            const auto heldImage = read(renderer);
            for (int i = 0; i < 120; ++i) {
                music.beatPulse = i % 2;
                music.downbeat = i % 3 == 0;
                music.clockConfidence = 1.0f;
                music.beatAnticipation = (i % 10) / 10.0f;
                music.section = i % 2;
                music.harmonicChange = i % 3 == 0;
                renderer.synchronizeFlowTime(i * 10.0f);
                render(music, 1.0f / 60.0f);
            }
            require(difference(heldImage, read(renderer)) < 0.00001f,
                    "predicted rhythm or scene time moved the picture");
            // A real hit moves the body, then settles within half a second.
            baseline();
            MusicFrame hit;
            hit.kick = 1.0f;
            render(hit, 1.0f / 60.0f);
            render(quiet, 1.0f / 60.0f);
            require(difference(reference, read(renderer)) > 0.001f,
                    "hit has no settling response");
            for (int i = 0; i < 30; ++i) render(quiet, 1.0f / 60.0f);
            require(difference(reference, read(renderer)) < 0.001f,
                    "hit did not settle within half a second");
            baseline();
            for (int i = 0; i < 20; ++i)
                require(renderer.render(music, scene, 1920, 1080,
                    {0.46f, 0.72f, 1.0f}, 0, 1.0f, 1.0f / 60.0f, error), error);
            glFinish();
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < 120; ++i)
                require(renderer.render(music, scene, 1920, 1080,
                    {0.46f, 0.72f, 1.0f}, 0, 1.0f, 1.0f / 60.0f, error), error);
            glFinish();
            const double ms = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count() / 120;
            std::cout << "1080p frame_ms=" << ms << '\n';
            require(ms < 16.67, "1080p renderer exceeds 60 FPS budget");
            std::cout << "music connection contract passed\n";
        } catch (const std::exception& exception) {
            std::cerr << exception.what() << '\n';
            status = 1;
        }
    }
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return status;
}
