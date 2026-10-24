#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "LevelMeterSource.h"

// One channel's peak meter in the meter window: a column of fine LED
// segments, +24 dB at the top and -60 dB at the bottom, where 0 is digital
// full scale (0 dBFS, as on the DAW's own meters). Lit segments are warm
// white below -18 dB, amber up to 0, and red above it (overs). The highest
// peak stays lit as a single segment until resetPeakHold().
class LevelMeter final : public juce::Component,
                         private juce::Timer
{
public:
    explicit LevelMeter (LevelMeterSource& source);

    void paint (juce::Graphics&) override;
    void resetPeakHold();

    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 24.0f;

    // The printed scale, top to bottom. The marks are evenly spaced, as on
    // the concept, so the 12 dB steps at the bottom are squeezed into the
    // same space as the 6 dB steps above them.
    static constexpr float scaleMarksDb[] = { 24.0f, 18.0f, 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -30.0f, -36.0f, -48.0f, -60.0f };
    // How far up the meter a level sits (0 at the bottom, 1 at the top), and back.
    static float proportionForDb (float db) noexcept;
    static float dbForProportion (float proportion) noexcept;

    float getDisplayedDb() const noexcept { return displayedDb; }
    float getPeakHoldDb() const noexcept { return heldDb; }

private:
    void timerCallback() override;

    LevelMeterSource& source;
    float displayedDb = minDb, heldDb = minDb;
};
