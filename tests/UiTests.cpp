#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"

using Catch::Matchers::WithinAbs;

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
    REQUIRE (modules.size() == 15);
    for (size_t i = 0; i < modules.size(); ++i)
    {
        CHECK (canvas.contains (modules[i].bounds));
        for (size_t j = i + 1; j < modules.size(); ++j)
            CHECK (! modules[i].bounds.intersects (modules[j].bounds));
    }
}
