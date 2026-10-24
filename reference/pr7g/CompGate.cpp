#include "CompGate.h"

namespace effects
{
    namespace
    {
        constexpr float sidechainCutHz = 100.0f;   // the detector ignores the bass below this
        constexpr float makeUpReferenceDb = -12.0f;   // Daybreak's make-up restores part of what it takes at this level

        struct Curve
        {
            float thresholdDb, ratio, kneeDb;
        };

        // COMP turns the threshold down (from -8 to -40 dB) and the ratio up
        // together. INTENSITY off: a gentle ratio (1.5 to 6) and a wide, soft
        // knee; on: a hard ratio (4 to 20), a narrow knee, and a threshold
        // 6 dB lower, so it grabs earlier as well as harder.
        Curve daybreakCurve (float amount, bool intensity) noexcept
        {
            amount = juce::jlimit (0.0f, 10.0f, amount);
            return { -8.0f - 3.2f * amount - (intensity ? 6.0f : 0.0f),
                     intensity ? 4.0f + 1.6f * amount : 1.5f + 0.45f * amount,
                     intensity ? 3.0f : 10.0f };
        }

        // The gain change (0 or less, in dB) for an input level, with a soft knee.
        float gainFor (float inputDb, const Curve& curve) noexcept
        {
            const auto over = inputDb - curve.thresholdDb;
            const auto slope = 1.0f / curve.ratio - 1.0f;
            if (2.0f * over < -curve.kneeDb)
                return 0.0f;
            if (2.0f * over <= curve.kneeDb)
                return slope * juce::square (over + curve.kneeDb * 0.5f) / (2.0f * curve.kneeDb);
            return slope * over;
        }

        float coefficientFor (double sampleRate, float seconds) noexcept
        {
            return 1.0f - std::exp (-1.0f / (seconds * (float) sampleRate));
        }
    }

    float CompGate::daybreakCurveDb (float inputDb, float amount, bool intensity) noexcept
    {
        return gainFor (inputDb, daybreakCurve (amount, intensity));
    }

    void CompGate::prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = newSampleRate;
        const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, 1 };
        gate.prepare (spec);
        compressor.prepare (spec);

        // Fixed ballistics that suit guitar: the gate opens instantly and
        // closes gently so notes don't get chopped; the compressor lets the
        // pick attack through before it clamps down.
        gate.setRatio (10.0f);
        gate.setAttack (1.0f);
        gate.setRelease (120.0f);
        compressor.setAttack (8.0f);
        compressor.setRelease (150.0f);

        makeUpGain.reset (sampleRate, 0.02);
        mix.reset (sampleRate, 0.05);
        daybreakMakeUp.reset (sampleRate, 0.05);
        envelope = sidechainState = 0.0f;
        bypass.prepare (sampleRate, maxBlockSize);
    }

    void CompGate::process (juce::dsp::AudioBlock<float> monoBlock, const Settings& settings) noexcept
    {
        if (! settings.on)
            gainReductionDb.store (0.0f);

        bypass.process (monoBlock, settings.on, [&] (juce::dsp::AudioBlock<float> block)
        {
            juce::dsp::ProcessContextReplacing<float> context (block);

            if (settings.gateThresholdDb > gateOffDb)
            {
                gate.setThreshold (settings.gateThresholdDb);
                gate.process (context);
            }

            if (settings.type == Type::daybreak)
            {
                processDaybreak (block, settings);
            }
            else
            {
                // The meter: how much quieter the compressor made this block.
                const auto* samples = block.getChannelPointer (0);
                const auto numSamples = (int) block.getNumSamples();
                double inPower = 0.0, outPower = 0.0;
                for (int i = 0; i < numSamples; ++i)
                    inPower += (double) samples[i] * samples[i];

                compressor.setThreshold (settings.thresholdDb);
                compressor.setRatio (settings.ratio);
                compressor.process (context);

                for (int i = 0; i < numSamples; ++i)
                    outPower += (double) samples[i] * samples[i];
                gainReductionDb.store (inPower > 1.0e-9 ? (float) juce::jmin (0.0, 10.0 * std::log10 ((outPower + 1.0e-12) / inPower)) : 0.0f);
            }

            makeUpGain.setTargetValue (juce::Decibels::decibelsToGain (settings.levelDb));
            makeUpGain.applyGain (block.getChannelPointer (0), (int) block.getNumSamples());
        });
    }

    void CompGate::processDaybreak (juce::dsp::AudioBlock<float> block, const Settings& settings) noexcept
    {
        const auto curve = daybreakCurve (settings.amount, settings.intensity);

        // DETAIL is the attack: at 0 it clamps in half a millisecond and
        // flattens the pick; at 10 it waits 30 ms, so the pick snaps through.
        // INTENSITY also lets go faster, so it grabs every note.
        const auto attackSeconds = 0.0005f * std::pow (60.0f, juce::jlimit (0.0f, 10.0f, settings.detail) / 10.0f);
        const auto attack = coefficientFor (sampleRate, attackSeconds);
        const auto release = coefficientFor (sampleRate, settings.intensity ? 0.09f : 0.25f);
        const auto sidechainCut = coefficientFor (sampleRate, 1.0f / (juce::MathConstants<float>::twoPi * sidechainCutHz));

        // Automatic make-up: give back most of what it takes from a firmly
        // played note, so turning COMP up squeezes rather than just quietens.
        daybreakMakeUp.setTargetValue (juce::Decibels::decibelsToGain (-0.6f * gainFor (makeUpReferenceDb, curve)));
        mix.setTargetValue (juce::jlimit (0.0f, 1.0f, settings.mix));

        auto* samples = block.getChannelPointer (0);
        auto mostReduction = 0.0f;
        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            const auto dry = samples[i];

            // The detector hears the guitar without its lowest bass.
            sidechainState += (dry - sidechainState) * sidechainCut;
            const auto detected = std::abs (dry - sidechainState);
            envelope += (detected - envelope) * (detected > envelope ? attack : release);

            const auto reductionDb = gainFor (juce::Decibels::gainToDecibels (envelope, -120.0f), curve);
            mostReduction = juce::jmin (mostReduction, reductionDb);

            const auto compressed = dry * juce::Decibels::decibelsToGain (reductionDb) * daybreakMakeUp.getNextValue();
            const auto mixNow = mix.getNextValue();
            samples[i] = dry * (1.0f - mixNow) + compressed * mixNow;
        }
        gainReductionDb.store (mostReduction);
    }
}
