#include "Parameters.h"

namespace Parameters
{
    namespace
    {
        // Each new parameter gets the next version hint, so hosts can tell
        // which plugin version added it.
        constexpr int firstVersion = 1;

        juce::String asDb (float value, int)
        {
            return (value > 0.0f ? "+" : "") + juce::String (value, 1) + " dB";
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { ParamIDs::outputLevel, firstVersion }, "Output Level",
            juce::NormalisableRange<float> (ParamRanges::outputLevelMin, ParamRanges::outputLevelMax, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (asDb)));
        return layout;
    }
}
