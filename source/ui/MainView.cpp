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

    // Modules whose plates have no lamp and title along the top.
    bool hasHeader (const juce::String& name) { return name != "STEREO MODE" && name != "OUTPUT TRIM"; }

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

        // Pearl black is near black: the PR7-G black unit's own face
        // gradient (#1a1819 at the lit top edge down to #070707).
        juce::ColourGradient pearl (Finish::pick (0xfff6f3f4, 0xff1a1819), 0.0f, bounds.getY(),
                                    Finish::pick (0xffd6d0d3, 0xff070707), 0.0f, bounds.getBottom(), false);
        if (black)
        {
            pearl.addColour (0.18, juce::Colour (0xff121112));
            pearl.addColour (0.42, juce::Colour (0xff0a0a0a));
            pearl.addColour (0.7, juce::Colour (0xff0e0d0e));
        }
        g.setGradientFill (pearl);
        g.fillPath (plate);

        // The bevel: the top edge catching the light, the bottom in shadow.
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (black ? 0.25f : 0.95f), 0.0f, bounds.getY(),
                                                 juce::Colours::black.withAlpha (black ? 0.6f : 0.35f), 0.0f, bounds.getBottom(), false));
        g.drawRoundedRectangle (bounds.reduced (1.0f), 7.0f, 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        const auto inset = bounds.reduced (8.0f).withTrimmedTop (header ? 30.0f : 0.0f);
        g.setColour (juce::Colours::black.withAlpha (black ? 0.15f : 0.03f));   // a shallow recess, never lighter on black
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
        // Five EQ bands: the concept's four, re-spaced to fit LF on its
        // blank plate. The bands with a Q knob need more height.
        { "HF",               { 645.0f, 80.0f, 286.0f, 170.0f } },
        { "HMF",              { 645.0f, 255.0f, 286.0f, 191.0f } },
        { "MF",               { 645.0f, 451.0f, 286.0f, 191.0f } },
        { "LMF",              { 645.0f, 647.0f, 286.0f, 170.0f } },
        { "LF",               { 645.0f, 822.0f, 286.0f, 174.0f } },
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
    addLed ("hpf.on", { 58, 105 }, "HPF on/off");
    addLed ("lpf.on", { 287, 105 }, "LPF on/off");
    addKnob ("hpf.freq", P (111, 187), 37, { "40", "80", "160", "320", "640", "1.2k" }, "Hz", {}, 0.0, "HPF frequency");
    addKnob ("lpf.freq", P (243, 187), 37, { "3k", "6k", "10k", "14k", "20k", "30k" }, "Hz", {}, 1.0, "LPF frequency");
    addSwitch ("filters.placement", R (166, 236, 22, 54), 3, 0, false,
               "Where the filters sit: IN (at the input), POST (after the compressor) or SC (in the dynamics' sidechain only)");

    // GATE / EXP.
    addLed ("gate.on", { 58, 324 }, "Gate on/off");
    addKnob ("gate.threshold", P (111, 400), 37, { "-60", "-40", "-20", "0" }, {}, "THRESHOLD", 0.0, "Gate threshold");
    addKnob ("gate.range", P (243, 400), 37, { "0", "10", "20", "30", "40" }, {}, "RANGE", 0.0, "Range: the most the gate turns down. 0 is off");
    addButton ("gate.minus30", "-30", R (70, 466, 85, 36), true, "-30: shifts the threshold range down 30 dB, for quiet material");
    addButton ("gate.exp", "EXP", R (171, 466, 84, 36), true, "EXP: an expander instead of a gate");
    addKnob ("gate.hold", P (94, 570), 31, { "0", "50", "500" }, {}, "HOLD", 0.3, "Hold: how long (ms) the gate stays open before it starts to close");
    addKnob ("gate.release", P (199, 570), 31, { "0.5", "1", "2" }, {}, "RELEASE", 0.5, "Gate release");
    addButton ("gate.fast", "FAST", R (250, 518, 60, 31), true, "Gate attack: FAST, for drums");
    addButton ("gate.med", "MED", R (250, 554, 60, 31), true, "Gate attack: MED");
    addButton ("gate.slow", "SLOW", R (250, 590, 60, 31), true, "Gate attack: SLOW, for smooth fades");
    addRadioGroup ({ "gate.fast", "gate.med", "gate.slow" }, 1, "gate.med");
    addKnob ("gate.ratio", P (116, 690), 35, { "1:1", "1.5:1", "3:1", "6:1", "20:1" }, {}, "RATIO", 0.5, "Expander ratio");
    addSwitch ("gate.linear", R (251, 658, 21, 50), 2, 1, true, "LINEAR: a linear release instead of the usual exponential one");

    // SC FILTER.
    addLed ("sc.on", { 58, 777 }, "Sidechain filter on/off");
    addKnob ("sc.hpf", P (104, 847), 33, { "20", "60", "200" }, {}, "HPF", 0.0, "Sidechain high-pass: the gate and compressor ignore lows below it");
    addKnob ("sc.lpf", P (248, 847), 33, { "1k", "4.5k", "20k" }, {}, "LPF", 1.0, "Sidechain low-pass: the gate and compressor ignore highs above it");
    addButton ("sc.listen", {}, R (44, 923, 32, 38), true, "LISTEN: hear the sidechain signal")
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);
    addButton ("sc.highQ", {}, R (136, 923, 32, 38), true, "HIGH Q: resonant sidechain filters")
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);
    addButton ("sc.edge", {}, R (228, 923, 32, 38), true, "EDGE: the detector listens harder to the harsh upper mids")
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);

    // COMP.
    addLed ("comp.on", { 373, 104 }, "Compressor on/off");
    addKnob ("comp.threshold", P (409, 185), 37, { "-40", "-30", "-20", "-10", "0" }, {}, "THRESHOLD", 1.0, "Compressor threshold");
    auto& ratio = addKnob ("comp.ratio", P (556, 185), 37, {}, {}, "RATIO", 0.4,
                           "Compressor ratio. At SOFT, the ratio rises gently with the level: light on quiet parts, firmer on peaks");
    ratio.setScale ({ { 0.0f, "SOFT" }, { 0.2f, "1.5" }, { 0.4f, "2" }, { 0.6f, "4" }, { 0.8f, "10" }, { 1.0f, juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e")) } });
    addButton ("comp.vca", "VCA", R (351, 268, 64, 30), true, "VCA: clean and precise. Follows the loudness, with a near-hard knee");
    addButton ("comp.opto", "OPTO", R (417, 268, 64, 30), true, "OPTO: smooth. A wide soft knee, a gentler attack, and a release that stretches the longer it works");
    addButton ("comp.fet", "FET", R (483, 268, 64, 30), true, "FET: fast and aggressive. Catches every transient, and adds a little grit the harder it works");
    addButton ("comp.modern", "MODERN", R (549, 268, 72, 30), true,
               "MODERN: the versatile modern studio compressor. Long gentle knees at low ratios, a smooth optical feel round 10:1, "
               "near-brick-wall at the top, very fast attack, and its own distortion (the lever below)");
    addRadioGroup ({ "comp.vca", "comp.opto", "comp.fet", "comp.modern" }, 2, "comp.vca");
    addSwitch ("comp.distortion", R (430, 307, 20, 48), 3, 0, false,
               "MODERN's distortion: CLEAN, 2ND (warm, tube-like) or 2+3 (2nd and 3rd, tape-like). It grows as it compresses.");
    addKnob ("comp.attack", P (395, 384), 26, { "0.1", "1", "30" }, {}, "ATTACK", 0.5, "Compressor attack (ms)");
    addKnob ("comp.release", P (485, 384), 26, { "0.1", "1", "3" }, {}, "RELEASE", 0.5, "Compressor release (s)");
    addLed ("comp.auto", { 551, 322 }, "AUTO: programme-dependent release").setToggleState (false, juce::dontSendNotification);
    addKnob ("comp.makeup", P (575, 384), 26, { "0", "10", "20" }, {}, "MAKE UP", 0.0, "Make-up gain (dB): brings the level back up after compressing");

    // LIMIT.
    addLed ("limit.on", { 375, 538 }, "Limiter on/off");
    addKnob ("limit.threshold", P (414, 614), 35, { "-20", "-15", "-10", "-5", "0" }, {}, "THRESHOLD", 1.0, "Limiter threshold");
    addKnob ("limit.makeup", P (551, 614), 35, { "0", "3", "6", "9", "12" }, {}, "MAKE UP", 0.0, "Make-up gain");
    addButton ("limit.brick", "BRICK", R (365, 690, 78, 32), true, "BRICK: a hard ceiling. Nothing gets past the threshold.");
    addButton ("limit.soft", "SOFT", R (446, 690, 78, 32), true, "SOFT: eases in from 3 dB below the threshold and lets go more gently. Still a ceiling: nothing gets past it.");
    addButton ("limit.auto", "AUTO", R (527, 690, 78, 32), true, "AUTO: BRICK's ceiling, letting go quickly after short peaks and slowly under steady limiting");
    addRadioGroup ({ "limit.brick", "limit.soft", "limit.auto" }, 3, "limit.brick");
    addKnob ("limit.mix", P (426, 775), 27, { "0", "50", "100" }, {}, "MIX %", 1.0, "Mix: how much of the limited signal you hear");
    addButton ("dynamics.link", "LINK", R (527, 760, 76, 34), true,
               "LINK: the gate, compressor and limiter turn both channels down together, so the stereo image stays put")
        .getProperties().set (ParagonLookAndFeel::ledProperty, true);

    // SATURATION.
    addLed ("sat.on", { 375, 850 }, "Saturation on/off");
    addKnob ("sat.drive", P (401, 926), 31, {}, {}, "DRIVE", 0.0, {});
    addButton ("sat.vcaClip", "VCA CLIP", R (459, 878, 70, 30), true, {});
    addKnob ("sat.harmonics", P (579, 926), 31, {}, {}, "HARMONICS", 0.5, {});
    // MIX: a small knob under VCA CLIP, turning 9 o'clock to 3 like the Q
    // knobs, its scale round the top.
    auto& satMix = addKnob ("sat.mix", P (494, 948), 15, {}, {}, {}, 1.0, {});
    satMix.setTravel (juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::pi * 2.5f);
    satMix.addLabel (0.5f, "MIX");

    // EQ: five bands. HF, LMF and LF have a SHELF key; HMF and MF a Q knob.
    struct Band { const char* id; const char* name; std::vector<juce::String> scale; bool shelf; };
    const Band bands[] {
        { "hf",  "HF",  { "2k", "4k", "8k", "12k", "16k", "20k" }, true },
        { "hmf", "HMF", { "400", "800", "1.2k", "2.5k", "5k" }, false },
        { "mf",  "MF",  { "150", "300", "600", "1.2k", "2.5k" }, false },
        { "lmf", "LMF", { "40", "80", "150", "300", "600" }, true },
        { "lf",  "LF",  { "20", "30", "50", "80", "120", "200" }, true },
    };
    for (const auto& band : bands)
    {
        const juce::String id (band.id), name (band.name);
        const auto plate = std::find_if (modules().begin(), modules().end(), [&] (const auto& m) { return name == m.name; })->bounds;
        const auto freqY = plate.getY() + (band.shelf ? 86.0f : 95.0f);
        addLed (id + ".on", { 684.0f, plate.getY() + 22.0f }, name + " band on/off");
        addKnob (id + ".freq", P (732, freqY), 33, band.scale, "Hz", {}, 0.5, name + " frequency");
        addKnob (id + ".gain", P (857, freqY - 6.0f), 29, { "-15", "0", "+15" }, {}, "GAIN", 0.5, name + " gain, -15 to +15 dB");
        if (band.shelf)
        {
            const auto y = juce::roundToInt (freqY + 53.0f);
            addButton (id + ".shelf", "SHELF", R (672, y, 88, 22), true, name + ": shelf (lit) or bell").setToggleState (true, juce::dontSendNotification);
            addButton (id + ".shape", {}, R (776, y, 54, 22), true, name + ": shelf or bell curve")
                .getProperties().set (ParagonLookAndFeel::iconProperty, "shape");
        }
        else
        {
            // As on the concept, the Q knob's scale runs round its top: it
            // turns from 9 o'clock (0.5) to 3 o'clock (4), with "Q" at the
            // bottom right. There's no room under it.
            auto& q = addKnob (id + ".q", P (800, plate.getBottom() - 32.0f), 18, {}, {}, {}, 0.5, {});
            q.setTravel (juce::MathConstants<float>::pi * 1.5f, juce::MathConstants<float>::pi * 2.5f);
            q.addLabel (1.25f, "Q");
        }
    }

    // FILTERS / TONE.
    addLed ("tone.on", { 962, 104 }, "Console colour on/off");
    addKnob ("tone.thd", P (1021, 165), 33, { "OFF", "100" }, {}, "THD", 0.0, "THD: harmonic saturation and density");
    addKnob ("tone.vgain", P (1151, 166), 33, { "OFF", "50", "100" }, {}, "V GAIN", 0.0, "V GAIN: the console's noise floor (hiss and hum), from off to audible");
    addKnob ("tone.channel", P (1086, 290), 32, {}, {}, {}, 0.0, "Picks the console channels (one of 72 each side)");

    // The meter window.
    for (auto* meter : { &inputLeft, &inputRight, &outputLeft, &outputRight })
        addAndMakeVisible (*meter);
    const auto meterHeight = juce::roundToInt (meterBottom - meterTop);
    inputLeft.setBounds (1018, (int) meterTop, 12, meterHeight);
    inputRight.setBounds (1036, (int) meterTop, 12, meterHeight);
    outputLeft.setBounds (1112, (int) meterTop, 12, meterHeight);
    outputRight.setBounds (1130, (int) meterTop, 12, meterHeight);

    peakReset.setTooltip ("Clears the peak markers (the single lit segment above each meter's level) and OVL");

    // OVL, between the IN and OUT meters: any peak past digital full scale.
    overload.setToggleState (false, juce::dontSendNotification);
    overload.setClickingTogglesState (false);
    overload.setTooltip ("OVL: lights when the signal has gone past digital full scale (+20 on the meters). Click to clear.");
    overload.onClick = [this] { resetPeakHolds(); };
    overload.setBounds (juce::Rectangle<int> (28, 28).withCentre ({ 1080, 392 }));
    addAndMakeVisible (overload);
    startTimerHz (15);
    peakReset.onClick = [this] { resetPeakHolds(); };
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
        auto* meter = reductionMeters.add (new GainReductionMeter (processor.getCompGainChange(), GainReductionMeter::Style::gainChange));
        meter->setMakeUpSource (&processor.getCompMakeUp());
        meter->setBounds (386, 458, 204, 15);
        addAndMakeVisible (meter);
    }

    // MONO MAKER / STEREO WIDTH.
    addLed ("mono.on", { 967, 892 }, "Mono maker on/off");
    addLed ("width.on", { 1102, 892 }, "Stereo width on/off");
    addKnob ("mono.freq", P (1020, 951), 24, { "20", "100", "500" }, "Hz", {}, 0.5, "Mono maker: everything below this is folded to mono");
    addKnob ("width.amount", P (1155, 951), 24, { "0", "100", "200" }, "%", {}, 0.5, "Stereo width: 100% leaves it as it is");

    // IN / OUT: INPUT, Ø and MUTE (working).
    addLed ("output.on", { 1265, 104 }, "The output stage on/off");
    auto& input = addKnob ("output.input", P (1339, 187), 43, {}, {}, "INPUT", 0.0,
                           "INPUT: the level into the strip, -24 to +12 dB. The IN meters show the signal after it.");
    input.setScale ({ { 0.0f, "-24" }, { 0.25f, "-12" }, { 0.5f, "0" }, { 1.0f, "+12" } });
    inputAttachment = std::make_unique<SliderAttachment> (processor.getParameters(), ParamIDs::inputGain, input.slider);
    input.slider.setDoubleClickReturnValue (true, 0.0);
    input.slider.setPopupDisplayEnabled (true, true, this);   // shows "-6.0 dB" while it's turned

    auto& phase = addButton ("output.phase", {}, R (1275, 272, 115, 48), true, "Ø: flips the polarity (phase) of both channels");
    phase.getProperties().set (ParagonLookAndFeel::iconProperty, "phase");
    phaseAttachment = std::make_unique<ButtonAttachment> (processor.getParameters(), ParamIDs::phaseInvert, phase);
    auto& mute = addButton ("output.mute", "MUTE", R (1275, 329, 115, 47), true, "MUTE: silences the output");
    mute.getProperties().set (ParagonLookAndFeel::alertProperty, true);
    muteAttachment = std::make_unique<ButtonAttachment> (processor.getParameters(), ParamIDs::mute, mute);

    // STEREO MODE.
    addButton ("stereo.analog", "ANALOG", R (1275, 421, 112, 32), true, "ANALOG: the left and right sides use different console channels");
    addButton ("stereo.digital", "DIGITAL", R (1275, 459, 112, 32), true, "DIGITAL: both sides use the same console channel");
    addRadioGroup ({ "stereo.analog", "stereo.digital" }, 4, "stereo.analog");
    addButton ("stereo.one", "ONE", R (1263, 521, 64, 32), false, "ONE: picks an unused random channel for this PR7-C");
    addButton ("stereo.all", "ALL", R (1333, 521, 64, 32), false, "ALL: picks new random channels for every PR7-C in the session");

    // OUTPUT TRIM and the fader (working).
    auto& trim = addKnob ("output.trim", P (1332, 647), 34, { "-12", "0", "+12" }, {}, "OUTPUT TRIM", 0.5, "OUTPUT TRIM: fine output level, -12 to +12 dB");
    trimAttachment = std::make_unique<SliderAttachment> (processor.getParameters(), ParamIDs::outputTrim, trim.slider);
    trim.slider.setDoubleClickReturnValue (true, 0.0);
    trim.slider.setPopupDisplayEnabled (true, true, this);

    fader.getProperties().set (ParagonLookAndFeel::channelFaderProperty, true);
    fader.setTooltip ("The channel fader: the final output level, +10 dB down to silence at the bottom. Double-click for 0 dB.");
    fader.setBounds (juce::roundToInt (faderX) - 27, juce::roundToInt (faderTop) - ParagonLookAndFeel::channelFaderThumbRadius,
                     54, juce::roundToInt (faderBottom - faderTop) + ParagonLookAndFeel::channelFaderThumbRadius * 2);
    faderAttachment = std::make_unique<SliderAttachment> (processor.getParameters(), ParamIDs::outputLevel, fader);
    fader.setDoubleClickReturnValue (true, 0.0);
    fader.setPopupDisplayEnabled (true, true, this);
    fader.setVelocityModeParameters (0.25, 1, 0.0, true, juce::ModifierKeys::shiftModifier);   // Shift-drag: fine
    addAndMakeVisible (fader);
    controls["output.fader"] = &fader;

    // HPF / LPF and the EQ (step 2).
    attachButton ("hpf.on", ParamIDs::hpfOn, "HPF on/off: cuts lows below its frequency (12 dB an octave)");
    attachButton ("lpf.on", ParamIDs::lpfOn, "LPF on/off: cuts highs above its frequency (12 dB an octave)");
    attachKnob ("hpf.freq", ParamIDs::hpfFreq, { { 20.0f, "20" }, { 50.0f, "50" }, { 100.0f, "100" }, { 200.0f, "200" }, { 500.0f, "500" }, { 2000.0f, "2k" } },
                "HPF frequency: rumble and lows below it are cut");
    attachKnob ("lpf.freq", ParamIDs::lpfFreq, { { 1000.0f, "1k" }, { 2000.0f, "2k" }, { 5000.0f, "5k" }, { 10000.0f, "10k" }, { 20000.0f, "20k" } },
                "LPF frequency: fizz and highs above it are cut");
    attachSwitch ("filters.placement", ParamIDs::filterPlacement,
                  "Where the HPF / LPF sit. IN: at the input, before everything. POST: after the compressor and EQ, before the limiter. "
                  "SC: only in what the gate and compressor listen to, so they react less to lows or highs; the audio itself isn't filtered.");

    const std::map<juce::String, std::vector<std::pair<float, juce::String>>> frequencyMarks {
        { "hf",  { { 2000.0f, "2k" }, { 3000.0f, "3k" }, { 5000.0f, "5k" }, { 8000.0f, "8k" }, { 12000.0f, "12k" }, { 20000.0f, "20k" } } },
        { "hmf", { { 400.0f, "400" }, { 800.0f, "800" }, { 1500.0f, "1.5k" }, { 3000.0f, "3k" }, { 5000.0f, "5k" } } },
        { "mf",  { { 150.0f, "150" }, { 300.0f, "300" }, { 600.0f, "600" }, { 1200.0f, "1.2k" }, { 2500.0f, "2.5k" } } },
        { "lmf", { { 40.0f, "40" }, { 80.0f, "80" }, { 150.0f, "150" }, { 300.0f, "300" }, { 600.0f, "600" } } },
        { "lf",  { { 20.0f, "20" }, { 30.0f, "30" }, { 50.0f, "50" }, { 80.0f, "80" }, { 120.0f, "120" }, { 200.0f, "200" } } },
    };
    for (const auto& band : ParamRanges::eqBands)
    {
        const juce::String id (band.id), name (band.name);
        attachButton (id + ".on", ParamIDs::eqOn (id), name + " band on/off");
        attachKnob (id + ".freq", ParamIDs::eqFreq (id), frequencyMarks.at (id), name + " frequency");
        attachKnob (id + ".gain", ParamIDs::eqGain (id), { { -15.0f, "-15" }, { 0.0f, "0" }, { 15.0f, "+15" } },
                    name + " gain, -15 to +15 dB. Bells narrow as you boost or cut harder, like a console EQ's.");
        if (band.hasQ)
        {
            attachKnob (id + ".q", ParamIDs::eqQ (id), { { 0.5f, "0.5" }, { 1.0f, "1" }, { 2.0f, "2" }, { 4.0f, "4" } },
                        name + " Q: the width of the bell. Low is wide and gentle, high is narrow");
        }
        else
        {
            const auto shelfTip = name + (juce::String (band.id) == "hf" ? ": SHELF lifts or cuts everything above the frequency; off, it's a bell"
                                                                         : ": SHELF lifts or cuts everything below the frequency; off, it's a bell");
            attachButton (id + ".shelf", ParamIDs::eqShelf (id), shelfTip);
            attachButton (id + ".shape", ParamIDs::eqShelf (id), shelfTip);   // the same switch, showing the curve
        }
    }

    // COMP and LINK (step 3).
    const auto infinity = juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x9e"));
    attachButton ("comp.on", ParamIDs::compOn, "Compressor on/off");
    attachKnob ("comp.threshold", ParamIDs::compThreshold,
                { { -50.0f, "-50" }, { -40.0f, "-40" }, { -30.0f, "-30" }, { -20.0f, "-20" }, { -10.0f, "-10" }, { 0.0f, "0" } },
                "Threshold (dBFS): the level above which it starts turning the sound down");
    attachKnob ("comp.ratio", ParamIDs::compRatio,
                { { 1.0f, "SOFT" }, { 1.5f, "1.5" }, { 2.0f, "2" }, { 4.0f, "4" }, { 10.0f, "10" }, { 30.0f, infinity } },
                "Ratio: how hard it turns down what's over the threshold. SOFT rises with level: gentle on quiet "
                "parts (about 3:1), firmer on peaks (6:1 and more). At the top it's a limiter.");
    attachChoice ({ "comp.vca", "comp.opto", "comp.fet", "comp.modern" }, ParamIDs::compCharacter);
    attachSwitch ("comp.distortion", ParamIDs::compDistortion,
                  "MODERN's distortion (MODERN only). CLEAN: none. 2ND: warm, mostly 2nd harmonic, like a tube. "
                  "2+3: 2nd and 3rd, rounding the peaks like tape. A little at all times, more the harder it compresses.");
    attachKnob ("comp.attack", ParamIDs::compAttack, { { 0.1f, "0.1" }, { 1.0f, "1" }, { 30.0f, "30" } },
                "Attack (ms): how quickly it clamps down. Slower lets the start of each note punch through.");
    attachKnob ("comp.release", ParamIDs::compRelease, { { 100.0f, "0.1" }, { 1000.0f, "1" }, { 3000.0f, "3" } },
                "Release (s): how quickly it lets go");
    attachButton ("comp.auto", ParamIDs::compAuto,
                  "AUTO release: lets go quickly after a short peak and slowly after long compression, so it doesn't pump");
    attachKnob ("comp.makeup", ParamIDs::compMakeUp, { { 0.0f, "0" }, { 10.0f, "10" }, { 20.0f, "20" } },
                "Make up (dB): brings the level back up after compressing");
    attachButton ("dynamics.link", ParamIDs::dynamicsLink,
                  "LINK: the gate, compressor and limiter turn both channels down together, from the louder one, so the stereo image stays put");

    // GATE / EXP and SC FILTER (step 5).
    attachButton ("gate.on", ParamIDs::gateOn, "Gate / expander on/off");
    attachKnob ("gate.threshold", ParamIDs::gateThreshold, { { -60.0f, "-60" }, { -40.0f, "-40" }, { -20.0f, "-20" }, { 0.0f, "0" } },
                "Threshold (dBFS): below this the gate closes (or the expander turns down). With -30, the whole range is 30 dB lower.");
    attachKnob ("gate.range", ParamIDs::gateRange, { { 0.0f, "0" }, { 20.0f, "20" }, { 40.0f, "40" }, { 60.0f, "60" }, { 80.0f, "80" } },
                "Range (dB): how far it turns down when closed. 0 does nothing; small amounts just tidy up.");
    attachButton ("gate.minus30", ParamIDs::gateMinus30, "-30: shifts the threshold 30 dB lower (-90 to -30 dBFS), for quiet material");
    attachButton ("gate.exp", ParamIDs::gateExpander,
                  "EXP: an expander instead of a gate. Quiet parts are turned down gradually, by the ratio, rather than shut off.");
    attachChoice ({ "gate.fast", "gate.med", "gate.slow" }, ParamIDs::gateAttack);
    attachKnob ("gate.hold", ParamIDs::gateHold, { { 0.0f, "0" }, { 50.0f, "50" }, { 500.0f, "500" } },
                "Hold (ms): how long it stays open after the sound drops, before it starts to close. Stops it chattering between hits.");
    attachKnob ("gate.release", ParamIDs::gateRelease, { { 5.0f, "5" }, { 50.0f, "50" }, { 500.0f, "500" }, { 3000.0f, "3s" } },
                "Release (ms): how long it takes to close");
    attachKnob ("gate.ratio", ParamIDs::gateRatio, { { 1.0f, "1:1" }, { 1.5f, "1.5:1" }, { 3.0f, "3:1" }, { 6.0f, "6:1" }, { 20.0f, "20:1" } },
                "Ratio (EXP only): below the threshold, each dB quieter is turned down this much more");
    attachSwitch ("gate.linear", ParamIDs::gateLinear,
                  "LINEAR (up): closes at a steady rate. Down: the usual curve, quick at first and then easing off.", true);

    attachButton ("sc.on", ParamIDs::scOn, "SC FILTER on/off: filters what the gate and compressor listen to. The audio isn't filtered.");
    attachKnob ("sc.hpf", ParamIDs::scHpf, { { 20.0f, "20" }, { 60.0f, "60" }, { 200.0f, "200" } },
                "Sidechain HPF: the gate and compressor ignore lows below it (so a bass or kick doesn't set them off)");
    attachKnob ("sc.lpf", ParamIDs::scLpf, { { 1000.0f, "1k" }, { 4500.0f, "4.5k" }, { 20000.0f, "20k" } },
                "Sidechain LPF: the gate and compressor ignore highs above it (so cymbals don't set them off)");
    attachButton ("sc.listen", ParamIDs::scListen, "LISTEN: hear what the gate and compressor hear, to set the filters. Turn it off to hear the music again.");
    attachButton ("sc.highQ", ParamIDs::scHighQ, "HIGH Q: resonant sidechain filters, focusing the gate and compressor on the frequencies at the cutoffs");
    attachButton ("sc.edge", ParamIDs::scEdge,
                  "EDGE: the gate and compressor listen harder to the harsh upper mids (around 6 kHz), so they clamp down on a piercing vocal or an edgy guitar. "
                  "The audio isn't filtered. Works while SC FILTER is on.");

    // SATURATION (step 6).
    attachButton ("sat.on", ParamIDs::satOn, "Saturation on/off");
    attachKnob ("sat.drive", ParamIDs::satDrive, { { 0.0f, "0" }, { 12.0f, "12" }, { 24.0f, "24" } },
                "Drive (dB): how hard it's pushed into the saturation. The level stays the same; the colour changes.");
    attachButton ("sat.vcaClip", ParamIDs::satVcaClip, "VCA CLIP: flattens the loudest peaks much harder before the saturation, for crunch");
    attachKnob ("sat.harmonics", ParamIDs::satHarmonics, { { 0.0f, "2nd" }, { 100.0f, "3rd" } },
                "Harmonics: left, 2nd-harmonic warmth (valve-like); right, 3rd-harmonic grit (transistor-like); in between, a blend");
    attachKnob ("sat.mix", ParamIDs::satMix, { { 0.0f, "0" }, { 100.0f, "100" } },
                "Mix: the saturated sound blended with the dry, for parallel saturation");

    // FILTERS / TONE: the console colour (step 6).
    attachButton ("tone.on", ParamIDs::toneOn,
                  "Console colour on/off: THD, the noise floor and the console channels' small differences. Off, both sides are the exact design.");
    attachKnob ("tone.thd", ParamIDs::toneThd, { { 0.0f, "OFF" }, { 100.0f, "100" } },
                "THD: the console's own harmonics and density. Subtle at normal levels, more as you run it hot. The level stays the same.");
    attachKnob ("tone.vgain", ParamIDs::toneVGain, { { 0.0f, "OFF" }, { 50.0f, "50" }, { 100.0f, "100" } },
                "V GAIN: the console's noise floor, hiss and a little mains hum, from off to audible");
    attachKnob ("tone.channel", ParamIDs::toneChannel, {},
                "Console channel: each of the 72 has its own small differences in the EQ, filters, THD and noise. "
                "LEFT shows the left side's channel; the right side uses the next one (ANALOG) or the same one (DIGITAL). Channel 1 is the exact design.");
    {
        auto* parameter = processor.getParameters().getParameter (ParamIDs::toneChannel);
        auto attachment = std::make_unique<juce::ParameterAttachment> (*parameter, [this] (float value)
        {
            channelNumber = juce::jlimit (1, 72, juce::roundToInt (value));
            repaint (juce::Rectangle<int> (960, 250, 250, 66));
        });
        attachment->sendInitialUpdate();
        switchAttachments.push_back (std::move (attachment));
    }

    // MONO MAKER / STEREO WIDTH and the STEREO MODE (step 7).
    attachButton ("mono.on", ParamIDs::monoOn, "Mono maker on/off: folds the lows to mono, for a solid centre and tight low end");
    attachKnob ("mono.freq", ParamIDs::monoFreq, { { 20.0f, "20" }, { 100.0f, "100" }, { 500.0f, "500" } },
                "Mono maker frequency (Hz): everything below it is folded to mono. Above it, the stereo image is untouched.");
    attachButton ("width.on", ParamIDs::widthOn, "Stereo width on/off");
    attachKnob ("width.amount", ParamIDs::width, { { 0.0f, "0" }, { 100.0f, "100" }, { 200.0f, "200" } },
                "Stereo width (%): 0 is mono, 100 as it was, 200 twice as wide. The centre and the mono sum stay the same at any width.");
    attachChoice ({ "stereo.analog", "stereo.digital" }, ParamIDs::stereoMode);
    if (auto* analog = dynamic_cast<juce::Button*> (findControl ("stereo.analog")))
        analog->setTooltip ("ANALOG: the right side uses the console channel next to the left's, so the two sides differ slightly, as on a real console");
    if (auto* digital = dynamic_cast<juce::Button*> (findControl ("stereo.digital")))
        digital->setTooltip ("DIGITAL: both sides use the left side's console channel, so they're treated identically");
    {
        auto* parameter = processor.getParameters().getParameter (ParamIDs::stereoMode);
        auto attachment = std::make_unique<juce::ParameterAttachment> (*parameter, [this] (float value)
        {
            analogMode = juce::roundToInt (value) == 0;
            repaint (juce::Rectangle<int> (960, 250, 250, 66));
        });
        attachment->sendInitialUpdate();
        switchAttachments.push_back (std::move (attachment));
    }
    for (const auto& [id, tip] : { std::pair ("stereo.one", "ONE: moves this PR7-C to a random console channel that no other PR7-C in the session is using"),
                                   { "stereo.all", "ALL: gives every PR7-C in the session new random console channels, each its own, as tracks on a real console" } })
    {
        auto* button = dynamic_cast<juce::Button*> (findControl (id));
        button->setTooltip (tip);
        button->onClick = juce::String (id) == "stereo.one" ? std::function<void()> ([this] { ChannelRegistry::rerollOne (processor); })
                                                            : std::function<void()> ([] { ChannelRegistry::rerollAll(); });
    }
    if (auto* lamp = dynamic_cast<juce::Button*> (findControl ("output.on")))
    {
        // The output stage can't be bypassed: its lamp just shows it's live.
        lamp->setClickingTogglesState (false);
        lamp->setToggleState (true, juce::dontSendNotification);
        lamp->setTooltip ("The output stage: INPUT, Ø, MUTE, the stereo mode, OUTPUT TRIM and the fader. Always on.");
    }

    // LIMIT (step 4).
    attachButton ("limit.on", ParamIDs::limitOn, "Limiter on/off. A peak limiter with no look-ahead, so it adds no delay.");
    attachKnob ("limit.threshold", ParamIDs::limitThreshold,
                { { -20.0f, "-20" }, { -15.0f, "-15" }, { -10.0f, "-10" }, { -5.0f, "-5" }, { 0.0f, "0" } },
                "Threshold (dBFS): the ceiling. Nothing gets past it.");
    attachKnob ("limit.makeup", ParamIDs::limitMakeUp, { { 0.0f, "0" }, { 3.0f, "3" }, { 6.0f, "6" }, { 9.0f, "9" }, { 12.0f, "12" } },
                "Make up (dB): gain after limiting. The ceiling rises with it.");
    attachChoice ({ "limit.brick", "limit.soft", "limit.auto" }, ParamIDs::limitMode);
    attachKnob ("limit.mix", ParamIDs::limitMix, { { 0.0f, "0" }, { 50.0f, "50" }, { 100.0f, "100" } },
                "Mix: the limited sound blended with the dry. Below 100%, some of the original peaks come back through.");

    // The header.
    addButton ("preset.previous", {}, R (522, 24, 32, 32), false, "The previous preset")
        .getProperties().set (ParagonLookAndFeel::iconProperty, "left");
    addButton ("preset.next", {}, R (894, 24, 32, 32), false, "The next preset")
        .getProperties().set (ParagonLookAndFeel::iconProperty, "right");
    for (const auto* id : { "preset.previous", "preset.next" })
        findControl (id)->getProperties().set (ParagonLookAndFeel::bareProperty, true);
    dynamic_cast<juce::Button*> (findControl ("preset.previous"))->onClick = [this] { processor.getPresets().step (-1); };
    dynamic_cast<juce::Button*> (findControl ("preset.next"))->onClick = [this] { processor.getPresets().step (1); };
    addButton ("preset.save", "SAVE", R (1203, 22, 96, 36), false, "Save the settings as a user preset").onClick = [this] { askToSavePreset(); };
    auto& presetMenu = addButton ("preset.menu", {}, R (1314, 22, 56, 36), false, "More preset options: back to the defaults, delete a user preset, find the presets folder");
    presetMenu.getProperties().set (ParagonLookAndFeel::iconProperty, "dots");
    presetMenu.onClick = [this] { showPresetMenu(); };
    auto& settings = addButton ("settings", {}, R (1382, 20, 40, 40), false, "Settings: the panel's finish");
    settings.getProperties().set (ParagonLookAndFeel::iconProperty, "gear");
    settings.getProperties().set (ParagonLookAndFeel::bareProperty, true);
    settings.onClick = [this] { showSettingsMenu(); };

    grain = Finish::makeSatinGrain (width, height, 7);
    Finish::changes().addChangeListener (this);
}

void MainView::resetPeakHolds()
{
    for (auto* meter : getLevelMeters())
        meter->resetPeakHold();
    overload.setToggleState (false, juce::dontSendNotification);
}

void MainView::timerCallback()
{
    // The preset display follows loads, saves and changes made since.
    if (const auto text = presetDisplayText(); text != shownPreset)
    {
        shownPreset = text;
        repaint (presetDisplay.toNearestInt());
    }

    const auto meters = getLevelMeters();
    if (std::any_of (meters.begin(), meters.end(), [] (auto* meter) { return meter->hasOverloaded(); }))
        overload.setToggleState (true, juce::dontSendNotification);
}

MainView::~MainView()
{
    Finish::changes().removeChangeListener (this);
}

void MainView::attachKnob (const juce::String& controlId, const juce::String& parameterId,
                          std::vector<std::pair<float, juce::String>> marks, const juce::String& tooltip)
{
    auto* knob = dynamic_cast<Knob*> (findControl (controlId));
    auto* parameter = processor.getParameters().getParameter (parameterId);
    jassert (knob != nullptr && parameter != nullptr);

    for (auto& mark : marks)
        mark.first = parameter->convertTo0to1 (mark.first);
    knob->setScale (std::move (marks));
    knobAttachments.push_back (std::make_unique<SliderAttachment> (processor.getParameters(), parameterId, knob->slider));
    knob->slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    knob->slider.setPopupDisplayEnabled (true, true, this);
    knob->slider.setTooltip (tooltip);
}

void MainView::attachButton (const juce::String& controlId, const juce::String& parameterId, const juce::String& tooltip)
{
    auto* button = dynamic_cast<juce::Button*> (findControl (controlId));
    jassert (button != nullptr);
    button->setClickingTogglesState (true);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (processor.getParameters(), parameterId, *button));
    if (auto* client = dynamic_cast<juce::SettableTooltipClient*> (button))
        client->setTooltip (tooltip);
}

