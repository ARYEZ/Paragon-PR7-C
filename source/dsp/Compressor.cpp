#include "dsp/Compressor.h"

namespace effects
{
    namespace
    {
        float coefficientFor (double sampleRate, float seconds) noexcept
        {
            return 1.0f - std::exp (-1.0f / (juce::jmax (1.0e-5f, seconds) * (float) sampleRate));
        }

        // An RMS reading is 3 dB below a sine's peak; add it back, so the
        // threshold means the same for every character (a sine peaking at
        // -10 dBFS reads -10 dB).
        constexpr float rmsToPeakDb = 3.0103f;
    }

    float Compressor::kneeDbFor (Character character) noexcept
    {
        switch (character)
        {
            case Character::opto: return 12.0f;
            case Character::fet:  return 3.0f;
            case Character::vca:  break;
        }
        return 1.0f;
    }

    float Compressor::gainComputerDb (float levelDb, float thresholdDb, float ratio, float kneeDb) noexcept
    {
        const auto over = levelDb - thresholdDb;

        // SOFT: the output rises ever more slowly over the threshold, so the
        // ratio grows with level (over / out = 1 + over / 4).
        if (ratio < 1.25f)
            return over <= 0.0f ? 0.0f : 4.0f * over / (4.0f + over) - over;

        const auto slope = ratio >= infiniteRatio ? 1.0f : 1.0f - 1.0f / ratio;
        if (2.0f * over < -kneeDb)
            return 0.0f;
        if (2.0f * over <= kneeDb)
            return -slope * juce::square (over + kneeDb * 0.5f) / (2.0f * kneeDb);
        return -slope * over;
    }

    void Compressor::prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;
        bypass.prepare (sampleRate, maxBlockSize);
        makeUp.reset (sampleRate, 0.02);
        fetGrit.reset (sampleRate, 0.02);
        reset();
        fresh = true;
    }

    void Compressor::reset()
    {
        meanSquare = {};
        peak = {};
        reduction = {};
        memory = {};
        for (auto& r : reductionDb)
            r.store (0.0f);
    }

    void Compressor::process (juce::AudioBuffer<float>& audio, const juce::AudioBuffer<float>& sidechain, int numSamples,
                              const Settings& settings) noexcept
    {
        const auto numChannels = juce::jmin (2, audio.getNumChannels());
        if (fresh)
        {
            // Playback is starting: on or off from the first sample.
            fresh = false;
            bypass.setImmediately (settings.on);
            makeUp.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (settings.makeUpDb));
            fetGrit.setCurrentAndTargetValue (settings.character == Character::fet ? 1.0f : 0.0f);
        }
        if (! settings.on && bypass.isSilent())
        {
            // Off: forget the state, so it starts fresh when switched on.
            reset();
            return;
        }

        const auto character = settings.character;
        const auto knee = kneeDbFor (character);
        const auto rmsTime = character == Character::opto ? 0.02f : 0.005f;
        const auto attackSeconds = settings.attackMs * 0.001f * (character == Character::opto ? 2.0f : 1.0f);
        const auto rmsCoefficient = coefficientFor (sampleRate, rmsTime);
        const auto attack = coefficientFor (sampleRate, attackSeconds);
        const auto peakRelease = coefficientFor (sampleRate, 0.01f);
        const auto memoryCoefficient = coefficientFor (sampleRate, 1.5f);
        const auto stretches = settings.autoRelease || character == Character::opto;
        makeUp.setTargetValue (juce::Decibels::decibelsToGain (settings.makeUpDb));
        fetGrit.setTargetValue (character == Character::fet ? 1.0f : 0.0f);

        std::array<float, 2> most {};
        auto block = juce::dsp::AudioBlock<float> (audio).getSubBlock (0, (size_t) numSamples);
        bypass.process (block, settings.on, [&] (juce::dsp::AudioBlock<float> wet)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                // What the detector hears, per channel, in dB. Both detectors
                // always run, so switching character starts from a live reading.
                std::array<float, 2> levelDb {};
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    const auto x = sidechain.getSample (juce::jmin (channel, sidechain.getNumChannels() - 1), i);
                    const auto c = (size_t) channel;
                    const auto level = std::abs (x);
                    peak[c] = level > peak[c] ? level : peak[c] + (level - peak[c]) * peakRelease;
                    meanSquare[c] += (x * x - meanSquare[c]) * rmsCoefficient;
                    levelDb[c] = character == Character::fet ? juce::Decibels::gainToDecibels (peak[c], -120.0f)
                                                             : 10.0f * std::log10 (meanSquare[c] + 1.0e-12f) + rmsToPeakDb;
                }
                if (settings.link && numChannels == 2)
                    levelDb[0] = levelDb[1] = juce::jmax (levelDb[0], levelDb[1]);

                const auto makeUpGain = makeUp.getNextValue();
                const auto gritAmount = fetGrit.getNextValue();
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    const auto c = (size_t) channel;
                    const auto target = -gainComputerDb (levelDb[c], settings.thresholdDb, settings.ratio, knee);   // reduction, 0 or more

                    // The release stretches with how long and how hard it has
                    // been compressing: 0.35x after a short peak, up to 3x.
                    memory[c] += (juce::jmin (1.0f, reduction[c] / 10.0f) - memory[c]) * memoryCoefficient;
                    const auto releaseSeconds = settings.releaseMs * 0.001f * (stretches ? 0.35f + 2.65f * memory[c] : 1.0f);
                    const auto release = coefficientFor (sampleRate, releaseSeconds);
                    reduction[c] += (target - reduction[c]) * (target > reduction[c] ? attack : release);
                    most[c] = juce::jmax (most[c], reduction[c]);

                    const auto gain = juce::Decibels::decibelsToGain (-reduction[c]);
                    auto y = wet.getSample (channel, i) * gain;
                    if (gritAmount > 0.0f)
                    {
                        // FET grit: a little third harmonic, growing with how
                        // hard it works (about 1% at 20 dB of reduction), at
                        // any level: the waveform is measured against its own
                        // peak level, as a FET's distortion follows its gain
                        // reduction rather than the signal level.
                        const auto grit = gritAmount * 0.04f * juce::jmin (1.0f, reduction[c] / 20.0f);
                        const auto shape = juce::jlimit (-1.5f, 1.5f, y / (peak[c] * gain + 1.0e-6f));
                        y -= grit * y * shape * shape;
                    }
                    wet.setSample (channel, i, y * makeUpGain);
                }
            }
        });

        for (int channel = 0; channel < 2; ++channel)
            reductionDb[(size_t) channel].store (settings.on ? most[(size_t) juce::jmin (channel, numChannels - 1)] : 0.0f);
    }
}
