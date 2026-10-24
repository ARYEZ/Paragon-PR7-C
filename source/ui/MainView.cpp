#include "MainView.h"

#include "ui/Finish.h"
#include "ui/LedDisplay.h"
#include "ui/Logo.h"
#include "ui/ParagonLookAndFeel.h"

namespace
{
    // The meter window in the FILTERS / TONE column, its glass, and where
    // its scale runs (+24 dB at the top, -60 at the bottom).
    const juce::Rectangle<float> meterFrame { 955.0f, 343.0f, 257.0f, 522.0f };
    const juce::Rectangle<float> meterGlass { 965.0f, 353.0f, 237.0f, 502.0f };
    constexpr float meterTop = 381.0f, meterBottom = 723.0f;

    // The output fader's scale: +10 dB at the top, -60 at the bottom.
    constexpr float faderTop = 731.0f, faderBottom = 954.0f, faderX = 1332.0f;

    // The blank plate under LMF, which holds no module.
    const juce::Rectangle<float> blankPlate { 645.0f, 884.0f, 286.0f, 112.0f };

    // Modules whose plates have no lamp and title along the top.
    bool hasHeader (const juce::String& name) { return name != "STEREO MODE" && name != "OUTPUT TRIM"; }

    // Tooltips for controls that don't work yet.
    juce::String later (const juce::String& what, const char* step)
    {
        return what + "\n(Not working yet: it arrives with " + step + ".)";
    }
    constexpr auto eqStep = "the EQ and filters";
    constexpr auto compStep = "the compressor";
    constexpr auto limitStep = "the limiter";
    constexpr auto gateStep = "the gate";
    constexpr auto colourStep = "saturation and the console colour";
    constexpr auto stereoStep = "the stereo features";
    constexpr auto presetStep = "presets";

    void drawStamped (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, float size,
                      juce::Justification justification = juce::Justification::centred)
    {
        g.setFont (juce::FontOptions (size, juce::Font::bold));
        ParagonLookAndFeel::drawStampedText (g, text, area, justification, Finish::ink());
    }

    // "PARAGON PR7-C" with "CHANNEL STRIP" spaced out under it.
    void drawName (juce::Graphics& g, juce::Point<float> origin)
    {
        const juce::Rectangle<float> name { origin.x, origin.y, 240.0f, 26.0f };
        const auto font = juce::FontOptions (26.0f, juce::Font::bold | juce::Font::italic);
        g.setFont (font);
        g.setColour (ParagonColours::text);
        g.drawText ("PARAGON", name, juce::Justification::centredLeft, false);
        const auto nameWidth = (float) juce::GlyphArrangement::getStringWidthInt (font, "PARAGON ");
        Logo::drawMark (g, name.withTrimmedLeft (nameWidth).withSizeKeepingCentre (name.getWidth() - nameWidth, 20.0f),
                        juce::Justification::centredLeft, { ParagonColours::glow, ParagonColours::glow, {}, {}, 1.0f, true });

        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.setColour (ParagonColours::text.withAlpha (0.8f));
        g.drawText ("C H A N N E L   S T R I P", juce::Rectangle<float> (origin.x + 2.0f, origin.y + 28.0f, 240.0f, 14.0f),
                    juce::Justification::centredLeft, false);
    }

    // A module's plate: the finish's pearl, bevelled, with a shallow inset
    // panel below the lamp and title.
    void drawPlate (juce::Graphics& g, juce::Rectangle<float> bounds, bool header)
    {
        const auto black = Finish::isBlack();
        juce::Path plate;
        plate.addRoundedRectangle (bounds, 8.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 10, { 0, 3 }).drawForPath (g, plate);

        g.setGradientFill (juce::ColourGradient (Finish::pick (0xfff6f3f4, 0xff343033), 0.0f, bounds.getY(),
                                                 Finish::pick (0xffd6d0d3, 0xff191618), 0.0f, bounds.getBottom(), false));
        g.fillPath (plate);

        // The bevel: the top edge catching the light, the bottom in shadow.
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (black ? 0.25f : 0.95f), 0.0f, bounds.getY(),
                                                 juce::Colours::black.withAlpha (black ? 0.6f : 0.35f), 0.0f, bounds.getBottom(), false));
        g.drawRoundedRectangle (bounds.reduced (1.0f), 7.0f, 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        const auto inset = bounds.reduced (8.0f).withTrimmedTop (header ? 30.0f : 0.0f);
        g.setColour (black ? juce::Colours::white.withAlpha (0.025f) : juce::Colours::black.withAlpha (0.03f));
        g.fillRoundedRectangle (inset, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (black ? 0.08f : 0.8f));
        g.drawRoundedRectangle (inset.translated (0.0f, 1.0f), 6.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (black ? 0.5f : 0.14f));
        g.drawRoundedRectangle (inset, 6.0f, 1.0f);
    }

    // A black domed screw in a shallow socket.
    void drawScrew (juce::Graphics& g, juce::Point<float> c)
    {
        const auto circle = [&] (float radius) { return juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c); };
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.5f), c.x, c.y - 7.0f, juce::Colours::white.withAlpha (0.5f), c.x, c.y + 7.0f, false));
        g.fillEllipse (circle (7.0f));
        juce::ColourGradient dome (juce::Colour (0xff57555a), c.translated (-1.5f, -2.0f), juce::Colour (0xff050506), c.translated (4.0f, 3.0f), true);
        g.setGradientFill (dome);
        g.fillEllipse (circle (5.5f));
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.fillEllipse (juce::Rectangle<float> (2.6f, 1.8f).withCentre (c.translated (-1.6f, -2.4f)));
    }

    // A dark tray that a row of keys sits in (VCA / OPTO / FET).
    void drawTray (juce::Graphics& g, juce::Rectangle<float> tray)
    {
        g.setColour (juce::Colour (0xff111013));
        g.fillRoundedRectangle (tray, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (Finish::isBlack() ? 0.1f : 0.7f));
        g.drawRoundedRectangle (tray.translated (0.0f, 1.0f), 6.0f, 1.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (tray, 6.0f, 1.0f);
    }

    juce::String signedDb (float db) { return (db > 0.0f ? "+" : "") + juce::String (juce::roundToInt (db)); }
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
        { "IN / OUT",         { 1235.0f, 80.0f, 190.0f, 304.0f } },
        { "STEREO MODE",      { 1235.0f, 389.0f, 190.0f, 197.0f } },
        { "OUTPUT TRIM",      { 1235.0f, 591.0f, 190.0f, 405.0f } },
    };
    return list;
}

