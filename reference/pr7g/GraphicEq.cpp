#include "GraphicEq.h"

namespace effects
{
    namespace
    {
        constexpr int retuneEvery = 32;   // samples between filter updates while a fader glides

        std::array<float, 6> peak (double sampleRate, float frequency, float gainDb)
        {
            return juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (sampleRate, frequency, GraphicEq::bandQ,
                                                                            juce::Decibels::decibelsToGain (gainDb));
        }
    }

    float GraphicEq::bandFrequency (int band, double sampleRate) noexcept
    {
        // At low sample rates the 16 kHz band would sit past the highest
        // frequency the audio can hold, so pull it down just below that.
        return juce::jmin (frequencies[(size_t) band], (float) sampleRate * 0.45f);
    }

    void GraphicEq::prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;
        for (int band = 0; band < numBands; ++band)
        {
            auto& filter = filters[(size_t) band];
            filter.coefficients = new juce::dsp::IIR::Coefficients<float>();
            *filter.coefficients = peak (sampleRate, bandFrequency (band, sampleRate), 0.0f);
            filter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, 1 });
            gains[(size_t) band].reset (sampleRate, 0.05);
            gains[(size_t) band].setCurrentAndTargetValue (0.0f);
            appliedDb[(size_t) band] = 0.0f;
        }
        level.reset (sampleRate, 0.05);
        level.setCurrentAndTargetValue (1.0f);
        bypass.prepare (sampleRate, maxBlockSize);
    }

    void GraphicEq::process (juce::dsp::AudioBlock<float> monoBlock, const Settings& settings) noexcept
    {
        bypass.process (monoBlock, settings.on, [&] (juce::dsp::AudioBlock<float> block)
        {
            for (int band = 0; band < numBands; ++band)
                gains[(size_t) band].setTargetValue (juce::jlimit (-maxGainDb, maxGainDb, settings.gainsDb[(size_t) band]));
            level.setTargetValue (juce::Decibels::decibelsToGain (settings.levelDb));

            const auto numSamples = (int) block.getNumSamples();
            for (int start = 0; start < numSamples; start += retuneEvery)
            {
                const auto count = juce::jmin (retuneEvery, numSamples - start);
                auto part = block.getSubBlock ((size_t) start, (size_t) count);

                // Retune any band whose gain moved, in place (no allocation).
                for (int band = 0; band < numBands; ++band)
                {
                    const auto gainDb = gains[(size_t) band].skip (count);
                    if (std::abs (gainDb - appliedDb[(size_t) band]) > 0.001f)
                    {
                        *filters[(size_t) band].coefficients = peak (sampleRate, bandFrequency (band, sampleRate), gainDb);
                        appliedDb[(size_t) band] = gainDb;
                    }
                    filters[(size_t) band].process (juce::dsp::ProcessContextReplacing<float> (part));
                }

                auto* samples = part.getChannelPointer (0);
                for (int i = 0; i < count; ++i)
                    samples[i] *= level.getNextValue();
            }
        });
    }

    float GraphicEq::responseDb (const std::array<float, numBands>& gainsDb, float frequency, double sampleRate)
    {
        // The bands are in series, so their responses (in dB) add up.
        float total = 0.0f;
        for (int band = 0; band < numBands; ++band)
        {
            if (std::abs (gainsDb[(size_t) band]) < 0.001f)
                continue;
            juce::dsp::IIR::Coefficients<float> coefficients;
            coefficients = peak (sampleRate, bandFrequency (band, sampleRate), gainsDb[(size_t) band]);
            total += juce::Decibels::gainToDecibels ((float) coefficients.getMagnitudeForFrequency (frequency, sampleRate), -100.0f);
        }
        return total;
    }
}
