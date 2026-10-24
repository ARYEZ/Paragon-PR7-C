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

    // LIMIT.
    inline constexpr auto limitOn = "limitOn";
    inline constexpr auto limitThreshold = "limitThreshold";   // dBFS
    inline constexpr auto limitMakeUp = "limitMakeUp";         // dB
    inline constexpr auto limitMode = "limitMode";             // choice: Brick, Soft, Auto
    inline constexpr auto limitMix = "limitMix";               // %

    // GATE / EXP.
    inline constexpr auto gateOn = "gateOn";
    inline constexpr auto gateThreshold = "gateThreshold";   // dBFS, before -30
    inline constexpr auto gateMinus30 = "gateMinus30";       // shifts the threshold down 30 dB
    inline constexpr auto gateRange = "gateRange";           // dB
    inline constexpr auto gateExpander = "gateExpander";     // EXP
    inline constexpr auto gateRatio = "gateRatio";           // the expander's ratio
    inline constexpr auto gateAttack = "gateAttack";         // choice: Fast, Med, Slow
    inline constexpr auto gateHold = "gateHold";             // ms
    inline constexpr auto gateRelease = "gateRelease";       // ms
    inline constexpr auto gateLinear = "gateLinear";         // LINEAR release

    // SC FILTER: filters in what the gate and compressor listen to.
    inline constexpr auto scOn = "scOn";
    inline constexpr auto scHpf = "scHpf";
    inline constexpr auto scLpf = "scLpf";
    inline constexpr auto scListen = "scListen";             // hear the sidechain
    inline constexpr auto scHighQ = "scHighQ";

    // SATURATION.
    inline constexpr auto satOn = "satOn";
    inline constexpr auto satDrive = "satDrive";             // dB
    inline constexpr auto satHarmonics = "satHarmonics";     // %: 0 = 2nd, 100 = 3rd
    inline constexpr auto satVcaClip = "satVcaClip";
    inline constexpr auto satMix = "satMix";                 // %

    // FILTERS / TONE: the console colour.
    inline constexpr auto toneOn = "toneOn";                 // the module's lamp: THD, noise and the channels' differences
    inline constexpr auto toneThd = "toneThd";               // %
    inline constexpr auto toneVGain = "toneVGain";           // %: the noise floor
    inline constexpr auto toneChannel = "toneChannel";       // 1 to 72: the left side's console channel (the right uses the next)

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

    inline constexpr float limitThresholdMin = -20.0f, limitMakeUpMax = 12.0f;
    inline const juce::StringArray limitModes { "Brick", "Soft", "Auto" };   // saved by index: only append

    inline constexpr float gateThresholdMin = -60.0f, gateRangeMax = 80.0f;
    inline constexpr float gateRatioMarks[] = { 1.0f, 1.5f, 3.0f, 6.0f, 20.0f };
    inline constexpr float gateHoldMarks[] = { 0.0f, 50.0f, 500.0f };              // ms
    inline constexpr float gateReleaseMin = 5.0f, gateReleaseMax = 3000.0f;        // ms
    inline const juce::StringArray gateAttacks { "Fast", "Med", "Slow" };        // saved by index: only append
    inline constexpr float scHpfMin = 20.0f, scHpfMax = 200.0f, scLpfMin = 1000.0f, scLpfMax = 20000.0f;
    inline constexpr float scHighQ = 2.5f;   // the SC FILTER's resonance with HIGH Q

    inline constexpr float satDriveMax = 24.0f;

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
