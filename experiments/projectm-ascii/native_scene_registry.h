#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

enum class NativeSceneKind : std::uint8_t {
    DepthTunnel = 0,
    Centrifuge = 1,
    WireOrganism = 2,
    PrismGarden = 3,
    OrbitalLoom = 4,
    TidalGrid = 5,
    PulseCathedral = 6,
    ConstellationField = 7,
    SpectralRibbons = 8,
    BloomEngine = 9,
    NegativeSpace = 10,
    InkCurrent = 11,
    GlassChoir = 12,
    ShadowArchitecture = 13,
    ParticleWeave = 14,
    LivingMosaic = 15,
    LumenFold = 16,
    PaperHorizon = 17,
};

inline constexpr std::size_t nativeSceneCount = 18;
inline constexpr unsigned int nativeSceneRegistryVersion = 20;

enum class NativeTransitionAnchor : std::uint8_t {
    Center,
    HorizontalAxis,
    VerticalAxis,
    DepthPoint,
};

enum class NativeMotionGrammar : std::uint8_t {
    Sparse,
    Selective,
    Flow,
};

enum class NativeVisualFamily : std::uint8_t {
    Depth,
    Radial,
    Filament,
    Vertical,
    Landscape,
    Network,
    Minimal,
    Fluid,
    Faceted,
    Cellular,
};

enum class NativeTransitionStyle : std::uint8_t {
    FlowCarry = 6,
    FocalMorph = 7,
    DepthTravel = 8,
    ControlledFracture = 9,
    NegativeSpaceReveal = 10,
};

enum class NativeDirectorProfile : std::uint8_t {
    Balanced,
    Kinetic,
    Restrained,
    HighContrast,
};

inline constexpr std::string_view nativeTransitionStyleName(
    NativeTransitionStyle style) {
    switch (style) {
        case NativeTransitionStyle::FlowCarry: return "flow-carry";
        case NativeTransitionStyle::FocalMorph: return "focal-morph";
        case NativeTransitionStyle::DepthTravel: return "depth-travel";
        case NativeTransitionStyle::ControlledFracture:
            return "controlled-fracture";
        case NativeTransitionStyle::NegativeSpaceReveal:
            return "negative-space-reveal";
    }
    return "unknown";
}

struct NativeTransitionContext {
    float energy = 0.5f;
    float percussive = 0.5f;
    float harmonic = 0.5f;
    float rhythmicDensity = 0.5f;
    float harmonicChange = 0.0f;
    float energySlope = 0.0f;
};

enum NativeMusicalRole : std::uint8_t {
    KickRole = 1 << 0,
    SnareRole = 1 << 1,
    HatRole = 1 << 2,
    GrooveRole = 1 << 3,
    HarmonyRole = 1 << 4,
    StructureRole = 1 << 5,
};

struct NativeSceneMaterial {
    float fieldExposure;
    float asciiExposure;
};

struct NativeSceneSelectionTraits {
    float energy;
    float percussive;
    float harmonic;
    float centroid;
    float stereo;
};

struct NativeTransitionGeometry {
    std::array<float, 2> focalPoint{0.5f, 0.5f};
    std::array<float, 2> motionVector{1.0f, 0.0f};
    float depthStrength = 0.0f;
};

inline constexpr NativeTransitionGeometry makeNativeTransitionGeometry(
        float focalX, float focalY, float motionX, float motionY,
        float depthStrength = 0.0f) {
    return {{focalX, focalY}, {motionX, motionY}, depthStrength};
}

struct NativeSceneDefinition {
    NativeSceneKind kind;
    std::string_view slug;
    std::string_view name;
    std::string_view shader;
    std::array<std::string_view, 3> aliases;
    NativeSceneMaterial material;
    NativeSceneSelectionTraits selection;
    NativeTransitionAnchor transitionAnchor;
    NativeMotionGrammar motionGrammar;
    NativeVisualFamily visualFamily;
    float maximumQuietMotionCoverage;
    float maximumGlobalPulse;
    std::uint8_t musicalRoles;
    float maximumFrameMilliseconds;
    NativeTransitionGeometry transitionGeometry;
};

inline constexpr std::uint8_t transientRoles
    = KickRole | SnareRole | HatRole;

