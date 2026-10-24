#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "ui/LevelMeter.h"

// Everything in the plugin window, on a fixed 1448 x 1086 canvas: the size
// of the concept image (docs/concept.png), so positions here can be read
// straight off it. The editor scales the whole view to the window.
//
// The seed: the header and footer, the five columns with each module's
// place outlined and named, the meter window with working input and output
// meters, and OUTPUT LEVEL. Each module replaces its outline as it's built
// (see CLAUDE.md for the order).
class MainView final : public juce::Component
{
public:
    explicit MainView (ParagonPR7CAudioProcessor&);

    static constexpr int width = 1448;
    static constexpr int height = 1086;

    void paint (juce::Graphics&) override;
    void resized() override;

    // A module's place on the canvas, read off the concept, and its name.
    struct Module
    {
        const char* name;
        juce::Rectangle<float> bounds;
    };
    static const std::vector<Module>& modules();

private:
    LevelMeter inputLeft, inputRight, outputLeft, outputRight;
    juce::Slider outputLevel { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
    juce::AudioProcessorValueTreeState::SliderAttachment outputLevelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
