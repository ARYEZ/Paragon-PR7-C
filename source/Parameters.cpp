#include "Parameters.h"

namespace Parameters
{
    namespace
    {
        // Each new parameter gets the next version hint, so hosts can tell
        // which plugin version added it.
        constexpr int firstVersion = 1;
        constexpr int panelVersion = 2;   // the OUTPUT column of the full panel layout
        constexpr int eqVersion = 3;      // the EQ and the HPF / LPF
        constexpr int compVersion = 4;    // the compressor and LINK
        constexpr int limitVersion = 5;   // the limiter
        constexpr int gateVersion = 6;    // the gate / expander and the SC FILTER
        constexpr int satVersion = 7;     // the saturation
        constexpr int toneVersion = 8;    // the console colour (THD, V GAIN, the channels)
        constexpr int stereoVersion = 9;  // MONO MAKER, STEREO WIDTH and the STEREO MODE

        // RATIO: the marks evenly round the knob, log-even between them. The
        // bottom stretch (below 1.25) is the SOFT position.
        // A knob whose printed marks sit evenly round it: between two marks
        // the travel is even in log (or straight, from a mark of 0). `snap`
        // tidies a value (the comp's SOFT stretch).
        juce::NormalisableRange<float> markedRange (std::vector<float> marks, std::function<float (float)> snap = {})
        {
            const auto steps = (float) (marks.size() - 1);
            auto between = [] (float low, float high, float t) { return low > 0.0f ? low * std::pow (high / low, t) : low + (high - low) * t; };
            auto within = [] (float low, float high, float value) { return low > 0.0f ? std::log (value / low) / std::log (high / low) : (value - low) / (high - low); };
            return { marks.front(), marks.back(),
                     [marks, steps, between] (float, float, float proportion)
                     {
                         const auto position = juce::jlimit (0.0f, 1.0f, proportion) * steps;
                         const auto below = juce::jmin ((int) position, (int) steps - 1);
                         return between (marks[(size_t) below], marks[(size_t) below + 1], position - (float) below);
                     },
                     [marks, steps, within] (float, float, float value)
                     {
                         for (size_t i = 1; i < marks.size(); ++i)
                             if (value <= marks[i])
                                 return ((float) (i - 1) + within (marks[i - 1], marks[i], juce::jmax (value, marks[i - 1]))) / steps;
                         return 1.0f;
                     },
                     [marks, snap] (float, float, float value)
                     {
                         value = juce::jlimit (marks.front(), marks.back(), value);
                         return snap ? snap (value) : value;
                     } };
        }

        // RATIO: SOFT, then 1.5:1 to infinite. Below 1.25 is the SOFT position.
        juce::NormalisableRange<float> ratioRange()
        {
            using ParamRanges::compRatioMarks;
            return markedRange ({ std::begin (compRatioMarks), std::end (compRatioMarks) },
                                [] (float ratio) { return ratio < 1.25f ? 1.0f : ratio; });
        }

        juce::String ratioText (float ratio, int)
        {
            if (ratio < 1.25f)
                return "Soft";
            if (ratio >= 29.5f)
                return "Inf:1";
            return juce::String (ratio, ratio < 10.0f ? 1 : 0) + ":1";
        }

        juce::String asMs (float ms, int)
        {
            return ms < 1000.0f ? juce::String (ms, ms < 10.0f ? 1 : 0) + " ms" : juce::String (ms / 1000.0f, 2) + " s";
        }

        // A frequency knob: even travel per octave.
        juce::NormalisableRange<float> frequencyRange (float minHz, float maxHz)
        {
            return { minHz, maxHz,
                     [] (float start, float end, float proportion) { return start * std::pow (end / start, proportion); },
                     [] (float start, float end, float hz) { return std::log (hz / start) / std::log (end / start); },
                     [] (float start, float end, float hz) { return juce::jlimit (start, end, hz); } };
        }

        juce::String asHz (float value, int) { return frequencyText (value); }

