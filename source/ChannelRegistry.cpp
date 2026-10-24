#include "ChannelRegistry.h"

#include "PluginProcessor.h"
#include "dsp/ConsoleChannel.h"

std::recursive_mutex& ChannelRegistry::lock()
{
    static std::recursive_mutex mutex;
    return mutex;
}

std::vector<ParagonPR7CAudioProcessor*>& ChannelRegistry::members()
{
    static std::vector<ParagonPR7CAudioProcessor*> list;
    return list;
}

void ChannelRegistry::add (ParagonPR7CAudioProcessor& processor)
{
    const std::scoped_lock scoped (lock());
    members().push_back (&processor);
}

void ChannelRegistry::remove (ParagonPR7CAudioProcessor& processor)
{
    const std::scoped_lock scoped (lock());
    auto& list = members();
    list.erase (std::remove (list.begin(), list.end(), &processor), list.end());
}

void ChannelRegistry::take (Taken& taken, int channel, bool analog) noexcept
{
    channel = juce::jlimit (1, effects::ConsoleChannel::count, channel);
    taken[(size_t) channel] = true;
    if (analog)
        taken[(size_t) effects::ConsoleChannel::nextTo (channel)] = true;
}

int ChannelRegistry::pickUnused (const Taken& taken, bool analog, int current, juce::Random& random)
{
    std::vector<int> free, other;
    for (int channel = 1; channel <= effects::ConsoleChannel::count; ++channel)
    {
        if (channel == current)
            continue;
        other.push_back (channel);
        if (! taken[(size_t) channel] && (! analog || ! taken[(size_t) effects::ConsoleChannel::nextTo (channel)]))
            free.push_back (channel);
    }
    const auto& from = free.empty() ? other : free;
    return from[(size_t) random.nextInt ((int) from.size())];
}

void ChannelRegistry::rerollOne (ParagonPR7CAudioProcessor& processor)
{
    const std::scoped_lock scoped (lock());
    Taken taken {};
    for (auto* member : members())
        if (member != &processor)
            take (taken, member->getConsoleChannel(), member->isAnalog());
    processor.setConsoleChannel (pickUnused (taken, processor.isAnalog(), processor.getConsoleChannel(), juce::Random::getSystemRandom()));
}

void ChannelRegistry::rerollAll()
{
    // One at a time, each avoiding the channels given out before it.
    const std::scoped_lock scoped (lock());
    Taken taken {};
    for (auto* member : members())
    {
        const auto channel = pickUnused (taken, member->isAnalog(), member->getConsoleChannel(), juce::Random::getSystemRandom());
        member->setConsoleChannel (channel);
        take (taken, channel, member->isAnalog());
    }
}
