#include <catch2/catch_test_macros.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ui/Finish.h"

// Hidden test (the "[.]" tag): renders the editor to a PNG for reviewing the
// UI without a DAW. Run with:
//   PARAGON_SNAPSHOT=editor.png ./ParagonPR7CTests "[snapshot]"
// PARAGON_SNAPSHOT_WIDTH sets the window width (default: the canvas's 1448;
// 2896 for a 2x render). PARAGON_SNAPSHOT_FINISH=black or =neon picks the finish;
// PARAGON_SNAPSHOT_DEMO=1 switches some modules on to show them working.
TEST_CASE ("Render the editor to a PNG", "[.][snapshot]")
{
    const auto path = juce::SystemStats::getEnvironmentVariable ("PARAGON_SNAPSHOT", "editor.png");
    const auto finish = juce::SystemStats::getEnvironmentVariable ("PARAGON_SNAPSHOT_FINISH", {});
    Finish::set (finish == "black" ? Finish::Type::pearlBlack : finish == "neon" ? Finish::Type::pearlWhiteNeon : Finish::Type::pearlWhite, false);

    ParagonPR7CAudioProcessor processor;
    processor.prepareToPlay (48000.0, 256);

    // PARAGON_SNAPSHOT_DEMO=1: some modules switched on and set, to show
    // them working (the expander, the compressor squeezing with make-up, then the limiter, saturation, console colour and stereo tools).
    if (juce::SystemStats::getEnvironmentVariable ("PARAGON_SNAPSHOT_DEMO", {}) == "1")
    {
        for (const auto& [id, value] : { std::pair (ParamIDs::compOn, 1.0f), { ParamIDs::compThreshold, -32.0f }, { ParamIDs::compMakeUp, 6.0f },
                                         { ParamIDs::compAuto, 1.0f }, { ParamIDs::limitOn, 1.0f }, { ParamIDs::limitThreshold, -24.0f },
                                         { ParamIDs::limitMakeUp, 4.0f }, { ParamIDs::gateOn, 1.0f }, { ParamIDs::gateThreshold, 0.0f },
                                         { ParamIDs::gateExpander, 1.0f }, { ParamIDs::gateRatio, 1.5f }, { ParamIDs::gateHold, 0.0f },
                                         { ParamIDs::gateRelease, 5.0f }, { ParamIDs::scOn, 1.0f }, { ParamIDs::satOn, 1.0f },
                                         { ParamIDs::satDrive, 12.0f }, { ParamIDs::toneThd, 60.0f }, { ParamIDs::toneVGain, 30.0f },
                                         { ParamIDs::toneChannel, 28.0f }, { ParamIDs::monoOn, 1.0f }, { ParamIDs::monoFreq, 120.0f },
                                         { ParamIDs::widthOn, 1.0f }, { ParamIDs::width, 140.0f } })
        {
            auto* parameter = processor.getParameters().getParameter (id);
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        }
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    const auto width = juce::SystemStats::getEnvironmentVariable ("PARAGON_SNAPSHOT_WIDTH", juce::String (MainView::width)).getIntValue();
    editor->setSize (width, width * MainView::height / MainView::width);

    // Play some signal and let the meters' timers pick it up, so they show.
    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;
    for (int round = 0; round < 8; ++round)
    {
        for (int i = 0; i < 256; ++i)
        {
            buffer.setSample (0, i, 0.5f * std::sin ((float) i * 0.1f));
            buffer.setSample (1, i, 0.3f * std::sin ((float) i * 0.13f));
        }
        processor.processBlock (buffer, midi);
        juce::Timer::callPendingTimersSynchronously();
        juce::Thread::sleep (40);
    }

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds());
    juce::File (juce::File::getCurrentWorkingDirectory().getChildFile (path)).deleteFile();
    juce::FileOutputStream stream (juce::File::getCurrentWorkingDirectory().getChildFile (path));
    REQUIRE (stream.openedOk());
    juce::PNGImageFormat png;
    REQUIRE (png.writeImageToStream (image, stream));
}