inline constexpr std::array<NativeSceneDefinition, nativeSceneCount>
nativeSceneRegistry{{
    {NativeSceneKind::DepthTunnel, "depth-tunnel", "Depth Tunnel",
     "depth-tunnel.frag", {"depth", "tunnel", ""}, {1.05f, 1.08f},
     {0.72f, 0.58f, 0.42f, 0.24f, 0.34f}, NativeTransitionAnchor::DepthPoint,
     NativeMotionGrammar::Flow, NativeVisualFamily::Depth, 0.50f, 0.55f,
     transientRoles | GrooveRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.50f, 0.0f, -1.0f, 1.0f)},
    {NativeSceneKind::Centrifuge, "centrifuge", "Centrifuge",
     "centrifuge.frag", {"", "", ""}, {0.92f, 1.00f},
     {0.82f, 0.86f, 0.28f, 0.66f, 0.44f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, NativeVisualFamily::Radial, 0.18f, 0.30f,
     transientRoles | GrooveRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.50f, 0.72f, 0.69f, 0.35f)},
    {NativeSceneKind::WireOrganism, "wire-organism", "Wire Organism",
     "wire-organism.frag", {"wire", "", ""}, {1.26f, 1.16f},
     {0.46f, 0.34f, 0.82f, 0.44f, 0.62f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Sparse, NativeVisualFamily::Filament, 0.12f, 0.28f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.53f, 0.55f, 0.84f, 0.20f)},
    {NativeSceneKind::PrismGarden, "prism-garden", "Prism Garden",
     "prism-garden.frag", {"prism", "garden", ""}, {1.10f, 1.08f},
     {0.54f, 0.28f, 0.88f, 0.78f, 0.48f}, NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Sparse, NativeVisualFamily::Vertical, 0.12f, 0.25f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.60f, 0.0f, 1.0f, 0.20f)},
    {NativeSceneKind::OrbitalLoom, "orbital-loom", "Orbital Loom",
     "orbital-loom.frag", {"orbit", "loom", ""}, {1.02f, 1.06f},
     {0.60f, 0.44f, 0.74f, 0.54f, 0.92f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, NativeVisualFamily::Radial, 0.28f, 0.35f,
     transientRoles | GrooveRole | HarmonyRole, 6.0f,
     makeNativeTransitionGeometry(0.44f, 0.48f, 0.92f, 0.38f, 0.24f)},
    {NativeSceneKind::TidalGrid, "tidal-grid", "Tidal Grid",
     "tidal-grid.frag", {"tide", "grid", ""}, {1.12f, 1.10f},
     {0.34f, 0.24f, 0.84f, 0.20f, 0.66f}, NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Selective, NativeVisualFamily::Landscape, 0.26f, 0.25f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.64f, 1.0f, 0.0f, 0.35f)},
    {NativeSceneKind::PulseCathedral, "pulse-cathedral", "Pulse Cathedral",
     "pulse-cathedral.frag", {"cathedral", "", ""}, {1.08f, 1.05f},
     {0.48f, 0.26f, 0.96f, 0.36f, 0.34f}, NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Selective, NativeVisualFamily::Radial, 0.30f, 0.38f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.48f, 0.0f, 1.0f, 0.30f)},
    {NativeSceneKind::ConstellationField, "constellation-field",
     "Constellation Field", "constellation-field.frag",
     {"stars", "constellation", ""}, {1.22f, 1.16f},
     {0.24f, 0.18f, 0.74f, 0.72f, 0.76f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Sparse, NativeVisualFamily::Network, 0.12f, 0.45f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.52f, 0.48f, 0.80f, 0.60f, 0.15f)},
    {NativeSceneKind::SpectralRibbons, "spectral-ribbons", "Spectral Ribbons",
     "spectral-ribbons.frag", {"ribbons", "", ""}, {1.04f, 1.06f},
     {0.64f, 0.56f, 0.64f, 0.62f, 0.72f}, NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Flow, NativeVisualFamily::Filament, 0.45f, 0.35f,
     transientRoles | GrooveRole | HarmonyRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.50f, 1.0f, 0.0f, 0.20f)},
    {NativeSceneKind::BloomEngine, "bloom-engine", "Bloom Engine",
     "bloom-engine.frag", {"bloom", "", ""}, {1.00f, 1.04f},
     {0.74f, 0.66f, 0.66f, 0.48f, 0.54f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, NativeVisualFamily::Radial, 0.18f, 0.30f,
     transientRoles | GrooveRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.50f, 0.71f, 0.71f, 0.25f)},
    {NativeSceneKind::NegativeSpace, "negative-space", "Negative Space",
     "negative-space.frag", {"negative", "void", ""}, {1.08f, 1.14f},
     {0.18f, 0.22f, 0.78f, 0.40f, 0.38f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Sparse, NativeVisualFamily::Minimal, 0.10f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.46f, 0.52f, 1.0f, 0.0f, 0.10f)},
    {NativeSceneKind::InkCurrent, "ink-current", "Ink Current",
     "ink-current.frag", {"ink", "current", ""}, {1.12f, 1.18f},
     {0.38f, 0.34f, 0.82f, 0.30f, 0.72f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Selective, NativeVisualFamily::Fluid, 0.18f, 0.24f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.52f, 0.99f, 0.14f, 0.25f)},
    {NativeSceneKind::GlassChoir, "glass-choir", "Glass Choir",
     "glass-choir.frag", {"glass", "choir", ""}, {1.16f, 1.20f},
     {0.32f, 0.18f, 0.96f, 0.60f, 0.54f},
     NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Selective, NativeVisualFamily::Faceted, 0.18f, 0.24f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.46f, 0.0f, 1.0f, 0.30f)},
    {NativeSceneKind::ShadowArchitecture, "shadow-architecture",
     "Shadow Architecture", "shadow-architecture.frag",
     {"shadow", "architecture", ""}, {1.20f, 1.50f},
     {0.28f, 0.30f, 0.70f, 0.24f, 0.36f},
     NativeTransitionAnchor::DepthPoint,
     NativeMotionGrammar::Sparse, NativeVisualFamily::Depth, 0.12f, 0.20f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.55f, 0.0f, -1.0f, 1.0f)},
    {NativeSceneKind::ParticleWeave, "particle-weave", "Particle Weave",
     "particle-weave.frag", {"particle", "weave", ""}, {1.14f, 1.34f},
     {0.52f, 0.60f, 0.76f, 0.66f, 0.82f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Selective, NativeVisualFamily::Filament, 0.18f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.50f, 0.995f, 0.10f, 0.20f)},
    {NativeSceneKind::LivingMosaic, "living-mosaic", "Living Mosaic",
     "living-mosaic.frag", {"living", "mosaic", ""}, {1.12f, 2.05f},
     {0.48f, 0.44f, 0.86f, 0.72f, 0.56f},
     NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, NativeVisualFamily::Cellular, 0.18f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.52f, 0.50f, 0.71f, 0.71f, 0.15f)},
    {NativeSceneKind::LumenFold, "lumen-fold", "Lumen Fold",
     "lumen-fold.frag", {"lumen", "fold", ""}, {1.18f, 2.30f},
     {0.40f, 0.38f, 0.94f, 0.60f, 0.68f},
     NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Selective, NativeVisualFamily::Vertical, 0.18f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.47f, 0.0f, 1.0f, 0.30f)},
    {NativeSceneKind::PaperHorizon, "paper-horizon", "Paper Horizon",
     "paper-horizon.frag", {"paper", "horizon", ""}, {1.08f, 2.25f},
     {0.34f, 0.32f, 0.90f, 0.42f, 0.40f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Sparse, NativeVisualFamily::Landscape, 0.12f, 0.20f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f,
     makeNativeTransitionGeometry(0.50f, 0.66f, 1.0f, 0.0f, 0.35f)},
}};

