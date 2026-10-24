#pragma once

#include <array>
#include <mutex>
#include <vector>

#include <juce_core/juce_core.h>

class ParagonPR7CAudioProcessor;

// Every PR7-C in this session (this process), so RANDOM CH. can give each
// one console channels no other is using, as tracks on a real console each
// have their own. Instances add themselves when created and leave when
// deleted. Used on the message thread only, never the audio thread.
//
// A host that runs each plugin in its own separate process can't share this
// list: there, ONE and ALL can only avoid the channels of the PR7-Cs in the
// same process.
class ChannelRegistry
{
public:
    static void add (ParagonPR7CAudioProcessor&);
    static void remove (ParagonPR7CAudioProcessor&);

    // ONE: a new random channel for this PR7-C, unused by the others.
    static void rerollOne (ParagonPR7CAudioProcessor&);
    // ALL: new random channels for every PR7-C, each unused by the rest.
    static void rerollAll();

    // Which channels are taken (index 1 to 72), and a random free one for a
    // PR7-C that uses its channel and the next (ANALOG) or only its own
    // (DIGITAL). Never `current`, so a re-roll always changes something. If
    // nothing is free (more than 36 PR7-Cs in ANALOG), any other channel.
    using Taken = std::array<bool, 73>;
    static void take (Taken&, int channel, bool analog) noexcept;
    static int pickUnused (const Taken&, bool analog, int current, juce::Random&);

private:
    static std::recursive_mutex& lock();
    static std::vector<ParagonPR7CAudioProcessor*>& members();
};