        void frequencyOf (juce::AudioProcessorValueTreeState::ParameterLayout& layout, const char* id, const juce::String& name,
                          float minHz, float maxHz, float defaultHz, int version)
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, version }, name, frequencyRange (minHz, maxHz), defaultHz,
                                                                     juce::AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (asHz)));
        }
        juce::String asQ (float value, int) { return juce::String (value, 2); }

        juce::String asDb (float value, int)
        {
            return (value > 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
        }

        juce::String onOff (bool on, int) { return on ? "On" : "Off"; }

        // The fader: at the very bottom it's off.
        juce::String faderDbText (float value, int length)
        {
            return value <= ParamRanges::outputLevelMin ? juce::String ("-inf dB") : asDb (value, length);
        }

        // INPUT: 0 dB sits at the top of the knob (half way), so there's as
        // much travel for cutting -24 dB as for adding +12.
        // A straight range in steps (0.1 dB, 1%...), snapped to whole steps
        // counted from 0. JUCE's own interval counts them from the minimum
        // (minimum + step x n), which a fused multiply-add (as on Apple
        // silicon) rounds a hair off: an EQ gain's 0 dB came out as
        // 0.0000002 dB, enough to keep every band running at its default.
        juce::NormalisableRange<float> steppedRange (float minimum, float maximum, float step)
        {
            const auto perUnit = std::round (1.0f / step);   // 10 for 0.1, 1 for 1
            return { minimum, maximum,
                     [] (float start, float end, float proportion) { return start + (end - start) * proportion; },
                     [] (float start, float end, float value) { return (value - start) / (end - start); },
                     [perUnit] (float start, float end, float value) { return juce::jlimit (start, end, std::round (value * perUnit) / perUnit); } };
        }

        juce::NormalisableRange<float> inputRange()
        {
            using namespace ParamRanges;
            return { inputGainMin, inputGainMax,
                     [] (float, float, float proportion)
                     {
                         return proportion < 0.5f ? inputGainMin * (1.0f - proportion * 2.0f)
                                                  : inputGainMax * (proportion * 2.0f - 1.0f);
                     },
                     [] (float, float, float db)
                     {
                         return db < 0.0f ? 0.5f * (1.0f - db / inputGainMin)
                                          : 0.5f + 0.5f * db / inputGainMax;
                     },
                     [] (float, float, float db) { return juce::jlimit (inputGainMin, inputGainMax, std::round (db * 10.0f) / 10.0f); } };
        }

        juce::NormalisableRange<float> faderRange()
        {
            using namespace ParamRanges;
            return { outputLevelMin, outputLevelMax,
                     [] (float, float, float proportion) { return faderDb (proportion); },
                     [] (float, float, float db) { return faderProportion (db); },
                     [] (float, float, float db) { return juce::jlimit (outputLevelMin, outputLevelMax, std::round (db * 10.0f) / 10.0f); } };
        }
    }

    float faderProportion (float db)
    {
        using ParamRanges::faderMarksDb;
        constexpr auto numMarks = (int) std::size (faderMarksDb);
        db = juce::jlimit (faderMarksDb[numMarks - 1], faderMarksDb[0], db);
        for (int i = numMarks - 1; i > 0; --i)
        {
            // Marks run top (index 0) to bottom; the bottom one is proportion 0.
            const auto low = faderMarksDb[i], high = faderMarksDb[i - 1];
            if (db <= high)
            {
                const auto step = 1.0f / (float) (numMarks - 1);
                return ((float) (numMarks - 1 - i) + (db - low) / (high - low)) * step;
            }
        }
        return 1.0f;
    }

    float faderDb (float proportion)
    {
        using ParamRanges::faderMarksDb;
        constexpr auto numMarks = (int) std::size (faderMarksDb);
        const auto position = juce::jlimit (0.0f, 1.0f, proportion) * (float) (numMarks - 1);
        const auto below = juce::jmin ((int) position, numMarks - 2);   // the mark at or below, counted from the bottom
        const auto low = faderMarksDb[numMarks - 1 - below], high = faderMarksDb[numMarks - 2 - below];
        return low + (high - low) * (position - (float) below);
    }

    juce::String frequencyText (float hz)
    {
        if (hz < 1000.0f)
            return juce::String (juce::roundToInt (hz)) + " Hz";
        return juce::String (hz / 1000.0f, hz < 10000.0f ? 2 : 1) + " kHz";
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        // OUTPUT LEVEL was a -24 to +12 dB knob in 0.1.0. It's now the output
        // fader; the ID stays, so projects keep their level.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::outputLevel, firstVersion }, "Output Level", faderRange(), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (faderDbText)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::inputGain, panelVersion }, "Input", inputRange(), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::outputTrim, panelVersion }, "Output Trim",
            steppedRange (ParamRanges::outputTrimMin, ParamRanges::outputTrimMax, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamIDs::phaseInvert, panelVersion }, "Phase Invert", false,
            juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { ParamIDs::mute, panelVersion }, "Mute", false,
            juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));

        // HPF / LPF.
        const auto onSwitch = [&] (const juce::String& id, const juce::String& name, bool on)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, eqVersion }, name, on,
                                                                    juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        };
        const auto frequency = [&] (const juce::String& id, const juce::String& name, float minHz, float maxHz, float defaultHz)
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, eqVersion }, name, frequencyRange (minHz, maxHz), defaultHz,
                                                                     juce::AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (asHz)));
        };
        using namespace ParamRanges;
        onSwitch (ParamIDs::hpfOn, "HPF", false);
        frequency (ParamIDs::hpfFreq, "HPF Frequency", hpfMin, hpfMax, hpfDefault);
        onSwitch (ParamIDs::lpfOn, "LPF", false);
        frequency (ParamIDs::lpfFreq, "LPF Frequency", lpfMin, lpfMax, lpfDefault);
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::filterPlacement, eqVersion }, "Filter Placement",
                                                                  filterPlacements, 0));

        // The EQ bands.
        for (const auto& band : eqBands)
        {
            const juce::String name (band.name);
            onSwitch (ParamIDs::eqOn (band.id), name, true);
            frequency (ParamIDs::eqFreq (band.id), name + " Frequency", band.minHz, band.maxHz, band.defaultHz);
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { ParamIDs::eqGain (band.id), eqVersion }, name + " Gain",
                steppedRange (-eqGainRange, eqGainRange, 0.1f), 0.0f,
                juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
            if (band.hasQ)
                layout.add (std::make_unique<juce::AudioParameterFloat> (
                    juce::ParameterID { ParamIDs::eqQ (band.id), eqVersion }, name + " Q", frequencyRange (eqQMin, eqQMax), 1.0f,
                    juce::AudioParameterFloatAttributes().withStringFromValueFunction (asQ)));
            else
                layout.add (std::make_unique<juce::AudioParameterBool> (
                    juce::ParameterID { ParamIDs::eqShelf (band.id), eqVersion }, name + " Shelf", band.shelfByDefault,
                    juce::AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool shelf, int) { return shelf ? "Shelf" : "Bell"; })));
        }

        // COMP, and LINK.
        const auto compBool = [&] (const char* id, const juce::String& name, bool on)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, compVersion }, name, on,
                                                                    juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        };
        const auto logRange = [] (float minimum, float maximum) { return frequencyRange (minimum, maximum); };   // even travel per doubling
        compBool (ParamIDs::compOn, "Compressor", false);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::compThreshold, compVersion }, "Comp Threshold",
            steppedRange (compThresholdMin, compThresholdMax, 0.1f), -18.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::compRatio, compVersion }, "Comp Ratio", ratioRange(), 4.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (ratioText)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::compAttack, compVersion }, "Comp Attack", logRange (compAttackMin, compAttackMax), 10.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (asMs)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::compRelease, compVersion }, "Comp Release", logRange (compReleaseMin, compReleaseMax), 300.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (asMs)));
        compBool (ParamIDs::compAuto, "Comp Auto Release", false);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::compMakeUp, compVersion }, "Comp Make Up",
            steppedRange (0.0f, compMakeUpMax, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::compCharacter, compVersion }, "Comp Character",
                                                                  compCharacters, 0));
        compBool (ParamIDs::dynamicsLink, "Link", true);

        // LIMIT.
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::limitOn, limitVersion }, "Limiter", false,
                                                                juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::limitThreshold, limitVersion }, "Limit Threshold",
            steppedRange (limitThresholdMin, 0.0f, 0.1f), -1.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::limitMakeUp, limitVersion }, "Limit Make Up",
            steppedRange (0.0f, limitMakeUpMax, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::limitMode, limitVersion }, "Limit Mode",
                                                                  limitModes, 0));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::limitMix, limitVersion }, "Limit Mix",
            steppedRange (0.0f, 100.0f, 1.0f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; })));
        // GATE / EXP.
        using namespace ParamRanges;
        const auto gateBool = [&] (const char* id, const juce::String& name, bool on)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, gateVersion }, name, on,
                                                                    juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        };
        gateBool (ParamIDs::gateOn, "Gate", false);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateThreshold, gateVersion }, "Gate Threshold",
            steppedRange (gateThresholdMin, 0.0f, 0.1f), -40.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        gateBool (ParamIDs::gateMinus30, "Gate -30", false);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateRange, gateVersion }, "Gate Range",
            steppedRange (0.0f, gateRangeMax, 0.1f), 40.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));
        gateBool (ParamIDs::gateExpander, "Gate Expander", false);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateRatio, gateVersion }, "Gate Ratio",
            markedRange ({ std::begin (gateRatioMarks), std::end (gateRatioMarks) }), 2.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + ":1"; })));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::gateAttack, gateVersion }, "Gate Attack",
                                                                  gateAttacks, 1));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateHold, gateVersion }, "Gate Hold",
            markedRange ({ std::begin (gateHoldMarks), std::end (gateHoldMarks) }), 50.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (asMs)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::gateRelease, gateVersion }, "Gate Release", frequencyRange (gateReleaseMin, gateReleaseMax), 200.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction (asMs)));
        gateBool (ParamIDs::gateLinear, "Gate Linear Release", false);

        // SC FILTER.
        gateBool (ParamIDs::scOn, "SC Filter", false);
        frequencyOf (layout, ParamIDs::scHpf, "SC HPF", scHpfMin, scHpfMax, 60.0f, gateVersion);
        frequencyOf (layout, ParamIDs::scLpf, "SC LPF", scLpfMin, scLpfMax, 20000.0f, gateVersion);
        gateBool (ParamIDs::scListen, "SC Listen", false);
        gateBool (ParamIDs::scHighQ, "SC High Q", false);

        // SATURATION.
        const auto percent = [] (float v, int) { return juce::String (juce::roundToInt (v)) + "%"; };
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::satOn, satVersion }, "Saturation", false,
                                                                juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::satDrive, satVersion }, "Sat Drive", steppedRange (0.0f, satDriveMax, 0.1f), 6.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::satHarmonics, satVersion }, "Sat Harmonics", steppedRange (0.0f, 100.0f, 1.0f), 30.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
            {
                const auto odd = juce::roundToInt (v);
                return odd == 0 ? juce::String ("2nd") : odd == 100 ? juce::String ("3rd") : juce::String (100 - odd) + "% 2nd / " + juce::String (odd) + "% 3rd";
            })));
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::satVcaClip, satVersion }, "Sat VCA Clip", false,
                                                                juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::satMix, satVersion }, "Sat Mix", steppedRange (0.0f, 100.0f, 1.0f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percent)));

        // FILTERS / TONE: the console colour.
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::toneOn, toneVersion }, "Console Colour", true,
                                                                juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        const auto offOrPercent = [] (float v, int) { return juce::roundToInt (v) == 0 ? juce::String ("OFF") : juce::String (juce::roundToInt (v)) + "%"; };
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::toneThd, toneVersion }, "THD", steppedRange (0.0f, 100.0f, 1.0f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (offOrPercent)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::toneVGain, toneVersion }, "V Gain", steppedRange (0.0f, 100.0f, 1.0f), 0.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (offOrPercent)));
        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { ParamIDs::toneChannel, toneVersion }, "Console Channel", 1, 72, 1,
            juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int left, int) { return "Channel " + juce::String (left); })));

        // MONO MAKER / STEREO WIDTH, and the STEREO MODE.
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::monoOn, stereoVersion }, "Mono Maker", false,
                                                                juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        frequencyOf (layout, ParamIDs::monoFreq, "Mono Maker Freq", monoFreqMin, monoFreqMax, monoFreqDefault, stereoVersion);
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParamIDs::widthOn, stereoVersion }, "Stereo Width", false,
                                                                juce::AudioParameterBoolAttributes().withStringFromValueFunction (onOff)));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::width, stereoVersion }, "Width", steppedRange (0.0f, widthMax, 1.0f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (percent)));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ParamIDs::stereoMode, stereoVersion }, "Stereo Mode",
                                                                  stereoModes, 0));
        return layout;
    }
}