MainView::MainView (ParagonPR7CAudioProcessor& processorToShow)
    : processor (processorToShow),
      inputLeft (processorToShow.getInputMeter (0)),
      inputRight (processorToShow.getInputMeter (1)),
      outputLeft (processorToShow.getOutputMeter (0)),
      outputRight (processorToShow.getOutputMeter (1))
{
    using P = juce::Point<float>;
    using R = juce::Rectangle<int>;

    // HPF / LPF.
    addLed ("hpf.on", { 58, 105 }, later ("HPF on/off", eqStep));
    addLed ("lpf.on", { 287, 105 }, later ("LPF on/off", eqStep));
    addKnob ("hpf.freq", P (111, 187), 37, { "40", "80", "160", "320", "640", "1.2k" }, "Hz", {}, 0.0, later ("HPF frequency", eqStep));
    addKnob ("lpf.freq", P (243, 187), 37, { "3k", "6k", "10k", "14k", "20k", "30k" }, "Hz", {}, 1.0, later ("LPF frequency", eqStep));
    addSwitch ("filters.placement", R (166, 236, 22, 54), 3, 0, false,
               later ("Where the filters sit: IN (at the input), POST (after the compressor) or SC (in the dynamics' sidechain only)", eqStep));

    // GATE / EXP.
    addLed ("gate.on", { 58, 324 }, later ("Gate on/off", gateStep));
    addKnob ("gate.threshold", P (111, 400), 37, { "-60", "-40", "-20", "0" }, {}, "THRESHOLD", 0.0, later ("Gate threshold", gateStep));
    addKnob ("gate.range", P (243, 400), 37, { "0", "10", "20", "30", "40" }, {}, "RANGE", 0.0, later ("Range: the most the gate turns down. 0 is off", gateStep));
    addButton ("gate.minus30", "-30", R (70, 466, 85, 36), true, later ("-30: shifts the threshold range down 30 dB, for quiet material", gateStep));
    addButton ("gate.exp", "EXP", R (171, 466, 84, 36), true, later ("EXP: an expander instead of a gate", gateStep));
    addKnob ("gate.attack", P (94, 570), 31, { "0.03", "1", "10" }, {}, "ATTACK", 0.5, later ("Gate attack", gateStep));
    addKnob ("gate.release", P (199, 570), 31, { "0.5", "1", "2" }, {}, "RELEASE", 0.5, later ("Gate release", gateStep));
    addButton ("gate.fast", "FAST", R (250, 518, 60, 31), true, later ("FAST hold and release", gateStep));
    addButton ("gate.med", "MED", R (250, 554, 60, 31), true, later ("MED hold and release", gateStep));
    addButton ("gate.slow", "SLOW", R (250, 590, 60, 31), true, later ("SLOW hold and release", gateStep));
    addRadioGroup ({ "gate.fast", "gate.med", "gate.slow" }, 1, "gate.med");
    addKnob ("gate.ratio", P (116, 690), 35, { "1:1", "1.5:1", "3:1", "6:1", "20:1" }, {}, "RATIO", 0.5, later ("Expander ratio", gateStep));
    addSwitch ("gate.linear", R (251, 658, 21, 50), 2, 1, true, later ("LINEAR: a linear release instead of the usual exponential one", gateStep));

    // SC FILTER.
    addLed ("sc.on", { 58, 777 }, later ("Sidechain filter on/off", gateStep));
    addKnob ("sc.hpf", P (104, 847), 33, { "20", "60", "200" }, {}, "HPF", 0.0, later ("Sidechain high-pass: the gate and compressor ignore lows below it", gateStep));
    addKnob ("sc.lpf", P (248, 847), 33, { "1k", "4.5k", "20k" }, {}, "LPF", 1.0, later ("Sidechain low-pass: the gate and compressor ignore highs above it", gateStep));
    addButton ("sc.listen", {}, R (87, 923, 32, 38), true, later ("LISTEN: hear the sidechain signal", gateStep))
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);
    addButton ("sc.highQ", {}, R (196, 923, 32, 38), true, later ("HIGH Q: resonant sidechain filters", gateStep))
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);

    // COMP.
    addLed ("comp.on", { 373, 104 }, later ("Compressor on/off", compStep));
    addKnob ("comp.threshold", P (409, 185), 37, { "-40", "-30", "-20", "-10", "0" }, {}, "THRESHOLD", 1.0, later ("Compressor threshold", compStep));
    addKnob ("comp.ratio", P (556, 185), 37, { "1.5", "2", "4", "8", "12", "20" }, {}, "RATIO", 0.4, later ("Compressor ratio", compStep));
    addButton ("comp.vca", "VCA", R (368, 268, 78, 30), true, later ("VCA: clean, precise, hard knee", compStep));
    addButton ("comp.opto", "OPTO", R (448, 268, 78, 30), true, later ("OPTO: slow, programme-dependent, smooth release", compStep));
    addButton ("comp.fet", "FET", R (528, 268, 78, 30), true, later ("FET: fast and aggressive, adds harmonics", compStep));
    addRadioGroup ({ "comp.vca", "comp.opto", "comp.fet" }, 2, "comp.vca");
    addKnob ("comp.attack", P (409, 375), 34, { "0.1", "1", "10", "30" }, {}, "ATTACK", 0.5, later ("Compressor attack", compStep));
    addKnob ("comp.release", P (560, 375), 34, { "0.1", "0.3", "1", "3" }, {}, "RELEASE", 0.5, later ("Compressor release", compStep));
    addLed ("comp.auto", { 616, 393 }, later ("AUTO: programme-dependent release", compStep)).setToggleState (false, juce::dontSendNotification);

    // LIMIT.
    addLed ("limit.on", { 375, 538 }, later ("Limiter on/off", limitStep));
    addKnob ("limit.threshold", P (414, 614), 35, { "-20", "-15", "-10", "-5", "0" }, {}, "THRESHOLD", 1.0, later ("Limiter threshold", limitStep));
    addKnob ("limit.makeup", P (551, 614), 35, { "0", "3", "6", "9", "12" }, {}, "MAKE UP", 0.0, later ("Make-up gain", limitStep));
    addButton ("limit.brick", "BRICK", R (365, 690, 78, 32), true, later ("BRICK: a brick-wall limit", limitStep));
    addButton ("limit.soft", "SOFT", R (446, 690, 78, 32), true, later ("SOFT: a soft knee", limitStep));
    addButton ("limit.auto", "AUTO", R (527, 690, 78, 32), true, later ("AUTO: programme-dependent release", limitStep));
    addRadioGroup ({ "limit.brick", "limit.soft", "limit.auto" }, 3, "limit.brick");
    addKnob ("limit.mix", P (426, 775), 27, { "0", "50", "100" }, {}, "MIX %", 1.0, later ("Mix: how much of the limited signal you hear", limitStep));
    addButton ("limit.link", "LINK", R (527, 760, 76, 34), true, later ("LINK: the same gain reduction on both channels", limitStep))
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);

    // SATURATION.
    addLed ("sat.on", { 375, 850 }, later ("Saturation on/off", colourStep));
    addKnob ("sat.drive", P (401, 926), 31, { "0", "50", "100" }, {}, "DRIVE", 0.0, later ("Drive", colourStep));
    addButton ("sat.vcaClip", "VCA CLIP", R (459, 908, 70, 35), true, later ("VCA CLIP: a soft-to-hard clipper, like overdriving the console's VCA", colourStep));
    addKnob ("sat.tone", P (579, 926), 31, { "-12", "0", "+12" }, {}, "TONE", 0.5, later ("Saturation tone", colourStep));

    // EQ: four bands.
    struct Band { const char* id; const char* name; float top, bottom; float freqY, gainY; std::vector<juce::String> scale; bool shelf; };
    const Band bands[] {
        { "hf",  "HF",  80.0f,  274.0f, 179.0f, 172.0f, { "2k", "4k", "8k", "12k", "16k", "20k" }, true },
        { "hmf", "HMF", 279.0f, 493.0f, 374.0f, 370.0f, { "400", "800", "1.2k", "2.5k", "5k" }, false },
        { "mf",  "MF",  498.0f, 689.0f, 589.0f, 585.0f, { "150", "300", "600", "1.2k", "2.5k" }, false },
        { "lmf", "LMF", 694.0f, 879.0f, 787.0f, 777.0f, { "20", "40", "80", "150", "300" }, true },
    };
    for (const auto& band : bands)
    {
        const juce::String id (band.id), name (band.name);
        addLed (id + ".on", { 684.0f, band.top + 22.0f }, later (name + " band on/off", eqStep));
        addKnob (id + ".freq", P (732, band.freqY), 35, band.scale, "Hz", {}, 0.5, later (name + " frequency", eqStep));
        addKnob (id + ".gain", P (857, band.gainY), 31, { "-15", "0", "+15" }, {}, "GAIN", 0.5, later (name + " gain, -15 to +15 dB", eqStep));
        if (band.shelf)
        {
            const auto y = juce::roundToInt (band.freqY + 62.0f);
            addButton (id + ".shelf", "SHELF", R (672, y, 88, 25), true, later (name + ": shelf", eqStep)).setToggleState (true, juce::dontSendNotification);
            addButton (id + ".shape", {}, R (776, y, 54, 25), true, later (name + ": shelf or bell curve", eqStep))
                .getProperties().set (ParagonLookAndFeel::iconProperty, "shape");
        }
        else
        {
            // As on the concept, the Q knob's scale runs round its top
            // (0.5 at the left to 4 at the right), with "Q" at the bottom
            // right: there's no room under it.
            const auto qY = juce::jmin (band.freqY + 80.0f, band.bottom - 32.0f);
            auto& q = addKnob (id + ".q", P (800, qY), 18, {}, {}, {}, 0.5, later (name + " Q: the width of the bell", eqStep));
            q.setScale ({ { 1.0f / 6.0f, "0.5" }, { 0.5f, "1" }, { 2.0f / 3.0f, "2" }, { 5.0f / 6.0f, "4" } });
            q.addLabel (1.0f, "Q");
            addButton (id + ".shape", {}, R (850, juce::roundToInt (qY - 13.0f), 52, 25), true, later (name + ": shelf or bell curve", eqStep))
                .getProperties().set (ParagonLookAndFeel::iconProperty, "shape");
        }
    }

    // FILTERS / TONE.
    addLed ("tone.on", { 962, 104 }, later ("Console colour on/off", colourStep));
    addKnob ("tone.thd", P (1021, 165), 33, { "OFF", "100" }, {}, "THD", 0.0, later ("THD: harmonic saturation and density", colourStep));
    addKnob ("tone.vgain", P (1151, 166), 33, { "-15", "0", "+15" }, {}, "V GAIN", 0.5, later ("V GAIN: drive into the console colour, with the console's noise", colourStep));
    addKnob ("tone.channel", P (1086, 290), 32, {}, {}, {}, 0.0, later ("Picks the console channels (one of 72 each side)", colourStep));

    // The meter window.
    for (auto* meter : { &inputLeft, &inputRight, &outputLeft, &outputRight })
        addAndMakeVisible (*meter);
    const auto meterHeight = juce::roundToInt (meterBottom - meterTop);
    inputLeft.setBounds (1018, (int) meterTop, 12, meterHeight);
    inputRight.setBounds (1036, (int) meterTop, 12, meterHeight);
    outputLeft.setBounds (1112, (int) meterTop, 12, meterHeight);
    outputRight.setBounds (1130, (int) meterTop, 12, meterHeight);

    peakReset.setTooltip ("Clears the peak markers (the single lit segment above each meter's level)");
    peakReset.onClick = [this]
    {
        for (auto* meter : getLevelMeters())
            meter->resetPeakHold();
    };
    peakReset.setBounds (R (30, 30).withCentre ({ 1080, 746 }));
    addAndMakeVisible (peakReset);

    using Dynamics = ParagonPR7CAudioProcessor::Dynamics;
    const std::pair<Dynamics, int> reductionColumns[] { { Dynamics::gate, 1006 }, { Dynamics::comp, 1065 }, { Dynamics::limit, 1125 } };
    for (const auto& [dynamics, x] : reductionColumns)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            auto* meter = reductionMeters.add (new GainReductionMeter (processor.getGainReduction (dynamics, channel), GainReductionMeter::Style::vertical));
            meter->setBounds (x + channel * 23, 770, 13, 48);
            addAndMakeVisible (meter);
        }
    }
    {
        auto* meter = reductionMeters.add (new GainReductionMeter (processor.getGainReduction (Dynamics::comp, 0), GainReductionMeter::Style::gainChange));
        meter->setBounds (386, 458, 204, 15);
        addAndMakeVisible (meter);
    }

    // MONO MAKER / STEREO WIDTH.
    addLed ("mono.on", { 967, 892 }, later ("Mono maker on/off", stereoStep));
    addLed ("width.on", { 1102, 892 }, later ("Stereo width on/off", stereoStep));
    addKnob ("mono.freq", P (1020, 951), 24, { "20", "80", "200" }, "Hz", {}, 0.5, later ("Mono maker: everything below this is folded to mono", stereoStep));
    addKnob ("width.amount", P (1155, 951), 24, { "0", "100", "200" }, "%", {}, 0.5, later ("Stereo width: 100% leaves it as it is", stereoStep));

    // IN / OUT: INPUT, Ø and MUTE (working).
    addLed ("output.on", { 1265, 104 }, later ("The output stage on/off", stereoStep));
    auto& input = addKnob ("output.input", P (1339, 187), 43, {}, {}, "INPUT", 0.0,
                           "INPUT: the level into the strip, -24 to +12 dB. The IN meters show the signal after it.");
    input.setScale ({ { 0.0f, "-24" }, { 0.25f, "-12" }, { 0.5f, "0" }, { 1.0f, "+12" } });
    inputAttachment = std::make_unique<SliderAttachment> (processor.getParameters(), ParamIDs::inputGain, input.slider);
    input.slider.setDoubleClickReturnValue (true, 0.0);

    auto& phase = addButton ("output.phase", {}, R (1275, 272, 115, 48), true, "Ø: flips the polarity (phase) of both channels");
    phase.getProperties().set (ParagonLookAndFeel::iconProperty, "phase");
    phaseAttachment = std::make_unique<ButtonAttachment> (processor.getParameters(), ParamIDs::phaseInvert, phase);
    auto& mute = addButton ("output.mute", "MUTE", R (1275, 329, 115, 47), true, "MUTE: silences the output");
    mute.getProperties().set (ParagonLookAndFeel::alertProperty, true);
    muteAttachment = std::make_unique<ButtonAttachment> (processor.getParameters(), ParamIDs::mute, mute);

    // STEREO MODE.
    addButton ("stereo.analog", "ANALOG", R (1275, 421, 112, 32), true, later ("ANALOG: the left and right sides use different console channels", stereoStep));
    addButton ("stereo.digital", "DIGITAL", R (1275, 459, 112, 32), true, later ("DIGITAL: both sides use the same console channel", stereoStep));
    addRadioGroup ({ "stereo.analog", "stereo.digital" }, 4, "stereo.analog");
    addButton ("stereo.one", "ONE", R (1263, 521, 64, 32), false, later ("ONE: picks an unused random channel for this PR7-C", stereoStep));
    addButton ("stereo.all", "ALL", R (1333, 521, 64, 32), false, later ("ALL: picks new random channels for every PR7-C in the session", stereoStep));

    // OUTPUT TRIM and the fader (working).
    auto& trim = addKnob ("output.trim", P (1332, 647), 34, { "-12", "0", "+12" }, {}, "OUTPUT TRIM", 0.5, "OUTPUT TRIM: fine output level, -12 to +12 dB");
    trimAttachment = std::make_unique<SliderAttachment> (processor.getParameters(), ParamIDs::outputTrim, trim.slider);
    trim.slider.setDoubleClickReturnValue (true, 0.0);

    fader.getProperties().set (ParagonLookAndFeel::channelFaderProperty, true);
    fader.setTooltip ("The channel fader: the final output level, +10 to -60 dB. Double-click for 0 dB.");
    fader.setBounds (juce::roundToInt (faderX) - 27, juce::roundToInt (faderTop) - ParagonLookAndFeel::channelFaderThumbRadius,
                     54, juce::roundToInt (faderBottom - faderTop) + ParagonLookAndFeel::channelFaderThumbRadius * 2);
    faderAttachment = std::make_unique<SliderAttachment> (processor.getParameters(), ParamIDs::outputLevel, fader);
    fader.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (fader);
    controls["output.fader"] = &fader;

    // The header.
    addButton ("preset.previous", {}, R (522, 24, 32, 32), false, later ("The previous preset", presetStep))
        .getProperties().set (ParagonLookAndFeel::iconProperty, "left");
    addButton ("preset.next", {}, R (894, 24, 32, 32), false, later ("The next preset", presetStep))
        .getProperties().set (ParagonLookAndFeel::iconProperty, "right");
    for (const auto* id : { "preset.previous", "preset.next" })
        findControl (id)->getProperties().set (ParagonLookAndFeel::bareProperty, true);
    addButton ("preset.save", "SAVE", R (1203, 22, 96, 36), false, later ("Save the settings as a preset", presetStep));
    addButton ("preset.menu", {}, R (1314, 22, 56, 36), false, later ("More preset options", presetStep))
        .getProperties().set (ParagonLookAndFeel::iconProperty, "dots");
    auto& settings = addButton ("settings", {}, R (1382, 20, 40, 40), false, "Settings: the panel's finish");
    settings.getProperties().set (ParagonLookAndFeel::iconProperty, "gear");
    settings.getProperties().set (ParagonLookAndFeel::bareProperty, true);
    settings.onClick = [this] { showSettingsMenu(); };

    grain = Finish::makeSatinGrain (width, height, 7);
    Finish::changes().addChangeListener (this);
}

