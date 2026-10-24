#include "PluginProcessor.h"

#include "PluginEditor.h"

ParagonPR7CAudioProcessor::ParagonPR7CAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", Parameters::createLayout()),
      outputLevel (parameters.getRawParameterValue (ParamIDs::outputLevel)),
      inputGainDb (parameters.getRawParameterValue (ParamIDs::inputGain)),
      outputTrim (parameters.getRawParameterValue (ParamIDs::outputTrim)),
      phaseInvert (parameters.getRawParameterValue (ParamIDs::phaseInvert)),
      mute (parameters.getRawParameterValue (ParamIDs::mute))
{
    filterParameters = { parameters.getRawParameterValue (ParamIDs::hpfOn), parameters.getRawParameterValue (ParamIDs::hpfFreq),
                         parameters.getRawParameterValue (ParamIDs::lpfOn), parameters.getRawParameterValue (ParamIDs::lpfFreq),
                         parameters.getRawParameterValue (ParamIDs::filterPlacement) };
    for (size_t i = 0; i < bandParameters.size(); ++i)
    {
        const auto* id = ParamRanges::eqBands[i].id;
        bandParameters[i] = { parameters.getRawParameterValue (ParamIDs::eqOn (id)), parameters.getRawParameterValue (ParamIDs::eqFreq (id)),
                              parameters.getRawParameterValue (ParamIDs::eqGain (id)), parameters.getRawParameterValue (ParamIDs::eqQ (id)),
                              parameters.getRawParameterValue (ParamIDs::eqShelf (id)) };   // the one the band doesn't have is null
    }
    const auto raw = [this] (const char* id) { return parameters.getRawParameterValue (id); };
    compParameters = { raw (ParamIDs::compOn), raw (ParamIDs::compThreshold), raw (ParamIDs::compRatio), raw (ParamIDs::compAttack),
                       raw (ParamIDs::compRelease), raw (ParamIDs::compAuto), raw (ParamIDs::compMakeUp), raw (ParamIDs::compCharacter),
                       raw (ParamIDs::dynamicsLink) };
    gateParameters = { raw (ParamIDs::gateOn), raw (ParamIDs::gateThreshold), raw (ParamIDs::gateMinus30), raw (ParamIDs::gateRange),
                       raw (ParamIDs::gateExpander), raw (ParamIDs::gateRatio), raw (ParamIDs::gateAttack), raw (ParamIDs::gateHold),
                       raw (ParamIDs::gateRelease), raw (ParamIDs::gateLinear) };
    scParameters = { raw (ParamIDs::scOn), raw (ParamIDs::scHpf), raw (ParamIDs::scLpf), raw (ParamIDs::scListen), raw (ParamIDs::scHighQ) };
    satParameters = { raw (ParamIDs::satOn), raw (ParamIDs::satDrive), raw (ParamIDs::satHarmonics), raw (ParamIDs::satVcaClip),
                      raw (ParamIDs::satMix) };
    toneParameters = { raw (ParamIDs::toneOn), raw (ParamIDs::toneThd), raw (ParamIDs::toneVGain), raw (ParamIDs::toneChannel) };
    limitParameters = { raw (ParamIDs::limitOn), raw (ParamIDs::limitThreshold), raw (ParamIDs::limitMakeUp), raw (ParamIDs::limitMode),
                        raw (ParamIDs::limitMix) };
}

ParagonPR7CAudioProcessor::EqBands ParagonPR7CAudioProcessor::makeEqBands()
{
    using Shelf = effects::EqBand::Shelf;
    return { effects::EqBand (Shelf::high), effects::EqBand (Shelf::none), effects::EqBand (Shelf::none),
             effects::EqBand (Shelf::low), effects::EqBand (Shelf::low) };
}

bool ParagonPR7CAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo in and out, or mono in and out: the same on both sides.
    const auto output = layouts.getMainOutputChannelSet();
    if (output != juce::AudioChannelSet::mono() && output != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == output;
}

void ParagonPR7CAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlockSize = juce::jmax (1, samplesPerBlock);
    sidechain.setSize (2, maxBlockSize);
    compressor.prepare (sampleRate, maxBlockSize);
    gate.prepare (sampleRate, maxBlockSize);
    scFilter.prepare (sampleRate);
    listen.reset (sampleRate, 0.02);
    listen.setCurrentAndTargetValue (scParameters.listen->load() >= 0.5f ? 1.0f : 0.0f);
    limiter.prepare (sampleRate, maxBlockSize);
    saturation.prepare (sampleRate, maxBlockSize);
    colour.prepare (sampleRate, maxBlockSize);
    filtersSidechain.prepare (sampleRate);
    for (auto* gain : { &inputGain, &outputGain })
        gain->reset (sampleRate, 0.02);
    inputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (inputGainDb->load()));
    for (int side = 0; side < 2; ++side)
    {
        for (auto* filters : { &filtersIn, &filtersPost })
            (*filters)[(size_t) side].prepare (sampleRate);
        for (auto& band : eqBands[(size_t) side])
            band.prepare (sampleRate);
    }
    outputGain.setCurrentAndTargetValue (targetOutputGain());
}

