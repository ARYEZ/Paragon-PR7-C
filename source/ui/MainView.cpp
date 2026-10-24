#include "MainView.h"

#include "ui/Logo.h"
#include "ui/ParagonLookAndFeel.h"

namespace
{
    // The meter window in the FILTERS / TONE column, and the four meters in it.
    const juce::Rectangle<float> meterWindow { 958.0f, 346.0f, 248.0f, 518.0f };
}

const std::vector<MainView::Module>& MainView::modules()
{
    // Positions measured off docs/concept.png, column by column.
    static const std::vector<Module> list {
        { "HPF / LPF",        { 24.0f, 80.0f, 302.0f, 224.0f } },
        { "GATE / EXP",       { 24.0f, 310.0f, 302.0f, 446.0f } },
        { "SC FILTER",        { 24.0f, 762.0f, 302.0f, 234.0f } },
        { "COMP",             { 334.0f, 80.0f, 302.0f, 432.0f } },
        { "LIMIT",            { 334.0f, 518.0f, 302.0f, 310.0f } },
        { "SATURATION",       { 334.0f, 834.0f, 302.0f, 162.0f } },
        { "HF",               { 645.0f, 80.0f, 286.0f, 194.0f } },
        { "HMF",              { 645.0f, 279.0f, 286.0f, 214.0f } },
        { "MF",               { 645.0f, 498.0f, 286.0f, 191.0f } },
        { "LMF",              { 645.0f, 694.0f, 286.0f, 185.0f } },
        { "FILTERS / TONE",   { 938.0f, 80.0f, 288.0f, 790.0f } },
        { "MONO MAKER / STEREO WIDTH", { 938.0f, 876.0f, 288.0f, 120.0f } },
        { "OUTPUT",           { 1235.0f, 80.0f, 190.0f, 304.0f } },
        { "STEREO MODE",      { 1235.0f, 389.0f, 190.0f, 197.0f } },
        { "OUTPUT TRIM",      { 1235.0f, 591.0f, 190.0f, 405.0f } },
    };
    return list;
}

MainView::MainView (ParagonPR7CAudioProcessor& processorToShow)
    : inputLeft (processorToShow.getInputMeter (0)),
      inputRight (processorToShow.getInputMeter (1)),
      outputLeft (processorToShow.getOutputMeter (0)),
      outputRight (processorToShow.getOutputMeter (1)),
      outputLevelAttachment (processorToShow.getParameters(), ParamIDs::outputLevel, outputLevel)
{
    for (auto* meter : { &inputLeft, &inputRight, &outputLeft, &outputRight })
        addAndMakeVisible (*meter);

    outputLevel.setTooltip ("OUTPUT LEVEL: the strip's output, after every module");
    outputLevel.setDoubleClickReturnValue (true, 0.0);
    outputLevel.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (outputLevel);
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (ParagonColours::background);

    // Header: the emblem, PARAGON PR7-C and CHANNEL STRIP under it.
    {
        const juce::Rectangle<float> emblem { 34.0f, 14.0f, 42.0f, 40.0f };
        juce::Path triangle;
        triangle.addTriangle (emblem.getTopLeft(), emblem.getTopRight(), { emblem.getCentreX(), emblem.getBottom() });
        g.setColour (ParagonColours::glow.withAlpha (0.3f));
        g.strokePath (triangle, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (ParagonColours::glow);
        g.strokePath (triangle, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        triangle.applyTransform (juce::AffineTransform::scale (0.5f, 0.5f, emblem.getCentreX(), emblem.getCentreY() - 3.0f));
        g.strokePath (triangle, juce::PathStrokeType (1.5f));

        const juce::Rectangle<float> name { 98.0f, 18.0f, 240.0f, 26.0f };
        const auto font = juce::FontOptions (26.0f, juce::Font::bold | juce::Font::italic);
        g.setFont (font);
        g.setColour (ParagonColours::text);
        g.drawText ("PARAGON", name, juce::Justification::centredLeft, false);
        const auto nameWidth = (float) juce::GlyphArrangement::getStringWidthInt (font, "PARAGON ");
        Logo::drawMark (g, name.withTrimmedLeft (nameWidth).withSizeKeepingCentre (name.getWidth() - nameWidth, 20.0f),
                        juce::Justification::centredLeft, { ParagonColours::glow, ParagonColours::glow, {}, {}, 1.0f, true });

        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.setColour (ParagonColours::text.withAlpha (0.8f));
        g.drawText ("C H A N N E L   S T R I P", juce::Rectangle<float> (100.0f, 46.0f, 240.0f, 14.0f), juce::Justification::centredLeft, false);
    }
    g.setColour (ParagonColours::panelEdge);
    g.drawHorizontalLine (70, 0.0f, (float) width);

    // Each module's place: a dashed outline and its name, until it's built.
    for (const auto& module : modules())
    {
        juce::Path outline;
        outline.addRoundedRectangle (module.bounds, 8.0f);
        juce::Path dashed;
        const float dashes[] = { 6.0f, 5.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (ParagonColours::panelEdge.brighter (0.6f));
        g.fillPath (dashed);
        g.setColour (ParagonColours::textDim);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (module.name, module.bounds.reduced (16.0f, 10.0f).removeFromTop (20.0f), juce::Justification::centredLeft, true);
    }

    // The meter window: dark glass with IN and OUT, left and right.
    g.setColour (juce::Colour (0xff050405));
    g.fillRoundedRectangle (meterWindow, 6.0f);
    g.setColour (ParagonColours::panelEdge);
    g.drawRoundedRectangle (meterWindow, 6.0f, 1.5f);
    g.setColour (ParagonColours::textDim);
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    auto labels = meterWindow.reduced (12.0f).removeFromBottom (20.0f);
    g.drawText ("IN", labels.removeFromLeft (labels.getWidth() / 2), juce::Justification::centred, false);
    g.drawText ("OUT", labels, juce::Justification::centred, false);

    g.setColour (ParagonColours::text);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("OUTPUT LEVEL", juce::Rectangle<float> (1235.0f, 340.0f, 190.0f, 20.0f), juce::Justification::centred, false);

    // Footer.
    g.setColour (ParagonColours::panelEdge);
    g.drawHorizontalLine (1008, 0.0f, (float) width);
    g.setColour (ParagonColours::textDim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("P R E C I S I O N .     T O N E .     C O N T R O L .", juce::Rectangle<float> (0.0f, 1030.0f, (float) width, 30.0f),
                juce::Justification::centred, false);
}

void MainView::resized()
{
    // The meters: IN on the left half of the window, OUT on the right, each
    // a left and a right bar.
    auto area = meterWindow.reduced (16.0f, 18.0f).withTrimmedBottom (28.0f);
    auto in = area.removeFromLeft (area.getWidth() / 2).reduced (10.0f, 0.0f);
    auto out = area.reduced (10.0f, 0.0f);
    inputLeft.setBounds (in.removeFromLeft (in.getWidth() / 2).reduced (4.0f, 0.0f).toNearestInt());
    inputRight.setBounds (in.reduced (4.0f, 0.0f).toNearestInt());
    outputLeft.setBounds (out.removeFromLeft (out.getWidth() / 2).reduced (4.0f, 0.0f).toNearestInt());
    outputRight.setBounds (out.reduced (4.0f, 0.0f).toNearestInt());

    outputLevel.setBounds (1275, 160, 110, 170);
}
