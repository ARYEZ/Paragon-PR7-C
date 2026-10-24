#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter's ID, in one place. IDs are saved in projects and presets:
// never rename one once it has shipped. Choice parameters are saved by
// index, so only ever add choices to the end.
namespace ParamIDs
{
    inline constexpr auto outputLevel = "outputLevel";   // the output FADER, dB (0.1.0's OUTPUT LEVEL knob)
    inline constexpr auto inputGain = "inputGain";       // INPUT, dB: the top knob of the OUTPUT column
    inline constexpr auto outputTrim = "outputTrim";     // OUTPUT TRIM, dB
    inline constexpr auto phaseInvert = "phaseInvert";   // Ø: flips the polarity of both channels
    inline constexpr auto mute = "mute";                 // MUTE
}

namespace ParamRanges
{
    inline constexpr float inputGainMin = -24.0f, inputGainMax = 12.0f;
    inline constexpr float outputTrimMin = -12.0f, outputTrimMax = 12.0f;
    inline constexpr float outputLevelMin = -60.0f, outputLevelMax = 10.0f;   // the very bottom is silence (-inf)

    // The output fader's printed scale, top to bottom. The marks are evenly
    // spaced on the fader (as on the concept), so the travel between two
    // marks is the same however many dB lie between them: fine control round
    // 0 dB, coarse towards the bottom.
    inline constexpr float faderMarksDb[] = { 10.0f, 5.0f, 0.0f, -5.0f, -10.0f, -20.0f, -30.0f, -40.0f, -60.0f };
}

namespace Parameters
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // The fader's travel (0 at the bottom, 1 at the top) for a level in dB,
    // and back: straight lines between the printed marks.
    float faderProportion (float db);
    float faderDb (float proportion);
}