MainView::~MainView()
{
    Finish::changes().removeChangeListener (this);
}

juce::Component* MainView::findControl (const juce::String& id) const
{
    const auto found = controls.find (id);
    return found != controls.end() ? found->second : nullptr;
}

Knob& MainView::addKnob (const juce::String& id, juce::Point<float> centre, float ringRadius, std::vector<juce::String> scale,
                         juce::String unit, juce::String caption, double initial, const juce::String& tooltip)
{
    auto* knob = new Knob (std::move (scale), std::move (unit), std::move (caption));
    ownedControls.add (knob);
    knob->place (centre, ringRadius);
    knob->slider.setValue (initial, juce::dontSendNotification);
    knob->slider.setDoubleClickReturnValue (true, initial);
    knob->slider.setTooltip (tooltip);
    addAndMakeVisible (knob);
    controls[id] = knob;
    return *knob;
}

juce::TextButton& MainView::addButton (const juce::String& id, const juce::String& text, juce::Rectangle<int> bounds,
                                       bool toggles, const juce::String& tooltip)
{
    auto* button = new juce::TextButton (text);
    ownedControls.add (button);
    button->getProperties().set (ParagonLookAndFeel::consoleButtonProperty, true);
    button->setClickingTogglesState (toggles);
    button->setTooltip (tooltip);
    button->setBounds (bounds);
    addAndMakeVisible (button);
    controls[id] = button;
    return *button;
}

