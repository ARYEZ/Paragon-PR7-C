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

    // HPF / LPF, and where they sit (IN / POST / SC).
    inline constexpr auto hpfOn = "hpfOn";
    inline constexpr auto hpfFreq = "hpfFreq";
    inline constexpr auto lpfOn = "lpfOn";
    inline constexpr auto lpfFreq = "lpfFreq";
    inline constexpr auto filterPlacement = "filterPlacement";   // choice: In, Post, Sidechain

    // The EQ: "<band>On", "<band>Freq", "<band>Gain", and "<band>Q" (HMF,
    // MF) or "<band>Shelf" (HF, LMF, LF). Bands: hf, hmf, mf, lmf, lf.
    inline const juce::String eqOn (const juce::String& band) { return band + "On"; }
    inline const juce::String eqFreq (const juce::String& band) { return band + "Freq"; }
    inline const juce::String eqGain (const juce::String& band) { return band + "Gain"; }
    inline const juce::String eqQ (const juce::String& band) { return band + "Q"; }
    inline const juce::String eqShelf (const juce::String& band) { return band + "Shelf"; }

    // COMP.
    inline constexpr auto compOn = "compOn";
    inline constexpr auto compThreshold = "compThreshold";   // dBFS
    inline constexpr auto compRatio = "compRatio";           // 1 = SOFT, 30 = infinite
    inline constexpr auto compAttack = "compAttack";         // ms
    inline constexpr auto compRelease = "compRelease";       // ms
    inline constexpr auto compAuto = "compAuto";
    inline constexpr auto compMakeUp = "compMakeUp";         // dB
    inline constexpr auto compCharacter = "compCharacter";   // choice: VCA, OPTO, FET

    // LINK, on the LIMIT panel: links the gate, compressor and limiter.
    inline constexpr auto dynamicsLink = "dynamicsLink";
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

    inline constexpr float hpfMin = 20.0f, hpfMax = 2000.0f, hpfDefault = 80.0f;
    inline constexpr float lpfMin = 1000.0f, lpfMax = 20000.0f, lpfDefault = 18000.0f;
    inline const juce::StringArray filterPlacements { "In", "Post", "Sidechain" };   // saved by index: only append

    inline constexpr float eqGainRange = 15.0f;   // ±15 dB
    inline constexpr float eqQMin = 0.5f, eqQMax = 4.0f;

    // The five bands, top to bottom of the panel.
    struct EqBandSpec
    {
        const char* id;
        const char* name;
        float minHz, maxHz, defaultHz;
        bool hasQ;              // HMF, MF: a Q knob; the others switch shelf / bell
        bool shelfByDefault;
    };
    inline constexpr float compThresholdMin = -50.0f, compThresholdMax = 0.0f;
    inline constexpr float compAttackMin = 0.1f, compAttackMax = 30.0f;          // ms
    inline constexpr float compReleaseMin = 50.0f, compReleaseMax = 3000.0f;     // ms
    inline constexpr float compMakeUpMax = 20.0f;
    inline const juce::StringArray compCharacters { "VCA", "OPTO", "FET" };    // saved by index: only append

    // RATIO's printed marks, evenly spaced round the knob: SOFT (1), then
    // 1.5:1 to infinite (30). The travel between marks is even in log ratio.
    inline constexpr float compRatioMarks[] = { 1.0f, 1.5f, 2.0f, 4.0f, 10.0f, 30.0f };

    inline constexpr EqBandSpec eqBands[] {
        { "hf",  "HF",  2000.0f, 20000.0f, 10000.0f, false, true },
        { "hmf", "HMF", 400.0f,  5000.0f,  2500.0f,  true,  false },
        { "mf",  "MF",  150.0f,  2500.0f,  600.0f,   true,  false },
        { "lmf", "LMF", 40.0f,   600.0f,   250.0f,   false, false },
        { "lf",  "LF",  20.0f,   200.0f,   60.0f,    false, true },
    };
}

namespace Parameters
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // The fader's travel (0 at the bottom, 1 at the top) for a level in dB,
    // and back: straight lines between the printed marks.
    float faderProportion (float db);
    float faderDb (float proportion);

    // "120 Hz", "2.40 kHz".
    juce::String frequencyText (float hz);
}
