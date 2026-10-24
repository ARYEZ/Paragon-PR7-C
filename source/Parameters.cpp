#include "Parameters.h"

namespace Parameters
{
    namespace
    {
        // Each new parameter gets the next version hint, so hosts can tell
        // which plugin version added it.
        constexpr int firstVersion = 1;
        constexpr int panelVersion = 2;   // the OUTPUT column of the full panel layout

        juce::String asDb (float value, int)
        {
            return (value > 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
        }

        juce::String onOff (bool on, int) { return on ? "On" : "Off"; }

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

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        // OUTPUT LEVEL was a -24 to +12 dB knob in 0.1.0. It's now the output
        // fader; the ID stays, so projects keep their level.
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::outputLevel, firstVersion }, "Output Level", faderRange(), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
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
        return layout;
    }
}