Led& MainView::addLed (const juce::String& id, juce::Point<float> centre, const juce::String& tooltip)
{
    auto* led = new Led();
    ownedControls.add (led);
    led->setToggleState (true, juce::dontSendNotification);   // as on the concept: every module on
    led->setTooltip (tooltip);
    led->setBounds (juce::Rectangle<int> (28, 28).withCentre (centre.toInt()));
    addAndMakeVisible (led);
    controls[id] = led;
    return *led;
}

LeverSwitch& MainView::addSwitch (const juce::String& id, juce::Rectangle<int> bounds, int positions, int initial, bool glow,
                                  const juce::String& tooltip)
{
    auto* lever = new LeverSwitch (positions, initial, glow);
    ownedControls.add (lever);
    lever->setTooltip (tooltip);
    lever->setBounds (bounds);
    addAndMakeVisible (lever);
    controls[id] = lever;
    return *lever;
}

void MainView::addRadioGroup (std::initializer_list<const char*> ids, int group, const char* initiallyOn)
{
    for (const auto* id : ids)
    {
        auto* button = dynamic_cast<juce::Button*> (findControl (id));
        jassert (button != nullptr);
        button->setRadioGroupId (group, juce::dontSendNotification);
        button->setToggleState (juce::String (id) == initiallyOn, juce::dontSendNotification);
    }
}

