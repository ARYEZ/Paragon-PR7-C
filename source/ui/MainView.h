#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "ui/LevelMeter.h"

// Everything in the plugin window, on a fixed 1448 x 1086 canvas: the size
// of the concept image (docs/concept.png), so positions here can be read
// straight off it. The editor scales the whole view to the window.
//
// Every module's panel is drawn with all its controls, placed as on the
// concept. Most are inert until their module's DSP is built (see CLAUDE.md
// for the order): each step attaches its controls to parameters, found by
// their IDs here. Working now: HPF / LPF, the EQ, INPUT, Ø, MUTE, OUTPUT
// TRIM, the fader, the meters, and the finish (from the settings gear).
class MainView final : public juce::Component,
                       private juce::ChangeListener,
                       private juce::Timer
{
public:
    explicit MainView (ParagonPR7CAudioProcessor&);
    ~MainView() override;

    static constexpr int width = 1448;
    static constexpr int height = 1086;

    void paint (juce::Graphics&) override;

    // A module's place on the canvas, read off the concept, and its name.
    struct Module
    {
        const char* name;
        juce::Rectangle<float> bounds;
    };
    static const std::vector<Module>& modules();

    // Every control, by ID: "<module>.<control>" ("comp.ratio", "hf.on").
    // Knobs are Knob, lamps Led, switches LeverSwitch, the rest TextButton.
    juce::Component* findControl (const juce::String& id) const;
    const std::map<juce::String, juce::Component*>& getControls() const noexcept { return controls; }

    // The meter window's four level meters (IN L, IN R, OUT L, OUT R).
    std::array<LevelMeter*, 4> getLevelMeters() noexcept { return { &inputLeft, &inputRight, &outputLeft, &outputRight }; }
    juce::Button& getPeakHoldReset() noexcept { return peakReset; }
    // OVL: lights once any meter has gone past digital full scale, until reset.
    Led& getOverloadLamp() noexcept { return overload; }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;   // the finish changed
    void timerCallback() override;                                      // follows the meters for OVL
    void resetPeakHolds();

    Knob& addKnob (const juce::String& id, juce::Point<float> centre, float ringRadius, std::vector<juce::String> scale,
                   juce::String unit, juce::String caption, double initial, const juce::String& tooltip);
    juce::TextButton& addButton (const juce::String& id, const juce::String& text, juce::Rectangle<int> bounds,
                                 bool toggles, const juce::String& tooltip);
    Led& addLed (const juce::String& id, juce::Point<float> centre, const juce::String& tooltip);
    LeverSwitch& addSwitch (const juce::String& id, juce::Rectangle<int> bounds, int positions, int initial, bool glow,
                            const juce::String& tooltip);
    void addRadioGroup (std::initializer_list<const char*> ids, int group, const char* initiallyOn);

    // Bring a control to life: attach it to its parameter and give it its
    // real tooltip. A knob's scale is printed at each mark's true place
    // (given as values), and it shows its value while turned.
    void attachKnob (const juce::String& controlId, const juce::String& parameterId,
                     std::vector<std::pair<float, juce::String>> marks, const juce::String& tooltip);
    void attachButton (const juce::String& controlId, const juce::String& parameterId, const juce::String& tooltip);
    void attachSwitch (const juce::String& controlId, const juce::String& parameterId, const juce::String& tooltip);
    // A row of keys that pick one choice (VCA / OPTO / FET), in the choice's order.
    void attachChoice (std::initializer_list<const char*> controlIds, const juce::String& parameterId);

    void showSettingsMenu();

    ParagonPR7CAudioProcessor& processor;
    juce::OwnedArray<juce::Component> ownedControls;
    std::map<juce::String, juce::Component*> controls;

    LevelMeter inputLeft, inputRight, outputLeft, outputRight;
    RoundButton peakReset;
    Led overload;
    juce::OwnedArray<GainReductionMeter> reductionMeters;
    juce::Slider fader { juce::Slider::LinearVertical, juce::Slider::NoTextBox };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> inputAttachment, trimAttachment, faderAttachment;
    std::unique_ptr<ButtonAttachment> phaseAttachment, muteAttachment;
    std::vector<std::unique_ptr<SliderAttachment>> knobAttachments;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> switchAttachments;

    // The satin grain over the panels, made for the current finish.
    juce::Image grain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainView)
};
