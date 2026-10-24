#include "PluginProcessor.h"

#include "PluginEditor.h"

ParagonPR7CAudioProcessor::ParagonPR7CAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", Parameters::createLayout()),
      outputLevel (parameters.getRawParameterValue (ParamIDs::outputLevel))
{
}

bool ParagonPR7CAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo in and out, or mono in and out: the same on both sides.
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == output;
}

void ParagonPR7CAudioProcessor::prepareToPlay (double sampleRate, int)
{
    outputGain.reset (sampleRate, 0.02);   // 20 ms, so moving OUTPUT LEVEL never clicks
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputLevel->load()));
}

void ParagonPR7CAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    pushMeters (inputMeters, buffer, numSamples);

    // The modules will go here, in their fixed order (see CLAUDE.md).

    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputLevel->load()));
    if (outputGain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto gain = outputGain.getNextValue();
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.getWritePointer (channel)[i] *= gain;
        }
    }
    else
    {
        buffer.applyGain (outputGain.getTargetValue());
    }

    pushMeters (outputMeters, buffer, numSamples);
}

void ParagonPR7CAudioProcessor::pushMeters (std::array<LevelMeterSource, 2>& meters, const juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    for (int channel = 0; channel < 2; ++channel)
        meters[(size_t) channel].pushSamples (buffer.getReadPointer (juce::jmin (channel, buffer.getNumChannels() - 1)), numSamples);
}

juce::AudioProcessorEditor* ParagonPR7CAudioProcessor::createEditor()
{
    return new ParagonPR7CAudioProcessorEditor (*this);
}

void ParagonPR7CAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (const auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void ParagonPR7CAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary (data, sizeInBytes); xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParagonPR7CAudioProcessor();
}