inline constexpr std::string_view nativeMotionGrammarName(
    NativeMotionGrammar grammar) {
    switch (grammar) {
        case NativeMotionGrammar::Sparse: return "sparse";
        case NativeMotionGrammar::Selective: return "selective";
        case NativeMotionGrammar::Flow: return "flow";
    }
    return "unknown";
}

inline constexpr std::size_t nativeSceneMotionGrammarCount(
    NativeMotionGrammar grammar) {
    std::size_t count = 0;
    for (const NativeSceneDefinition& definition : nativeSceneRegistry) {
        if (definition.motionGrammar == grammar) ++count;
    }
    return count;
}

inline constexpr std::string_view nativeVisualFamilyName(
    NativeVisualFamily family) {
    switch (family) {
        case NativeVisualFamily::Depth: return "depth";
        case NativeVisualFamily::Radial: return "radial";
        case NativeVisualFamily::Filament: return "filament";
        case NativeVisualFamily::Vertical: return "vertical";
        case NativeVisualFamily::Landscape: return "landscape";
        case NativeVisualFamily::Network: return "network";
        case NativeVisualFamily::Minimal: return "minimal";
        case NativeVisualFamily::Fluid: return "fluid";
        case NativeVisualFamily::Faceted: return "faceted";
        case NativeVisualFamily::Cellular: return "cellular";
    }
    return "unknown";
}

inline constexpr std::size_t nativeSceneVisualFamilyCount(
    NativeVisualFamily family) {
    std::size_t count = 0;
    for (const NativeSceneDefinition& definition : nativeSceneRegistry) {
        if (definition.visualFamily == family) ++count;
    }
    return count;
}