void MainView::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("Finish");
    // In the order they're best read, not the order they're saved in.
    for (const auto type : { Finish::Type::pearlWhite, Finish::Type::pearlWhiteNeon, Finish::Type::pearlBlack })
        menu.addItem (Finish::names[(int) type], true, Finish::get() == type, [type] { Finish::set (type); });

    auto* settings = findControl ("settings");
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (settings));
}

void MainView::changeListenerCallback (juce::ChangeBroadcaster*)
{
    grain = Finish::makeSatinGrain (width, height, 7);
    repaint();
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (ParagonColours::background);

    // Header: the emblem, the name, and the preset display.
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
        drawName (g, { 98.0f, 18.0f });

        const juce::Rectangle<float> preset { 513.0f, 18.0f, 422.0f, 44.0f };
        g.setColour (juce::Colour (0xff0a090b));
        g.fillRoundedRectangle (preset, 6.0f);
        g.setColour (ParagonColours::panelEdge);
        g.drawRoundedRectangle (preset, 6.0f, 1.0f);
        g.setColour (juce::Colour (0xffffb04a));
        g.setFont (juce::FontOptions (19.0f));
        g.drawText ("Untitled", preset, juce::Justification::centred, false);
    }

    // The body: a dark frame round the modules, edged in red-orange light.
    {
        const juce::Rectangle<float> body { 13.0f, 71.0f, 1422.0f, 934.0f };
        g.setColour (juce::Colour (0xff0e0c0e));
        g.fillRoundedRectangle (body, 16.0f);
        for (const auto& [alpha, thickness] : { std::pair (0.07f, 16.0f), { 0.16f, 8.0f }, { 0.4f, 4.0f }, { 0.9f, 1.6f } })
        {
            g.setColour (ParagonColours::glow.withAlpha (alpha));
            g.drawRoundedRectangle (body, 16.0f, thickness);
        }
    }

    // The modules' plates, the satin grain over them, and their screws.
    juce::Path plates;
    for (const auto& module : modules())
    {
        drawPlate (g, module.bounds, hasHeader (module.name));
        plates.addRoundedRectangle (module.bounds, 8.0f);
    }
    drawPlate (g, blankPlate, false);
    plates.addRoundedRectangle (blankPlate, 8.0f);
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (plates);
        g.drawImageAt (grain, 0, 0);
    }
    for (const auto& module : modules())
    {
        const auto inner = module.bounds.reduced (14.0f);
        for (auto corner : { inner.getBottomLeft(), inner.getBottomRight() })
            drawScrew (g, corner);
        if (juce::String (module.name) != "MONO MAKER / STEREO WIDTH")   // its second title runs to the corner
            drawScrew (g, inner.getTopRight());
        if (! hasHeader (module.name))
            drawScrew (g, inner.getTopLeft());
    }
    for (auto corner : { blankPlate.reduced (14.0f).getTopLeft(), blankPlate.reduced (14.0f).getTopRight(),
                         blankPlate.reduced (14.0f).getBottomLeft(), blankPlate.reduced (14.0f).getBottomRight() })
        drawScrew (g, corner);

    // Module titles, beside their lamps.
    const auto title = [&] (const juce::String& text, float x, float y, juce::Justification justification = juce::Justification::centredLeft)
    {
        drawStamped (g, text, juce::Rectangle<float> (x, y - 11.0f, 220.0f, 22.0f), 17.0f, justification);
    };
    title ("HPF", 78.0f, 105.0f);
    drawStamped (g, "LPF", { 168.0f, 94.0f, 100.0f, 22.0f }, 17.0f, juce::Justification::centredRight);
    title ("GATE / EXP", 78.0f, 324.0f);
    title ("SC FILTER", 78.0f, 777.0f);
    title ("COMP", 394.0f, 104.0f);
    title ("LIMIT", 396.0f, 538.0f);
    title ("SATURATION", 396.0f, 850.0f);
    for (const auto& [name, y] : { std::pair ("HF", 102.0f), { "HMF", 301.0f }, { "MF", 520.0f }, { "LMF", 716.0f } })
        title (name, 704.0f, y);
    title ("FILTERS / TONE", 982.0f, 104.0f);
    drawStamped (g, "MONO MAKER", { 984.0f, 881.0f, 120.0f, 22.0f }, 14.5f, juce::Justification::centredLeft);
    drawStamped (g, "STEREO WIDTH", { 1118.0f, 881.0f, 104.0f, 22.0f }, 14.5f, juce::Justification::centredLeft);
    title ("IN / OUT", 1285.0f, 104.0f);
    drawStamped (g, "STEREO MODE", { 1235.0f, 395.0f, 190.0f, 20.0f }, 14.5f);

    // Lettering printed by the switches and keys.
    drawStamped (g, "IN", { 192.0f, 238.0f, 50.0f, 14.0f }, 12.0f, juce::Justification::centredLeft);
    drawStamped (g, "POST", { 192.0f, 256.0f, 50.0f, 14.0f }, 12.0f, juce::Justification::centredLeft);
    drawStamped (g, "SC", { 192.0f, 274.0f, 50.0f, 14.0f }, 12.0f, juce::Justification::centredLeft);
    drawStamped (g, "LINEAR", { 180.0f, 676.0f, 66.0f, 14.0f }, 12.0f, juce::Justification::centredRight);
    drawStamped (g, "LISTEN", { 127.0f, 935.0f, 70.0f, 16.0f }, 13.5f, juce::Justification::centredLeft);
    drawStamped (g, "HIGH Q", { 236.0f, 935.0f, 70.0f, 16.0f }, 13.5f, juce::Justification::centredLeft);
    drawStamped (g, "AUTO", { 590.0f, 408.0f, 52.0f, 14.0f }, 12.0f);
    drawStamped (g, "LEFT", { 965.0f, 318.0f, 77.0f, 16.0f }, 13.0f);
    drawStamped (g, "RIGHT", { 1129.0f, 318.0f, 77.0f, 16.0f }, 13.0f);
    drawStamped (g, "RANDOM CH.", { 1235.0f, 500.0f, 190.0f, 16.0f }, 12.5f);

    // The keys' trays.
    drawTray (g, { 364.0f, 265.0f, 245.0f, 36.0f });
    drawTray (g, { 361.0f, 687.0f, 248.0f, 38.0f });

    // COMP's gain-change meter: a dark window with its scale.
    {
        const juce::Rectangle<float> window { 351.0f, 447.0f, 271.0f, 47.0f };
        g.setColour (juce::Colour (0xff050405));
        g.fillRoundedRectangle (window, 5.0f);
        g.setColour (juce::Colours::white.withAlpha (Finish::isBlack() ? 0.1f : 0.7f));
        g.drawRoundedRectangle (window.translated (0.0f, 1.0f), 5.0f, 1.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (window, 5.0f, 1.0f);
        g.setColour (juce::Colour (0xffe8e2dc));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("GR", juce::Rectangle<float> (356.0f, 457.0f, 28.0f, 16.0f), juce::Justification::centred, false);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        for (const auto db : { -20.0f, -10.0f, 0.0f, 10.0f, 20.0f })
        {
            const auto x = 386.0f + 204.0f * (db + 20.0f) / 40.0f;
            g.drawText (signedDb (db), juce::Rectangle<float> (36.0f, 14.0f).withCentre ({ x, 485.0f }), juce::Justification::centred, false);
        }
    }

    // FILTERS / TONE: the channel badge and the two channel numbers.
    {
        const juce::Rectangle<float> badge { 1028.0f, 229.0f, 116.0f, 21.0f };
        g.setColour (juce::Colour (0xff141215));
        g.fillRoundedRectangle (badge, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (badge.reduced (1.5f), 3.0f, 0.8f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("CONSOLE CHANNEL", badge, juce::Justification::centred, false);

        for (const auto& [area, number] : { std::pair (juce::Rectangle<float> (965.0f, 256.0f, 77.0f, 56.0f), "01"),
                                            { juce::Rectangle<float> (1129.0f, 256.0f, 77.0f, 56.0f), "02" } })
        {
            const auto glass = LedDisplay::paintScreen (g, area, 0.5f);
            auto digits = glass.reduced (glass.getWidth() * 0.16f, glass.getHeight() * 0.2f);
            const auto digitWidth = digits.getWidth() * 0.44f;
            LedDisplay::drawDigit (g, number[0], digits.removeFromLeft (digitWidth), LedDisplay::ledOrange);
            LedDisplay::drawDigit (g, number[1], digits.removeFromRight (digitWidth), LedDisplay::ledOrange);
            LedDisplay::paintGlassOverlay (g, glass, 0.5f);
        }
    }

    // The meter window: a dark frame, the glass, and the scale either side.
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a373b), 0.0f, meterFrame.getY(), juce::Colour (0xff121013), 0.0f, meterFrame.getBottom(), false));
        g.fillRoundedRectangle (meterFrame, 8.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (meterFrame, 8.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.drawRoundedRectangle (meterFrame.reduced (1.5f), 7.0f, 0.8f);
        g.setColour (juce::Colour (0xff050405));
        g.fillRoundedRectangle (meterGlass, 5.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (meterGlass, 5.0f, 1.5f);

        const juce::Colour lettering { 0xffe8e2dc };
        g.setColour (lettering);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        for (const auto db : LevelMeter::scaleMarksDb)
        {
            const auto y = meterBottom - LevelMeter::proportionForDb (db) * (meterBottom - meterTop);
            for (const auto x : { 999.0f, 1170.0f })
                g.drawText (signedDb (db), juce::Rectangle<float> (34.0f, 14.0f).withCentre ({ x, y }), juce::Justification::centred, false);
        }

        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText ("IN", juce::Rectangle<float> (40.0f, 16.0f).withCentre ({ 1033.0f, 748.0f }), juce::Justification::centred, false);
        g.drawText ("OUT", juce::Rectangle<float> (40.0f, 16.0f).withCentre ({ 1127.0f, 748.0f }), juce::Justification::centred, false);

        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText ("L", juce::Rectangle<float> (20.0f, 14.0f).withCentre ({ 988.0f, 794.0f }), juce::Justification::centred, false);
        g.drawText ("R", juce::Rectangle<float> (20.0f, 14.0f).withCentre ({ 1182.0f, 794.0f }), juce::Justification::centred, false);
        for (const auto& [name, x] : { std::pair ("GR", 1024.0f), { "COMP", 1083.0f }, { "LIMIT", 1143.0f }, { "dB", 1184.0f } })
            g.drawText (name, juce::Rectangle<float> (50.0f, 14.0f).withCentre ({ x, 832.0f }), juce::Justification::centred, false);
    }

    // The fader's scale: a long line at each mark, short ones between.
    {
        for (int i = 0; i < (int) std::size (ParamRanges::faderMarksDb); ++i)
        {
            const auto db = ParamRanges::faderMarksDb[i];
            const auto y = faderBottom - Parameters::faderProportion (db) * (faderBottom - faderTop);
            g.setColour (Finish::ink().withAlpha (0.85f));
            g.fillRect (juce::Rectangle<float> (1293.0f, y - 0.7f, 80.0f, 1.4f));
            drawStamped (g, juce::String (juce::roundToInt (db)), { 1244.0f, y - 8.0f, 42.0f, 16.0f }, 13.0f, juce::Justification::centredRight);

            if (i + 1 < (int) std::size (ParamRanges::faderMarksDb))
            {
                const auto nextY = faderBottom - Parameters::faderProportion (ParamRanges::faderMarksDb[i + 1]) * (faderBottom - faderTop);
                g.setColour (Finish::ink().withAlpha (0.4f));
                for (int step = 1; step < 5; ++step)
                    g.fillRect (juce::Rectangle<float> (1302.0f, y + (nextY - y) * (float) step / 5.0f - 0.4f, 62.0f, 0.8f));
            }
        }
    }

    // Footer.
    drawName (g, { 32.0f, 1024.0f });
    g.setColour (ParagonColours::textDim);
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText ("P R E C I S I O N .     T O N E .     C O N T R O L .", juce::Rectangle<float> (0.0f, 1032.0f, (float) width, 30.0f),
                juce::Justification::centred, false);
    g.drawText ("P R 7   P R O C E S S O R   S E R I E S", juce::Rectangle<float> (1000.0f, 1032.0f, 416.0f, 30.0f),
                juce::Justification::centredRight, false);
}
