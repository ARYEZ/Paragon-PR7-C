#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "LevelMeterSource.h"

// Vertical LED-style meter with a dB scale, matching the input/output meters
// in the mockup (+12 dB at the top, -60 dB at the bottom).
class LevelMeter final : public juce::Component,
                         private juce::Timer
{
public:
    explicit LevelMeter (LevelMeterSource& source);

    void paint (juce::Graphics&) override;

    static constexpr float minDb = -60.0f;
    static constexpr float maxDb = 12.0f;

private:
    void timerCallback() override;
    float dbToProportion (float db) const noexcept;

    LevelMeterSource& source;
    float displayedDb = minDb;
};
