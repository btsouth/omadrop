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
};

inline constexpr std::size_t nativeSceneCount = 17;
inline constexpr unsigned int nativeSceneRegistryVersion = 11;

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

enum class NativeTransitionStyle : std::uint8_t {
    FlowCarry = 6,
    FocalMorph = 7,
    DepthTravel = 8,
    ControlledFracture = 9,
    NegativeSpaceReveal = 10,
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
    float maximumQuietMotionCoverage;
    float maximumGlobalPulse;
    std::uint8_t musicalRoles;
    float maximumFrameMilliseconds;
};

inline constexpr std::uint8_t transientRoles
    = KickRole | SnareRole | HatRole;

inline constexpr std::array<NativeSceneDefinition, nativeSceneCount>
nativeSceneRegistry{{
    {NativeSceneKind::DepthTunnel, "depth-tunnel", "Depth Tunnel",
     "depth-tunnel.frag", {"depth", "tunnel", ""}, {1.05f, 1.08f},
     {0.72f, 0.58f, 0.42f, 0.24f, 0.34f}, NativeTransitionAnchor::DepthPoint,
     NativeMotionGrammar::Flow, 0.50f, 0.55f,
     transientRoles | GrooveRole | StructureRole, 6.0f},
    {NativeSceneKind::Centrifuge, "centrifuge", "Centrifuge",
     "centrifuge.frag", {"", "", ""}, {0.92f, 1.00f},
     {0.82f, 0.86f, 0.28f, 0.66f, 0.44f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, 0.18f, 0.30f,
     transientRoles | GrooveRole | StructureRole, 6.0f},
    {NativeSceneKind::WireOrganism, "wire-organism", "Wire Organism",
     "wire-organism.frag", {"wire", "", ""}, {1.26f, 1.16f},
     {0.46f, 0.34f, 0.82f, 0.44f, 0.62f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Sparse, 0.12f, 0.28f,
     transientRoles | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::PrismGarden, "prism-garden", "Prism Garden",
     "prism-garden.frag", {"prism", "garden", ""}, {1.10f, 1.08f},
     {0.54f, 0.28f, 0.88f, 0.78f, 0.48f}, NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Sparse, 0.12f, 0.25f,
     transientRoles | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::OrbitalLoom, "orbital-loom", "Orbital Loom",
     "orbital-loom.frag", {"orbit", "loom", ""}, {1.02f, 1.06f},
     {0.60f, 0.44f, 0.74f, 0.54f, 0.92f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, 0.28f, 0.35f,
     transientRoles | GrooveRole | HarmonyRole, 6.0f},
    {NativeSceneKind::TidalGrid, "tidal-grid", "Tidal Grid",
     "tidal-grid.frag", {"tide", "grid", ""}, {1.12f, 1.10f},
     {0.34f, 0.24f, 0.84f, 0.20f, 0.66f}, NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Selective, 0.26f, 0.25f,
     transientRoles | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::PulseCathedral, "pulse-cathedral", "Pulse Cathedral",
     "pulse-cathedral.frag", {"cathedral", "", ""}, {1.08f, 1.05f},
     {0.48f, 0.26f, 0.96f, 0.36f, 0.34f}, NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Selective, 0.30f, 0.38f,
     transientRoles | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::ConstellationField, "constellation-field",
     "Constellation Field", "constellation-field.frag",
     {"stars", "constellation", ""}, {1.22f, 1.16f},
     {0.24f, 0.18f, 0.74f, 0.72f, 0.76f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Sparse, 0.12f, 0.45f,
     transientRoles | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::SpectralRibbons, "spectral-ribbons", "Spectral Ribbons",
     "spectral-ribbons.frag", {"ribbons", "", ""}, {1.04f, 1.06f},
     {0.64f, 0.56f, 0.64f, 0.62f, 0.72f}, NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Flow, 0.45f, 0.70f,
     transientRoles | GrooveRole | HarmonyRole, 6.0f},
    {NativeSceneKind::BloomEngine, "bloom-engine", "Bloom Engine",
     "bloom-engine.frag", {"bloom", "", ""}, {1.00f, 1.04f},
     {0.74f, 0.66f, 0.66f, 0.48f, 0.54f}, NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, 0.18f, 0.30f,
     transientRoles | GrooveRole | StructureRole, 6.0f},
    {NativeSceneKind::NegativeSpace, "negative-space", "Negative Space",
     "negative-space.frag", {"negative", "void", ""}, {1.08f, 1.14f},
     {0.18f, 0.22f, 0.78f, 0.40f, 0.38f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Sparse, 0.10f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::InkCurrent, "ink-current", "Ink Current",
     "ink-current.frag", {"ink", "current", ""}, {1.12f, 1.18f},
     {0.38f, 0.34f, 0.82f, 0.30f, 0.72f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Selective, 0.18f, 0.24f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::GlassChoir, "glass-choir", "Glass Choir",
     "glass-choir.frag", {"glass", "choir", ""}, {1.16f, 1.20f},
     {0.32f, 0.18f, 0.96f, 0.60f, 0.54f},
     NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Selective, 0.18f, 0.24f,
     transientRoles | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::ShadowArchitecture, "shadow-architecture",
     "Shadow Architecture", "shadow-architecture.frag",
     {"shadow", "architecture", ""}, {1.20f, 1.50f},
     {0.28f, 0.30f, 0.70f, 0.24f, 0.36f},
     NativeTransitionAnchor::DepthPoint,
     NativeMotionGrammar::Sparse, 0.12f, 0.20f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::ParticleWeave, "particle-weave", "Particle Weave",
     "particle-weave.frag", {"particle", "weave", ""}, {1.14f, 1.34f},
     {0.52f, 0.60f, 0.76f, 0.66f, 0.82f},
     NativeTransitionAnchor::HorizontalAxis,
     NativeMotionGrammar::Selective, 0.18f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::LivingMosaic, "living-mosaic", "Living Mosaic",
     "living-mosaic.frag", {"living", "mosaic", ""}, {1.12f, 2.05f},
     {0.48f, 0.44f, 0.86f, 0.72f, 0.56f},
     NativeTransitionAnchor::Center,
     NativeMotionGrammar::Selective, 0.18f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f},
    {NativeSceneKind::LumenFold, "lumen-fold", "Lumen Fold",
     "lumen-fold.frag", {"lumen", "fold", ""}, {1.18f, 2.30f},
     {0.40f, 0.38f, 0.94f, 0.60f, 0.68f},
     NativeTransitionAnchor::VerticalAxis,
     NativeMotionGrammar::Selective, 0.18f, 0.22f,
     transientRoles | GrooveRole | HarmonyRole | StructureRole, 6.0f},
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

inline constexpr NativeTransitionStyle nativeTransitionStyle(
    NativeSceneKind source, NativeSceneKind incoming) {
    if (source == NativeSceneKind::NegativeSpace
        || incoming == NativeSceneKind::NegativeSpace) {
        return NativeTransitionStyle::NegativeSpaceReveal;
    }
    if (source == NativeSceneKind::ShadowArchitecture
        || incoming == NativeSceneKind::ShadowArchitecture) {
        return NativeTransitionStyle::DepthTravel;
    }
    if (source == NativeSceneKind::GlassChoir
        || incoming == NativeSceneKind::GlassChoir) {
        return NativeTransitionStyle::ControlledFracture;
    }
    if (source == NativeSceneKind::InkCurrent
        || incoming == NativeSceneKind::InkCurrent) {
        return NativeTransitionStyle::FlowCarry;
    }
    const NativeSceneDefinition& sourceDefinition
        = nativeSceneRegistry[static_cast<std::size_t>(source)];
    const NativeSceneDefinition& incomingDefinition
        = nativeSceneRegistry[static_cast<std::size_t>(incoming)];
    if (sourceDefinition.motionGrammar == NativeMotionGrammar::Flow
        || incomingDefinition.motionGrammar == NativeMotionGrammar::Flow) {
        return NativeTransitionStyle::FlowCarry;
    }
    if (sourceDefinition.transitionAnchor == NativeTransitionAnchor::DepthPoint
        || incomingDefinition.transitionAnchor == NativeTransitionAnchor::DepthPoint) {
        return NativeTransitionStyle::DepthTravel;
    }
    return NativeTransitionStyle::FocalMorph;
}

inline constexpr int nativeTransitionMode(NativeSceneKind source,
                                          NativeSceneKind incoming) {
    return static_cast<int>(nativeTransitionStyle(source, incoming));
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