inline constexpr NativeTransitionStyle nativeTransitionStyle(
    NativeSceneKind source, NativeSceneKind incoming,
    const NativeTransitionContext& context) {
    const NativeSceneDefinition& sourceDefinition
        = nativeSceneRegistry[static_cast<std::size_t>(source)];
    const NativeSceneDefinition& incomingDefinition
        = nativeSceneRegistry[static_cast<std::size_t>(incoming)];

    // Composition compatibility has priority. These scenes expose a specific
    // landmark or material that should survive every musical context.
    if (source == NativeSceneKind::NegativeSpace
        || incoming == NativeSceneKind::NegativeSpace) {
        return NativeTransitionStyle::NegativeSpaceReveal;
    }
    if (source == NativeSceneKind::ShadowArchitecture
        || incoming == NativeSceneKind::ShadowArchitecture
        || sourceDefinition.transitionAnchor == NativeTransitionAnchor::DepthPoint
        || incomingDefinition.transitionAnchor == NativeTransitionAnchor::DepthPoint) {
        return NativeTransitionStyle::DepthTravel;
    }
    if (source == NativeSceneKind::GlassChoir
        || incoming == NativeSceneKind::GlassChoir) {
        return NativeTransitionStyle::ControlledFracture;
    }
    if (source == NativeSceneKind::InkCurrent
        || incoming == NativeSceneKind::InkCurrent
        || sourceDefinition.motionGrammar == NativeMotionGrammar::Flow
        || incomingDefinition.motionGrammar == NativeMotionGrammar::Flow) {
        return NativeTransitionStyle::FlowCarry;
    }

    // For otherwise compatible scenes, let the arrangement choose the
    // character of the change. This is sampled once when the director starts
    // the transition, never from momentary hits during the transition.
    const float energy = context.energy < 0.0f ? 0.0f
                       : context.energy > 1.0f ? 1.0f : context.energy;
    if (context.energySlope < -0.08f
        || (energy < 0.28f && context.rhythmicDensity < 0.24f)) {
        return NativeTransitionStyle::NegativeSpaceReveal;
    }
    if (context.harmonicChange > 0.22f && context.harmonic > 0.42f) {
        return NativeTransitionStyle::ControlledFracture;
    }
    if (context.energySlope > 0.08f
        || (context.rhythmicDensity > 0.70f
            && context.percussive > 0.55f)) {
        return NativeTransitionStyle::FlowCarry;
    }
    return NativeTransitionStyle::FocalMorph;
}

inline constexpr NativeTransitionStyle nativeTransitionStyle(
    NativeSceneKind source, NativeSceneKind incoming) {
    return nativeTransitionStyle(source, incoming, NativeTransitionContext{});
}

inline constexpr int nativeTransitionMode(NativeSceneKind source,
                                          NativeSceneKind incoming) {
    return static_cast<int>(nativeTransitionStyle(source, incoming));
}

inline constexpr int nativeTransitionMode(
    NativeSceneKind source, NativeSceneKind incoming,
    const NativeTransitionContext& context) {
    return static_cast<int>(nativeTransitionStyle(source, incoming, context));
}

static_assert(nativeSceneMotionGrammarCount(NativeMotionGrammar::Flow)
              <= nativeSceneCount / 3,
              "flow scenes must not dominate the native scene library");

inline const NativeSceneDefinition& nativeSceneDefinition(NativeSceneKind scene) {
    return nativeSceneRegistry[static_cast<std::size_t>(scene)];
}

inline const char* nativeSceneName(NativeSceneKind scene) {
    return nativeSceneDefinition(scene).name.data();
}

inline NativeSceneMaterial nativeSceneMaterial(NativeSceneKind scene) {
    return nativeSceneDefinition(scene).material;
}

inline bool nativeSceneFromName(std::string_view name, NativeSceneKind& scene) {
    for (const NativeSceneDefinition& definition : nativeSceneRegistry) {
        if (name == definition.slug || name == definition.name) {
            scene = definition.kind;
            return true;
        }
        for (const std::string_view alias : definition.aliases) {
            if (!alias.empty() && name == alias) {
                scene = definition.kind;
                return true;
            }
        }
    }
    return false;
}

inline NativeSceneKind nativeSceneOffset(NativeSceneKind scene, int direction) {
    const int count = static_cast<int>(nativeSceneCount);
    const int index = static_cast<int>(scene);
    return static_cast<NativeSceneKind>((index + direction % count + count) % count);
}