effects::CutFilters::Settings ParagonPR7CAudioProcessor::filterSettings (int placement, const effects::ConsoleChannel& channel) const noexcept
{
    // The filters only touch the audio when the switch puts them here.
    const auto here = juce::roundToInt (filterParameters.placement->load()) == placement;
    return { here && filterParameters.hpfOn->load() >= 0.5f, here && filterParameters.lpfOn->load() >= 0.5f,
             filterParameters.hpfFrequency->load() * channel.highPass, filterParameters.lpfFrequency->load() * channel.lowPass };
}

void ParagonPR7CAudioProcessor::updateSides() noexcept
{
    // Off, both sides are the design itself (channel 1). On, the left side
    // is the chosen channel and the right the next one.
    const auto on = toneParameters.on->load() >= 0.5f;
    const auto left = on ? juce::roundToInt (toneParameters.channel->load()) : 1;
    const std::array<int, 2> wanted { left, on ? effects::ConsoleChannel::nextTo (left) : 1 };
    for (size_t side = 0; side < 2; ++side)
    {
        if (wanted[side] != sideNumbers[side])
        {
            sides[side] = effects::ConsoleChannel::number (wanted[side]);
            sideNumbers[side] = wanted[side];
        }
    }
}

void ParagonPR7CAudioProcessor::processFilters (std::array<effects::CutFilters, 2>& filters, juce::AudioBuffer<float>& buffer,
                                                int numSamples, int placement) noexcept
{
    for (int side = 0; side < juce::jmin (2, buffer.getNumChannels()); ++side)
    {
        juce::AudioBuffer<float> one (buffer.getArrayOfWritePointers() + side, 1, numSamples);
        filters[(size_t) side].process (one, numSamples, filterSettings (placement, sides[(size_t) side]));
    }
}

void ParagonPR7CAudioProcessor::processEq (juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    for (int side = 0; side < juce::jmin (2, buffer.getNumChannels()); ++side)
    {
        juce::AudioBuffer<float> one (buffer.getArrayOfWritePointers() + side, 1, numSamples);
        const auto& channel = sides[(size_t) side];
        for (size_t i = 0; i < eqBands[(size_t) side].size(); ++i)
        {
            const auto& p = bandParameters[i];
            eqBands[(size_t) side][i].process (one, numSamples, { p.on->load() >= 0.5f, p.frequency->load() * channel.eqFrequency[i],
                                                                  p.gain->load() * channel.eqGain[i], p.q != nullptr ? p.q->load() : 1.0f,
                                                                  p.shelf != nullptr && p.shelf->load() >= 0.5f });
        }
    }
}

float ParagonPR7CAudioProcessor::targetOutputGain() const noexcept
{
    // MUTE, or the fader right at the bottom: silence.
    if (mute->load() >= 0.5f || outputLevel->load() <= ParamRanges::outputLevelMin)
        return 0.0f;
    const auto gain = juce::Decibels::decibelsToGain (outputTrim->load() + outputLevel->load());
    return phaseInvert->load() >= 0.5f ? -gain : gain;
}

namespace
{
    // Applies a gain that may be gliding, to every channel.
    void applyGain (juce::AudioBuffer<float>& buffer, juce::SmoothedValue<float>& gain, int numSamples) noexcept
    {
        if (! gain.isSmoothing())
        {
            if (! juce::approximatelyEqual (gain.getTargetValue(), 1.0f))
                buffer.applyGain (0, numSamples, gain.getTargetValue());
            return;
        }
        for (int i = 0; i < numSamples; ++i)
        {
            const auto value = gain.getNextValue();
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
                buffer.getWritePointer (channel)[i] *= value;
        }
    }
}

void ParagonPR7CAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, numSamples);

    // INPUT first, so the IN meters show what the strip works on.
    inputGain.setTargetValue (juce::Decibels::decibelsToGain (inputGainDb->load()));
    applyGain (buffer, inputGain, numSamples);
    pushMeters (inputMeters, buffer, numSamples);

    // The modules, in stretches no longer than the block size we were
    // prepared for (their working buffers are that size). Unprepared, they're
    // skipped.
    for (int start = 0; maxBlockSize > 0 && start < numSamples; start += maxBlockSize)
    {
        const auto length = juce::jmin (maxBlockSize, numSamples - start);
        juce::AudioBuffer<float> stretch (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, length);
        processModules (stretch, length);
    }

    outputGain.setTargetValue (targetOutputGain());
    applyGain (buffer, outputGain, numSamples);
    pushMeters (outputMeters, buffer, numSamples);
}