void MainView::attachSwitch (const juce::String& controlId, const juce::String& parameterId, const juce::String& tooltip, bool upIsOn)
{
    auto* lever = dynamic_cast<LeverSwitch*> (findControl (controlId));
    auto* parameter = processor.getParameters().getParameter (parameterId);
    jassert (lever != nullptr && parameter != nullptr);
    auto attachment = std::make_unique<juce::ParameterAttachment> (*parameter, [lever, upIsOn] (float value)
    {
        lever->setPosition (upIsOn ? (value >= 0.5f ? 0 : 1) : juce::roundToInt (value));
    });
    lever->onChange = [lever, upIsOn, raw = attachment.get()]
    {
        raw->setValueAsCompleteGesture (upIsOn ? (lever->getPosition() == 0 ? 1.0f : 0.0f) : (float) lever->getPosition());
    };
    attachment->sendInitialUpdate();
    switchAttachments.push_back (std::move (attachment));
    lever->setTooltip (tooltip);
}

void MainView::attachChoice (std::initializer_list<const char*> controlIds, const juce::String& parameterId)
{
    auto* parameter = processor.getParameters().getParameter (parameterId);
    jassert (parameter != nullptr);
    std::vector<juce::Button*> keys;
    for (const auto* id : controlIds)
        keys.push_back (dynamic_cast<juce::Button*> (findControl (id)));

    auto attachment = std::make_unique<juce::ParameterAttachment> (*parameter, [keys] (float value)
    {
        for (size_t i = 0; i < keys.size(); ++i)
            keys[i]->setToggleState ((int) i == juce::roundToInt (value), juce::dontSendNotification);
    });
    for (size_t i = 0; i < keys.size(); ++i)
        keys[i]->onClick = [raw = attachment.get(), i] { raw->setValueAsCompleteGesture ((float) i); };
    attachment->sendInitialUpdate();
    switchAttachments.push_back (std::move (attachment));
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

juce::String MainView::presetDisplayText() const
{
    auto& presets = processor.getPresets();
    const auto name = presets.getCurrentName();
    if (name.isEmpty())
        return "Untitled";
    return presets.isModified() ? name + " *" : name;   // * : changed since it was loaded
}

void MainView::mouseDown (const juce::MouseEvent& event)
{
    // Clicking the preset's name lists them all.
    if (presetDisplay.contains (event.position))
        showPresetList();
}

void MainView::showPresetList()
{
    auto& presets = processor.getPresets();
    const auto list = presets.getPresets();
    const auto* current = presets.findCurrent (list);
    juce::PopupMenu menu;
    bool userHeader = false;
    menu.addSectionHeader ("Factory");
    for (size_t i = 0; i < list.size(); ++i)
    {
        if (! list[i].factory && ! userHeader)
        {
            menu.addSectionHeader ("Yours");
            userHeader = true;
        }
        menu.addItem ((int) i + 1, list[i].name, true, &list[i] == current);
    }
    if (! userHeader)
        menu.addItem (-1, "(Your presets appear here once you SAVE one)", false);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (findControl ("preset.next")).withTargetScreenArea (
                            localAreaToGlobal (presetDisplay.toNearestInt())),
                        [this, list] (int chosen)
                        {
                            if (juce::isPositiveAndNotGreaterThan (chosen, (int) list.size()))
                                processor.getPresets().load (list[(size_t) chosen - 1]);
                        });
}

