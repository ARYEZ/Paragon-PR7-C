#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The unit's finish: Obsidian (a pearl black unit, the default) or Aurora
// (pearl white). On both the keys and knobs are black (the fader's cap
// chrome), in neon rings (pink to red to orange): the keys' edges and the
// knobs' rings of light. Only the unit itself changes (its module plates and
// the lettering printed on them); the screens, the meters and the dark panels
// round it stay as they are. (0.4.7 and before also had Pearl White, with
// dark key edges and orange knob rings: retired. The family's decision, from
// PR7-G 0.60.0 and PR7-B 0.24.0.)
//
// It's a preference of the person, not part of a project or a rig: kept in a
// small settings file in the user's application data folder, so every
// PR7-C opened afterwards, in any project, uses the same finish. Every open
// window repaints when it changes.
namespace Finish
{
    // Saved by number: only ever add to the end. retiredPearlWhite (0) is
    // only ever read from an old settings file, and opens as Obsidian; the
    // old Pearl Black (1) is Obsidian, the old Pearl White + Neon (2) Aurora.
    enum class Type { retiredPearlWhite, obsidian, aurora };
    // The finishes offered, in the order they're shown, and their names.
    inline constexpr Type choices[] { Type::obsidian, Type::aurora };
    inline juce::String nameOf (Type type) { return type == Type::aurora ? "AURORA" : "OBSIDIAN"; }

    Type get();
    // `remember` saves it to the settings file (off for tests and snapshots).
    void set (Type, bool remember = true);
    inline bool isBlack() { return get() != Type::aurora; }              // the chassis colour

    // Told whenever the finish changes, to repaint.
    juce::ChangeBroadcaster& changes();

    // Colours that follow the finish.
    juce::Colour ink();          // lettering printed on the unit: near black on white, pearl white on black
    juce::Colour inkDim();
    juce::Colour stampEdge();    // the fine edge under stamped lettering: a light catch on white, a shadow on black
    juce::Colour printedRed();   // the red lettering and lines (a touch brighter on black, so it reads as clearly)

    // The satin grain laid over pearl surfaces: a fine speckle with no
    // direction to it (so no brushed lines) and the odd bright mica fleck.
    // On black the flecks show more, as they do in a real black pearl paint.
    juce::Image makeSatinGrain (int width, int height, int seed);

    // The neon finishes' rings: hot pink at the left through red to orange
    // at the right (the reference swatch's colours, 25% darker), across
    // `left` to `right`. Lighting is drawn over it.
    juce::ColourGradient neonGradient (float left, float right);

    // AURORA's pearl is darkened by this much (the owner: full white was "too
    // bright, almost disorienting", in PR7-B; the family's choice since): every
    // pixel of the pearl surfaces, their sheen and highlights included, is
    // drawn at 90% of its old value, so pure white becomes 230/255. The
    // lettering, the neon, the keys, knobs, meters and screens on it keep
    // their own colours.
    inline constexpr float auroraDimming = 0.10f;

    // Darkens a pearl surface just drawn, inside `area`, by auroraDimming on
    // Aurora (black laid over it: each colour times 1 - auroraDimming); nothing on Obsidian.
    void dimPearl (juce::Graphics&, const juce::Path& area);
    // A colour of Aurora's pearl as drawn (times 1 - auroraDimming); unchanged on Obsidian.
    juce::Colour pearl (juce::Colour);

    // Picks the colour for the current finish.
    inline juce::Colour pick (juce::uint32 onWhite, juce::uint32 onBlack) { return juce::Colour (isBlack() ? onBlack : onWhite); }
}
