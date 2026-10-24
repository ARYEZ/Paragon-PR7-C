#include "PluginEditor.h"

ParagonPR7CAudioProcessorEditor::ParagonPR7CAudioProcessorEditor (ParagonPR7CAudioProcessor& processorToEdit)
    : AudioProcessorEditor (processorToEdit),
      view (processorToEdit)
{
    setLookAndFeel (&look->lookAndFeel);

    view.setSize (MainView::width, MainView::height);
    addAndMakeVisible (view);

    // Keep the concept's shape at any size the user drags to.
    setResizable (true, true);
    setResizeLimits (MainView::width / 2, MainView::height / 2, MainView::width * 3 / 2, MainView::height * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) MainView::width / (double) MainView::height);
    const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
    const auto size = openingSize (display != nullptr ? display->userBounds.toNearestInt()
                                                      : juce::Rectangle<int> (MainView::width, MainView::height + 80));
    setSize (size.x, size.y);
}

ParagonPR7CAudioProcessorEditor::~ParagonPR7CAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

juce::Point<int> ParagonPR7CAudioProcessorEditor::openingSize (juce::Rectangle<int> screen)
{
    // `screen` is the part above the taskbar (or below the menu bar). Leave
    // room for the DAW's window frame, title bar and plugin toolbar.
    const auto fitHeight = juce::jmin (screen.getHeight() - 80, (screen.getWidth() - 40) * MainView::height / MainView::width);
    const auto height = juce::jlimit (MainView::height / 2, MainView::height, fitHeight);
    return { height * MainView::width / MainView::height, height };
}

void ParagonPR7CAudioProcessorEditor::resized()
{
    // Scale the whole view rather than re-laying it out, so it looks the
    // same at every size. JUCE draws through the transform, so text and
    // shapes stay sharp.
    view.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) MainView::width));
}