void MainView::showPresetMenu()
{
    auto& presets = processor.getPresets();
    const auto list = presets.getPresets();
    const auto* current = presets.findCurrent (list);
    const auto deletable = current != nullptr && ! current->factory;
    juce::PopupMenu menu;
    menu.addItem (1, "Back to the default settings");
    menu.addItem (2, deletable ? "Delete \"" + current->name + "\"" : juce::String ("Delete (only your own presets)"), deletable);
    menu.addItem (3, "Show the presets folder");
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (findControl ("preset.menu")),
                        [this, toDelete = deletable ? *current : PresetManager::Preset()] (int chosen)
                        {
                            auto& p = processor.getPresets();
                            if (chosen == 1)
                                p.loadFactory (0);
                            else if (chosen == 2)
                                juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                                                  .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                                                  .withTitle ("Delete preset")
                                                                  .withMessage ("Delete \"" + toDelete.name + "\"? This can't be undone.")
                                                                  .withButton ("Delete").withButton ("Cancel").withAssociatedComponent (this),
                                                              [this, toDelete] (int result)
                                                              {
                                                                  if (result == 1)
                                                                      processor.getPresets().remove (toDelete);
                                                              });
                            else if (chosen == 3)
                            {
                                p.getUserFolder().createDirectory();
                                p.getUserFolder().startAsProcess();
                            }
                        });
}