void ParagonPR7CAudioProcessor::processModules (juce::AudioBuffer<float>& buffer, int numSamples) noexcept
{
    // In their fixed order (see CLAUDE.md).
    constexpr int filtersAtInput = 0, filtersAfterDynamics = 1, filtersInSidechain = 2;   // the IN / POST / SC switch's positions
    updateSides();
    processFilters (filtersIn, buffer, numSamples, filtersAtInput);

    // What the gate and compressor listen to: the audio here, through the
    // HPF / LPF when they're set to SC, and the SC FILTER.
    for (int channel = 0; channel < 2; ++channel)
        sidechain.copyFrom (channel, 0, buffer, juce::jmin (channel, buffer.getNumChannels() - 1), 0, numSamples);
    juce::AudioBuffer<float> detector (sidechain.getArrayOfWritePointers(), 2, 0, numSamples);
    filtersSidechain.process (detector, numSamples, filterSettings (filtersInSidechain));
    const auto& sc = scParameters;
    const auto scOn = sc.on->load() >= 0.5f;
    scFilter.process (detector, numSamples, { scOn, scOn, sc.hpf->load(), sc.lpf->load(),
                                              sc.highQ->load() >= 0.5f ? ParamRanges::scHighQ : effects::CutFilters::butterworthQ });

    const auto& g = gateParameters;
    gate.process (buffer, detector, numSamples,
                  { g.on->load() >= 0.5f, g.threshold->load() - (g.minus30->load() >= 0.5f ? 30.0f : 0.0f), g.range->load(),
                    g.expander->load() >= 0.5f, g.ratio->load(),
                    (effects::Gate::Attack) juce::jlimit (0, 2, juce::roundToInt (g.attack->load())),
                    g.hold->load(), g.release->load(), g.linear->load() >= 0.5f, compParameters.link->load() >= 0.5f });
    for (int channel = 0; channel < 2; ++channel)
        getGainReduction (Dynamics::gate, channel).push (gate.getReductionDb (channel));

    const auto& c = compParameters;
    compressor.process (buffer, detector, numSamples,
                        { c.on->load() >= 0.5f, c.threshold->load(), c.ratio->load(), c.attack->load(), c.release->load(),
                          c.autoRelease->load() >= 0.5f, c.makeUp->load(),
                          (effects::Compressor::Character) juce::jlimit (0, 2, juce::roundToInt (c.character->load())),
                          c.link->load() >= 0.5f });
    for (int channel = 0; channel < 2; ++channel)
        getGainReduction (Dynamics::comp, channel).push (compressor.getReductionDb (channel));
    compGainChange.push (juce::jmax (compressor.getReductionDb (0), compressor.getReductionDb (1)));
    compMakeUpShown.store (c.on->load() >= 0.5f ? c.makeUp->load() : 0.0f);

    processEq (buffer, numSamples);
    processFilters (filtersPost, buffer, numSamples, filtersAfterDynamics);

    const auto& l = limitParameters;
    limiter.process (buffer, numSamples,
                     { l.on->load() >= 0.5f, l.threshold->load(), l.makeUp->load(),
                       (effects::Limiter::Mode) juce::jlimit (0, 2, juce::roundToInt (l.mode->load())),
                       l.mix->load() / 100.0f, compParameters.link->load() >= 0.5f });
    for (int channel = 0; channel < 2; ++channel)
        getGainReduction (Dynamics::limit, channel).push (limiter.getReductionDb (channel));

    const auto& s = satParameters;
    saturation.process (buffer, numSamples, { s.on->load() >= 0.5f, s.drive->load(), s.harmonics->load() / 100.0f,
                                              s.vcaClip->load() >= 0.5f, s.mix->load() / 100.0f });

    // The console colour: THD and the noise floor, each side with its channel's amounts.
    const auto& t = toneParameters;
    const auto colourOn = t.on->load() >= 0.5f;
    effects::ConsoleColour::Settings colourSettings;
    colourSettings.thd = colourOn ? t.thd->load() / 100.0f : 0.0f;
    colourSettings.noise = colourOn ? t.vGain->load() / 100.0f : 0.0f;
    for (size_t side = 0; side < 2; ++side)
    {
        colourSettings.second[side] = sides[side].second;
        colourSettings.third[side] = sides[side].third;
        colourSettings.hissDb[side] = sides[side].hissDb;
        colourSettings.humDb[side] = sides[side].humDb;
    }
    colour.process (buffer, numSamples, colourSettings);

    // The stereo tools will go here.

    // LISTEN: hear what the gate and compressor hear instead (a 20 ms crossfade).
    listen.setTargetValue (sc.listen->load() >= 0.5f ? 1.0f : 0.0f);
    if (listen.isSmoothing() || listen.getCurrentValue() > 0.0f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto amount = listen.getNextValue();
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                auto* samples = buffer.getWritePointer (channel);
                samples[i] += (detector.getSample (juce::jmin (channel, 1), i) - samples[i]) * amount;
            }
        }
    }
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
