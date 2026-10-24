#pragma once

#include "EffectUtils.h"

namespace effects
{
    // Footswitch 1: noise gate then compressor, before the amp.
    // The gate comes first so the compressor doesn't bring the noise back up.
    // Two compressors:
    //
    //   classic   threshold and ratio, as studio compressors have
    //   daybreak  a modern pedal compressor, laid out like the boutique
    //             ones metal players use: COMP (how much, with automatic
    //             make-up gain), DETAIL (how much pick attack gets through
    //             before it clamps), MIX (parallel compression: the
    //             squashed sound blended with the dry guitar) and INTENSITY
    //             (off: smooth and gentle; on: fast and hard). Its detector
    //             ignores the lowest bass, so low palm mutes don't make it pump.
    class CompGate
    {
    public:
        // Saved by number in rigs and projects: only ever add to the end.
        enum class Type { classic, daybreak };
        static inline const juce::StringArray typeNames { "Classic", "Daybreak" };

        struct Settings
        {
            bool on = false;
            float thresholdDb = -18.0f;  // classic
            float ratio = 4.0f;          // classic
            float levelDb = 0.0f;        // make-up gain (both)
            float gateThresholdDb = gateOffDb;
            Type type = Type::classic;
            float amount = 5.0f;         // daybreak: COMP, 0 to 10
            float detail = 5.0f;         // daybreak: DETAIL, 0 to 10
            float mix = 1.0f;            // daybreak: MIX, 0 to 1
            bool intensity = false;      // daybreak: INTENSITY
        };

        // Gate threshold at (or below) this means the gate is off.
        static constexpr float gateOffDb = -90.0f;

        void prepare (double sampleRate, int maxBlockSize);
        void process (juce::dsp::AudioBlock<float> monoBlock, const Settings& settings) noexcept;

        // How far the compressor turned the guitar down in the last block, in
        // dB (0 or less), for the panel's meter. 0 while it's off.
        float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

        // Daybreak's gain at a steady input level, in dB, before its make-up
        // gain: exposed for tests, and the shape of its compression curve.
        static float daybreakCurveDb (float inputDb, float amount, bool intensity) noexcept;

    private:
        void processDaybreak (juce::dsp::AudioBlock<float> block, const Settings& settings) noexcept;

        double sampleRate = 48000.0;
        juce::dsp::NoiseGate<float> gate;
        juce::dsp::Compressor<float> compressor;
        juce::SmoothedValue<float> makeUpGain;
        BypassFade bypass;
        std::atomic<float> gainReductionDb { 0.0f };

        // Daybreak
        float envelope = 0.0f;               // the detector's level
        float sidechainState = 0.0f;         // its bass cut
        juce::SmoothedValue<float> mix, daybreakMakeUp;
    };
}