void MainView::askToSavePreset()
{
    auto& presets = processor.getPresets();
    const auto list = presets.getPresets();
    const auto* current = presets.findCurrent (list);
    saveDialog = std::make_unique<juce::AlertWindow> ("Save preset", "A name for your preset:", juce::MessageBoxIconType::NoIcon, this);
    saveDialog->addTextEditor ("name", current != nullptr && ! current->factory ? current->name : juce::String());
    saveDialog->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveDialog->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveDialog->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        const auto name = saveDialog->getTextEditorContents ("name").trim();
        saveDialog.reset();
        if (result == 1 && name.isNotEmpty())
            savePreset (name);
    }), false);
}

void MainView::savePreset (const juce::String& name)
{
    auto& presets = processor.getPresets();
    const auto list = presets.getPresets();
    const auto clash = std::find_if (list.begin(), list.end(), [&] (const auto& p) { return p.name.equalsIgnoreCase (name); });
    const auto tell = [this] (const juce::String& message)
    {
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::WarningIcon).withTitle ("Save preset")
                                          .withMessage (message).withButton ("OK").withAssociatedComponent (this), nullptr);
    };
    if (clash != list.end() && clash->factory)
        return tell ("\"" + clash->name + "\" is a factory preset. Please pick another name.");
    if (clash != list.end() && clash->name != presets.getCurrentName())
    {
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon).withTitle ("Save preset")
                                          .withMessage ("You already have a preset called \"" + clash->name + "\". Replace it?")
                                          .withButton ("Replace").withButton ("Cancel").withAssociatedComponent (this),
                                      [this, name, tell] (int result)
                                      {
                                          if (result == 1 && ! processor.getPresets().save (name))
                                              tell ("The preset couldn't be saved.");
                                      });
        return;
    }
    if (! presets.save (name))
        tell ("The preset couldn't be saved.");
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

        g.setColour (juce::Colour (0xff0a090b));
        g.fillRoundedRectangle (presetDisplay, 6.0f);
        g.setColour (ParagonColours::panelEdge);
        g.drawRoundedRectangle (presetDisplay, 6.0f, 1.0f);
        g.setColour (juce::Colour (0xffffb04a));
        g.setFont (juce::FontOptions (19.0f));
        g.drawFittedText (presetDisplayText(), presetDisplay.reduced (48.0f, 0.0f).toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }

    // The body: a dark frame round the modules, edged in red-orange light.
    {
        const juce::Rectangle<float> body { 13.0f, 71.0f, 1422.0f, 934.0f };
        g.setColour (juce::Colour (0xff070507));
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
    for (const auto& module : modules())
        if (juce::StringArray { "HF", "HMF", "MF", "LMF", "LF" }.contains (module.name))
            title (module.name, 704.0f, module.bounds.getY() + 22.0f);
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
    drawStamped (g, "LISTEN", { 82.0f, 935.0f, 56.0f, 16.0f }, 12.5f, juce::Justification::centredLeft);
    drawStamped (g, "HIGH Q", { 174.0f, 935.0f, 56.0f, 16.0f }, 12.5f, juce::Justification::centredLeft);
    drawStamped (g, "EDGE", { 266.0f, 935.0f, 50.0f, 16.0f }, 12.5f, juce::Justification::centredLeft);
    drawStamped (g, "AUTO", { 566.0f, 315.0f, 40.0f, 14.0f }, 11.0f, juce::Justification::centredLeft);
    drawStamped (g, "CLEAN", { 455.0f, 308.0f, 50.0f, 13.0f }, 10.5f, juce::Justification::centredLeft);
    drawStamped (g, "2ND", { 455.0f, 324.0f, 50.0f, 13.0f }, 10.5f, juce::Justification::centredLeft);
    drawStamped (g, "2+3", { 455.0f, 340.0f, 50.0f, 13.0f }, 10.5f, juce::Justification::centredLeft);
    drawStamped (g, "LEFT", { 965.0f, 318.0f, 77.0f, 16.0f }, 13.0f);
    drawStamped (g, "RIGHT", { 1129.0f, 318.0f, 77.0f, 16.0f }, 13.0f);
    drawStamped (g, "RANDOM CH.", { 1235.0f, 500.0f, 190.0f, 16.0f }, 12.5f);

    // The keys' trays.
    drawTray (g, { 347.0f, 265.0f, 278.0f, 36.0f });
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

        const auto twoDigits = [] (int number) { return juce::String (number).paddedLeft ('0', 2); };
        const auto left = twoDigits (channelNumber), right = twoDigits (analogMode ? channelNumber % 72 + 1 : channelNumber);
        for (const auto& [area, number] : { std::pair (juce::Rectangle<float> (965.0f, 256.0f, 77.0f, 56.0f), left),
                                            { juce::Rectangle<float> (1129.0f, 256.0f, 77.0f, 56.0f), right } })
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

        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("OVL", juce::Rectangle<float> (40.0f, 12.0f).withCentre ({ 1080.0f, 412.0f }), juce::Justification::centred, false);

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
            // The bottom of the fader is silence.
            const auto label = i + 1 == (int) std::size (ParamRanges::faderMarksDb) ? juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e"))
                                                                                     : juce::String (juce::roundToInt (db));
            drawStamped (g, label, { 1244.0f, y - 8.0f, 42.0f, 16.0f }, 13.0f, juce::Justification::centredRight);

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
