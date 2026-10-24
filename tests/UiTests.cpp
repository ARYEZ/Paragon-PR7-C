#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"

using Catch::Matchers::WithinAbs;

namespace
{
    MainView& viewOf (juce::AudioProcessorEditor& editor)
    {
        auto* view = dynamic_cast<MainView*> (editor.getChildComponent (0));
        REQUIRE (view != nullptr);
        return *view;
    }

    // The part of a control that takes the mouse, on the canvas: for a knob
    // the knob itself, not its printed scale.
    juce::Rectangle<int> activeArea (juce::Component& control)
    {
        if (auto* knob = dynamic_cast<Knob*> (&control))
            return knob->slider.getBounds() + knob->getPosition();
        return control.getBounds();
    }

    // Lets the meters' timers run, as they would in a host.
    void runTimers (int milliseconds)
    {
        juce::Thread::sleep (milliseconds);
        juce::Timer::callPendingTimersSynchronously();
    }

    void setParameter (ParagonPR7CAudioProcessor& processor, const char* id, float value)
    {
        auto* parameter = processor.getParameters().getParameter (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    }
}

TEST_CASE ("The window keeps the concept's shape at every size", "[ui]")
{
    ParagonPR7CAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    for (const auto width : { 724, 1086, 1448 })
    {
        DYNAMIC_SECTION (width << " wide")
        {
            editor->setSize (width, width * MainView::height / MainView::width);
            auto* view = editor->getChildComponent (0);
            REQUIRE (view != nullptr);
            CHECK_THAT (view->getTransform().mat00, WithinAbs ((float) width / (float) MainView::width, 0.001));
        }
    }
}

TEST_CASE ("The window opens as big as fits above the taskbar, up to the concept's size", "[ui]")
{
    using Editor = ParagonPR7CAudioProcessorEditor;
    const auto fullHd = Editor::openingSize ({ 1920, 1040 });   // 1080p with a 40-pixel taskbar
    CHECK (fullHd.y + 80 <= 1040);
    CHECK (Editor::openingSize ({ 2560, 1400 }) == juce::Point<int> (MainView::width, MainView::height));
}

TEST_CASE ("Every module's place is inside the window and none overlap", "[ui]")
{
    const auto canvas = juce::Rectangle<float> ((float) MainView::width, (float) MainView::height);
    const auto& modules = MainView::modules();
    REQUIRE (modules.size() == 16);
    for (size_t i = 0; i < modules.size(); ++i)
    {
        CHECK (canvas.contains (modules[i].bounds));
        for (size_t j = i + 1; j < modules.size(); ++j)
            CHECK (! modules[i].bounds.intersects (modules[j].bounds));
    }
}

TEST_CASE ("Every control sits on a module (or in the header), and none overlap", "[ui]")
{
    ParagonPR7CAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto& view = viewOf (*editor);
    const auto& controls = view.getControls();
    REQUIRE (controls.size() > 80);

    const auto header = juce::Rectangle<int> (0, 0, MainView::width, 70);
    for (const auto& [id, control] : controls)
    {
        const auto area = activeArea (*control);
        const auto onAModule = std::any_of (MainView::modules().begin(), MainView::modules().end(),
                                            [&] (const auto& module) { return module.bounds.toNearestInt().contains (area); });
        INFO (id);
        CHECK ((onAModule || header.contains (area)));
    }

    for (auto first = controls.begin(); first != controls.end(); ++first)
    {
        for (auto second = std::next (first); second != controls.end(); ++second)
        {
            INFO (first->first << " and " << second->first);
            CHECK (! activeArea (*first->second).intersects (activeArea (*second->second)));
        }
    }
}

TEST_CASE ("Every module has its on lamp", "[ui]")
{
    ParagonPR7CAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto& view = viewOf (*editor);
    for (const auto* id : { "hpf.on", "lpf.on", "gate.on", "sc.on", "comp.on", "limit.on", "sat.on",
                            "hf.on", "hmf.on", "mf.on", "lmf.on", "lf.on", "tone.on", "mono.on", "width.on", "output.on" })
    {
        INFO (id);
        CHECK (dynamic_cast<Led*> (view.findControl (id)) != nullptr);
    }
}

TEST_CASE ("INPUT, Ø, MUTE, OUTPUT TRIM and the fader work the parameters", "[ui]")
{
    ParagonPR7CAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto& view = viewOf (*editor);
    auto& parameters = processor.getParameters();

    auto* input = dynamic_cast<Knob*> (view.findControl ("output.input"));
    auto* trim = dynamic_cast<Knob*> (view.findControl ("output.trim"));
    auto* fader = dynamic_cast<juce::Slider*> (view.findControl ("output.fader"));
    auto* phase = dynamic_cast<juce::Button*> (view.findControl ("output.phase"));
    auto* mute = dynamic_cast<juce::Button*> (view.findControl ("output.mute"));
    REQUIRE ((input != nullptr && trim != nullptr && fader != nullptr && phase != nullptr && mute != nullptr));

    input->slider.setValue (-6.0, juce::sendNotificationSync);
    CHECK_THAT (parameters.getRawParameterValue (ParamIDs::inputGain)->load(), WithinAbs (-6.0, 0.05));
    trim->slider.setValue (4.0, juce::sendNotificationSync);
    CHECK_THAT (parameters.getRawParameterValue (ParamIDs::outputTrim)->load(), WithinAbs (4.0, 0.05));
    fader->setValue (-20.0, juce::sendNotificationSync);
    CHECK_THAT (parameters.getRawParameterValue (ParamIDs::outputLevel)->load(), WithinAbs (-20.0, 0.05));
    phase->setToggleState (true, juce::sendNotificationSync);
    CHECK (parameters.getRawParameterValue (ParamIDs::phaseInvert)->load() > 0.5f);

    // And the other way: the host moves MUTE, the key lights.
    setParameter (processor, ParamIDs::mute, 1.0f);   // on the message thread, so the key follows at once
    CHECK (mute->getToggleState());
}

TEST_CASE ("The meters' printed scale is evenly spaced, +24 at the top to -60 at the bottom", "[ui]")
{
    const auto numMarks = (int) std::size (LevelMeter::scaleMarksDb);
    for (int i = 0; i < numMarks; ++i)
        CHECK_THAT (LevelMeter::proportionForDb (LevelMeter::scaleMarksDb[i]), WithinAbs (1.0 - (double) i / (numMarks - 1), 1.0e-6));
    for (float db = -60.0f; db <= 24.0f; db += 1.3f)
        CHECK_THAT (LevelMeter::dbForProportion (LevelMeter::proportionForDb (db)), WithinAbs (db, 1.0e-3));
}

TEST_CASE ("The meters hold the highest peak until it's reset", "[ui]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (48000.0, 256);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto& view = viewOf (*editor);
    auto* inputLeft = view.getLevelMeters()[0];

    // One loud block (-6 dBFS, +14 on the console scale), then quiet: the
    // meter falls, the peak stays.
    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;
    buffer.clear();
    buffer.setSample (0, 100, 0.5f);
    processor.processBlock (buffer, midi);
    for (int round = 0; round < 3; ++round)   // the first round only starts the timers' clock
        runTimers (40);
    CHECK_THAT (inputLeft->getPeakHoldDb(), WithinAbs (13.98, 0.05));

    buffer.clear();
    for (int round = 0; round < 10; ++round)
    {
        processor.processBlock (buffer, midi);
        runTimers (40);
    }
    CHECK (inputLeft->getDisplayedDb() < inputLeft->getPeakHoldDb() - 6.0f);   // fallen well below the held peak
    CHECK_THAT (inputLeft->getPeakHoldDb(), WithinAbs (13.98, 0.05));

    view.getPeakHoldReset().onClick();   // what a click does (triggerClick() would only queue it)
    runTimers (40);
    CHECK (inputLeft->getPeakHoldDb() <= LevelMeter::minDb);
}

TEST_CASE ("OVL lights when the signal goes past digital full scale, until it's cleared", "[ui]")
{
    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (48000.0, 256);
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto& view = viewOf (*editor);

    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;
    const auto play = [&] (float level)
    {
        buffer.clear();
        buffer.setSample (1, 10, level);
        for (int round = 0; round < 3; ++round)
        {
            processor.processBlock (buffer, midi);
            runTimers (70);
        }
    };

    play (0.99f);   // just under full scale: no
    CHECK (! view.getOverloadLamp().getToggleState());
    play (1.2f);    // over it
    CHECK (view.getOverloadLamp().getToggleState());
    play (0.0f);    // stays lit after the overload has passed
    CHECK (view.getOverloadLamp().getToggleState());

    view.getPeakHoldReset().onClick();
    CHECK (! view.getOverloadLamp().getToggleState());
}
