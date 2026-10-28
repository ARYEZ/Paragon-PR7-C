#pragma once

#include "PluginProcessor.h"

// The strip delays everything by `latencySamples` (reported to the host, so
// it lines the tracks up). The input sample that should come out at `i`:
// the one that delay earlier, or silence before the start.
inline float delayedInput (const juce::AudioBuffer<float>& input, int channel, int i)
{
    const auto from = i - ParagonPR7CAudioProcessor::latencySamples;
    return from >= 0 ? input.getSample (channel, from) : 0.0f;
}
