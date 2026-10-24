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

        // A frequency knob: even travel per octave.
        juce::NormalisableRange<float> frequencyRange (float minHz, float maxHz)
        {
            return { minHz, maxHz,
                     [] (float start, float end, float proportion) { return start * std::pow (end / start, proportion); },
                     [] (float start, float end, float hz) { return std::log (hz / start) / std::log (end / start); },
                     [] (float start, float end, float hz) { return juce::jlimit (start, end, hz); } };
        }

        juce::String asHz (float value, int) { return frequencyText (value); }
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
            juce::NormalisableRange<float> (ParamRanges::outputTrimMin, ParamRanges::outputTrimMax, 0.1f), 0.0f,
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
                juce::NormalisableRange<float> (-eqGainRange, eqGainRange, 0.1f), 0.0f,
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
        return layout;
    }
}
