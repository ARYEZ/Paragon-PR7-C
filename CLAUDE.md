# Paragon PR7-C: notes for Claude

**Paragon PR7-C** is a console **channel strip** plugin (AGPLv3, JUCE, VST3). It's the fourth member of the Paragon PR7 family:

| Plugin | Repo |
|---|---|
| PR7-G (guitar) | `ARYEZ/Paragon-X1` |
| PR7-B (bass) | `ARYEZ/Paragon-PR7-B` |
| PR7-V (vocals) | `ARYEZ/Paragon-PR7-V` |

Unlike PR7-B and PR7-V, it is **not** a copy of PR7-G. It was seeded lean: PR7-G's build, release and test setup, its look-and-feel pieces, and an empty true-stereo processor. Read this file, then look at **`docs/concept.png`**, the owner's concept design. The UI is built to match it.

## What's here

- **The plugin:** true stereo (stereo in/out, or mono in/out on a mono track).
  - Its own identity: `Paragon PR7-C`, plugin code `Pr7c`, bundle `com.paragon.paragonpr7c`, maker code `Prgn`.
  - Parameters: `inputGain`, `outputTrim`, `outputLevel` (the fader), `phaseInvert`, `mute`; the HPF / LPF (`hpfOn`, `hpfFreq`, `lpfOn`, `lpfFreq`, `filterPlacement`); the EQ (`<band>On/Freq/Gain` plus `<band>Q` or `<band>Shelf`, bands from `ParamRanges::eqBands`); the gate (`gate*`), SC FILTER (`sc*`, incl. `scEdge`), compressor (`comp*`, incl. `compDistortion`), limiter (`limit*`), saturation (`sat*`), the console colour (`toneOn`, `toneThd`, `toneVGain`, `toneChannel`), the stereo tools (`monoOn`, `monoFreq`, `widthOn`, `width`, `stereoMode`) and `dynamicsLink`. Input and output peak meters (console-calibrated, with OVL), and gain-reduction sources (`getGainReduction`) for the GR meters (gate, COMP, LIMIT, per side).
- **`source/ui/MainView`:** a fixed **1448 × 1086** canvas, exactly the concept image's size, so coordinates can be read straight off `docs/concept.png`. The editor scales it to the window.
  - Every module's panel is drawn with all its controls, placed as on the concept, and **every control works** (a UI test checks each has a real tooltip). `MainView::modules()` holds each module's rectangle.
  - Controls are registered by ID (`"comp.ratio"`, `"hf.on"`...; see `MainView.cpp`), then attached to parameters with `attachKnob` (prints the scale at the true values and turns on the value popup), `attachButton`, `attachSwitch` and `attachChoice`. A new control needs its own tooltip.
- **`source/dsp/`:** `Biquad` (cookbook filters), `ChannelEq` (`EqBand`, `CutFilters`), `Compressor`, `Limiter`, `Gate`, `Saturation`, `ConsoleColour` (THD and noise), `ConsoleChannel` (the 72 channels' tolerances) and `StereoTools` (mono maker, width).
  - `source/ChannelRegistry`: every PR7-C in the process, for RANDOM CH. (message thread only).
- **`source/FactoryPresets`:** the 162 factory presets, filed as PR7-X (`ARYEZ/Paragon-PR7-X`) files its rigs (the owner's ask): banks A drums, B bass, C guitars, D synths & keys, E vocals & voice, F mix, G mastering; 01–10 single presets; 11–19 the nine genre sets and 31–38 the eight signature-sound sets (PR7-X's genres and signature sounds), one preset per part (vocal, drums, guitars, bass, mix, master) at the same number in each bank but D. Sets are built from a few style traits per genre (aggression, brightness, weight, scoop, grit, harmonics, loudness, width, vocal and drum compressor characters) through one recipe per part. Our reading, never tuned by ear. A test checks the filing.
- **`source/PresetManager`:** factory presets (from `FactoryPresets`) and user presets (XML `.pr7cpreset` files in Documents/Paragon PR7-C/Presets). The processor owns one (`getPresets()`); the current name lives in the state (`presetName`).
  - The HPF / LPF and EQ run one instance per side (`processFilters`, `processEq`), as each side can be a different console channel.
  - Modules jump straight to their settings on the first block after `prepare`/`reset` (`fresh`), and glide or fade only for changes made while playing.
  - The processor runs the modules in stretches no longer than the prepared block size (`processModules`), as their working buffers are that size. The gate and compressor share one detector signal, `sidechain`: the audio at the gate's input, through the HPF / LPF when they're on SC, then the SC FILTER. LISTEN crossfades the strip's output to it.
- **`PARAGON_SNAPSHOT_DEMO=1`** switches modules on for a render that shows them working (the expander, compressor, limiter, saturation, console colour and stereo tools).
  - `source/ui/Controls`: `Knob` (draws itself; its slider only takes the mouse over the knob), `Led`, `LeverSwitch`, `RoundButton`, `GainReductionMeter`. Buttons are `TextButton`s with `ParagonLookAndFeel::consoleButtonProperty`.
- **Carried over from PR7-G:**
  - `ParagonLookAndFeel` (Inter fonts, knobs, LED/neon bars, piano keys, chrome bezels).
  - `Finish` (Pearl White / White + Neon / Pearl Black finishes and the satin grain). Its settings file is "Paragon PR7-C".
  - `LedDisplay`: static drawing functions only (`paintScreen`, `paintGlassOverlay`, `drawDigit`), for the channel-number displays. PR7-G's rig/tuner screen was removed.
  - `Logo` (the PR7 mark, now "-C").
  - `LevelMeter` / `LevelMeterSource`.
  - `dsp/EffectUtils.h` (`BypassFade` etc.).
- **`reference/pr7g/`:** PR7-G's compressor (Daybreak), graphic EQ and drive code. **Not compiled.** See `reference/README.md`.

## The modules (from the concept)

The **order is fixed**: the owner decided the user can't reorder modules. The signal flows:

**input → HPF/LPF → GATE/EXP → COMP → EQ (HF, HMF, MF, LMF, LF) → LIMIT → SATURATION → FILTERS/TONE colour → MONO MAKER / STEREO WIDTH → OUTPUT**

The **SC FILTER** shapes only the dynamics' **sidechain** (detector), never the audio. Every module has an **on LED** in its header.

The behaviour notes below are **modelled on** a classic 1970s/80s British console channel and mastering compressor (the owner supplied a commercial plugin manual as reference). They're described in our own words. **Never use that product's names or trademarks** (see "Names" below).

1. **HPF / LPF. (Built, step 2.)** HPF 20 Hz–2 kHz, LPF 1–20 kHz, 12 dB/octave Butterworth, both off by default. Each fades in/out over 20 ms (on/off and placement changes). In SC they leave the audio alone and filter the dynamics' detector instead (`filterSettings` in the processor). Each side runs its own filters, with its console channel's corners.
   - A three-way switch **IN / POST / SC** places them:
     - **IN:** at the input, before everything.
     - **POST:** after the compressor, before the limiter.
     - **SC:** in the dynamics' sidechain only.
   - That's a routing choice inside the fixed order, not reordering.
2. **GATE / EXP. (Built, step 5.)** Off by default. THRESHOLD −60 to 0 dBFS (**−30** shifts it to −90 to −30); RANGE 0–80 dB; a peak detector riding over the waveform (10 ms fall).
   - **Gate** (default): open/closed with 3 dB hysteresis; closed = −RANGE. **EXP**: below the threshold, (ratio − 1) dB more per dB, at most RANGE; RATIO 1:1–20:1 (`gateRatioMarks`).
   - **FAST / MED / SLOW** = attack 0.1 / 1 / 10 ms (our times). **HOLD** 0–500 ms (it starts held open at playback start). **RELEASE** 5 ms–3 s: exponential (time constant release / 3, so ~95% in RELEASE) or **LINEAR** (the lever up: RANGE dB in RELEASE). LINK shared.
3. **SC FILTER. (Built, step 5.)** Off by default. HPF 20–200 Hz (default 60) and LPF 1–20 kHz on the shared detector signal (gate **and** compressor), never the audio. **HIGH Q**: resonance Q 2.5 instead of 0.707. **EDGE** (`scEdge`, owner's request): a +8 dB bell at 6 kHz in the detector (an `EqBand`, so it glides and switches itself off), while SC FILTER is on, so the dynamics clamp down on harsh upper mids. **LISTEN**: the output becomes the detector signal (20 ms crossfade).
4. **COMP. (Built, step 3; all of it our design.)** Off by default. THRESHOLD -50 to 0 dBFS; RATIO SOFT, 1.5:1 to ∞ (marks evenly round the knob, `compRatioMarks`); ATTACK 0.1–30 ms; RELEASE 50 ms–3 s; AUTO; MAKE UP 0–20 dB; LINK (`dynamicsLink`, on by default) uses the louder channel's level for both.
   - **SOFT:** the output over the threshold is 4·over/(4+over): about 3:1 at 8 dB over, 6:1 at 20, 10:1 at 36, a smooth knee built in.
   - **VCA:** RMS detector (5 ms), 1 dB knee. **OPTO:** RMS (20 ms), 12 dB knee, attack ×2, stretching release. **FET:** peak detector, 3 dB knee, third-harmonic grit ∝ gain reduction (about 1% at 20 dB), measured against the signal's own peak so it doesn't depend on level. The grit (a gentle cubic) is computed at 2× the rate on a copy of the compressed signal and only the grit is added back (`addGrit`), so VCA / OPTO are untouched and switching character shifts nothing; the grit lags a few samples, inaudible at 1%. A 9 kHz tone's folded 3rd harmonic is about −86 dB (−40 before). RMS readings get +3 dB, so a sine's threshold is the same in every character.
   - **MODERN** (`compCharacter` index 3, added after 0.4.1 at the owner's request; our design, inspired by a well-known modern studio compressor whose plug-in manual the owner supplied; **never use its name or maker's**). From the manual's descriptions, not measurements: peak detector; the knee changes along RATIO (`kneeDbFor (modern, ratio)`: 24 dB at 1.5:1, 18 at 3:1, 10 at 6:1, 6 at 10:1, 2 at 20:1, 1 at ∞), so low ratios creep in and the top is near brick-wall; attack ×0.5 (down to 0.05 ms); release ÷ (1 + reduction / 6), quick out of deep compression then slower; ratios 8–14 get OPTO's stretching release.
   - **MODERN's distortion lever** (`compDistortion`: Clean / 2nd / 2nd + 3rd, the CLEAN / 2ND / 2+3 lever between ATTACK and RELEASE; does nothing in the other characters). Computed with FET's grit in `addGrit` (2× rate, only the added part filtered, plus a 5 Hz DC blocker for the even part): 2ND gives about 1.3% 2nd at 2 dB of reduction, 4% at 22 dB; 2+3 about 0.8% 2nd + 1.3% 3rd, rising to 2.7% + 3.3%. Amounts are our guess.
   - **Release stretch** (OPTO always, VCA/FET with AUTO): 0.35× after a short peak up to 3× after long compression (a 1.5 s memory of the reduction).
   - Meters: the meter window's COMP pair per channel; COMP's own gain-change meter (its own source: each reading empties a source) shows reduction left, make-up right.
5. **EQ: five bands. (Built, step 2.)** Each band has an on LED, frequency (even travel per octave), **GAIN ±15 dB**.
   - **HF** 2–20 kHz, high shelf (default) or bell. **LMF** 40–600 Hz, bell (default) or low shelf. **LF** 20–200 Hz, low shelf (default) or bell. The SHELF key and the curve key are the same switch. As bells they use Q 0.9.
   - **HMF** 400 Hz–5 kHz and **MF** 150 Hz–2.5 kHz: bells with **Q** 0.5–4 (the small knob turns 9 to 3 o'clock). The concept's curve keys on these two were removed: they'd have done nothing.
   - Bells use **proportional Q** (our design): Q × 2^((|gain| − 6) / 18), so the knob reads true at ±6 dB, broader for small moves, narrower for big ones. Shelves are the cookbook's slope-1 shelves (no overshoot), the frequency at half the gain.
   - Changes glide (30 ms; coefficients every 32 samples); shelf↔bell fades the band out and in; a band at 0 dB stops running once its filter has rung out, so defaults are bit-exact.
6. **LIMIT. (Built, step 4; our design.)** Off by default. A peak limiter with **no look-ahead (zero latency)**: an instant-attack peak envelope, so the ceiling always holds, at the price of squaring off the first instant of a hard-limited peak. THRESHOLD −20 to 0 dBFS (default −1); MAKE UP 0–12 dB (the ceiling rises with it); MIX 0–100% (below 100% some original peaks come back); LINK shared with COMP.
   - **BRICK:** gain = ceiling / envelope, 100 ms release. **SOFT:** a 6 dB knee centred on the threshold (it flattens exactly at the threshold, so it holds the ceiling too), 150 ms release. An attack time was tried and dropped: without look-ahead it lets every hit through. **AUTO:** BRICK's ceiling, release 30 ms after a short peak stretching to about 400 ms under steady limiting.
   - If the owner wants cleaner hard limiting, add look-ahead as a constant latency (reported with `setLatencySamples`), on or off.
7. **SATURATION. (Built, step 6; our design.)** Off by default. The owner wanted it "more like" a well-known harmonics-generator saturator (they supplied its user guide; **don't name it**): we took the general idea, not a model of it (the guide has no curves, and it can't be measured here).
   - **DRIVE** 0 to +24 dB (dB, not the concept's %), level-compensated (÷ drive), so it changes colour, not loudness. The saturation point at DRIVE 0 is +6 dBFS (`headroom` 2): normal levels barely coloured at 0, well in at +18 to +24.
   - **HARMONICS** (the concept's TONE knob): 0% = a tanh biased by 0.5 (lopsided, valve-like, mostly 2nd), 100% = plain tanh (transistor-like, 3rd); a blend between. A 5 Hz DC blocker removes the lopsided curve's offset.
   - **VCA CLIP**: a hard-kneed clip at 0.8 (in the driven domain) before the curves, blended in over 30 ms. **MIX** 0–100%: a small knob under VCA CLIP (9 to 3 o'clock).
   - 4× oversampling (JUCE polyphase IIR, 2 stages); dry and wet are mixed at the high rate, so they line up. A few samples' delay while on (not reported to the host); while off, the audio is untouched but the filters keep running on a copy (`warmUp`), so switching on doesn't ring.
8. **FILTERS / TONE**, the console colour module, which also holds the meter window. **(Built, step 6b; our design.)** The lamp (`toneOn`, on by default) switches THD, the noise and the channels' differences; off, both sides are channel 1.
   - **THD** (`toneThd`, OFF to 100): x + a·x² + b·x³ at 2× oversampling (enough for a cubic: no aliasing), a 5 Hz DC blocker. At 100: a = 0.06, b = 0.04, so about 3% 2nd at full scale, 0.3% at the console's nominal −20 dBFS; it grows with level like a line amp. The amounts are our guess. While THD is 0 the audio is untouched (warm-up copy as in SATURATION).
   - **V GAIN** (`toneVGain`, OFF to 100): the noise floor only. White hiss, independent per side, −64 dBFS RMS at 100; 50 Hz hum (plus 100 and 150 Hz) at −72 dBFS. Level ∝ V GAIN², so −76 at 50. Levels are our guess.
   - **The channel knob** (`toneChannel`, 1–72, default 1) and the LEFT / RIGHT displays: the left side uses the chosen channel, the right the next (72 wraps to 1).
     - **Channel 1 is the exact nominal design** (factors of exactly 1, so bit-exact with the colour off).
     - Each channel's tolerances come from a hash of its number (`ConsoleChannel::number`), the same every time: EQ frequencies ±4% and gains ±4% (of the gain dialled in, so flat stays flat), HPF / LPF corners ±3%, THD's 2nd and 3rd ±25%, hiss ±1.5 dB, hum ±3 dB. The sizes are our guess; the SATURATION module and the dynamics don't vary.
     - The STEREO MODE picks the right side's channel in `updateSides()`.
   - **The meter window:** IN/OUT peak meters, L and R, scale +24 to -60. IN and OUT side by side (owner decision), with OVL and the peak-reset button. Small **GR / COMP / LIMIT** gain-reduction meters run along the bottom, for L and R.
9. **MONO MAKER / STEREO WIDTH. (Built, step 7; our design.)** Both off by default, each with its on LED and a 20 ms fade; nothing happens on a mono track. Mono maker first, then width.
   - **MONO MAKER** 20–500 Hz (default 100): the side goes through a Linkwitz-Riley 24 dB/oct high-pass (two Butterworth sections); the mid through the same crossover's low + high halves summed (an allpass), so mid and side keep the same phase and the image above the frequency is exact. Coefficients every 8 samples while the frequency glides (every 32 clicked slightly).
   - **STEREO WIDTH** 0–200% (default 100): side × width; the mid (and so the mono sum) is never touched.
10. **IN / OUT. (Built, step 1.)** **INPUT** (−24 to +12 dB, before everything), **Ø** (phase invert) and **MUTE**.
11. **STEREO MODE. (Built, step 7.)** `stereoMode`: **ANALOG** (default; the right side uses the next channel) or **DIGITAL** (both use the left channel; the RIGHT display shows the same number).
    - **RANDOM CH.** (momentary keys, `ChannelRegistry`): **ONE** moves this instance to a random channel that no other PR7-C in the process uses (ANALOG takes its channel and the next; DIGITAL only its own), never its current one. **ALL** re-rolls every instance in turn, each avoiding those given out before it. With nothing free (over 36 ANALOG instances), any other channel. The new channel is set as a parameter change (the host records it).
    - A host that runs each plugin in its own process can't share the registry.
    - The IN / OUT lamp (`output.on`) is lit and does nothing: the output stage is always on.
12. **OUTPUT TRIM** knob (±12 dB) and the **output fader** (+10 dB down to silence at the very bottom; `outputLevel`). (Built, step 1.)
13. **Header. (Built, step 8.)**
    - The preset's slot and name ("A11  Modern Metal Drums"; click it for the list: Default, a submenu per bank, Genres and Signature Sounds submenus with each set's six parts, then "Yours" A to Z) with ◀ ▶ (step through Default, A01 ... G38, then yours, wrapping). `getCurrentSlot()` doesn't read the user folder, so the header can ask often. " *" after the name = changed since loaded (compared against what the preset set, in the editor's timer).
    - **SAVE** asks for a name (a user preset; asks before replacing one; factory names refused). The **"…"** menu: back to the defaults, delete the current user preset, show the presets folder.
    - A preset sets **every** parameter except `isKept` ones: the fader, the console channel (so ONE / ALL survive), LISTEN and MUTE. Unmentioned parameters go to their defaults; a user preset saved before a parameter existed gets its default.
    - Presets are **not** exposed to the host as programs: hosts calling setCurrentProgram (0) at song load would reset the settings.
    - The settings gear holds the FINISH choice. No A/B, no copy/paste (owner decision).

## Build order (agreed with the owner)

One module at a time. Each step is built, tested and **rendered**, and the owner sees the render before you move on. **All eight steps are done (0.4.0).** Further work comes from the owner's listening tests in BeatMaker.

1. **The panel layout and meters:**
   - The concept's look: brushed light panels with screws, black knobs with glowing orange rings, LED buttons, the header and footer.
   - Every module's panel drawn, with its controls.
   - Working meters.
2. **EQ (five bands, as decided) and the HPF/LPF filters.**
3. **Compressor.**
4. **Limiter.**
5. **Gate / expander** (and SC FILTER).
6. **Saturation**, then the FILTERS / TONE colour (THD, V GAIN, channels).
7. **Stereo features:** mono maker, stereo width, stereo modes.
8. **Presets and the header.** (Built: factory and user presets, no A/B.)

## Owner decisions

Settled with the owner during step 1 (after reading a research summary of the reference). Follow these; they override the module notes above where they differ.

- **GATE:** FAST / MED / SLOW are the **attack** (as on the reference). The knob that was ATTACK on the concept is **HOLD** (how long the gate stays open before closing). No INV button. (Built as such, step 5.)
- **COMP:** keep ATTACK, RELEASE, AUTO and VCA / OPTO / FET (our own design). Added later: **MODERN** as a fourth key, its **CLEAN / 2ND / 2+3** distortion lever, and **EDGE** on the SC FILTER (the owner chose the name MODERN, the lever and EDGE). RATIO has a **SOFT** position at the bottom (the ratio rises gently with level, roughly 3:1 on normal material towards 6:1 and more on peaks), then about 1.5:1 to ∞:1. Added **MAKE UP** (0 to +20 dB). No MIX: it didn't fit the panel cleanly.
- **EQ: five bands.** LF sits on the concept's blank plate. HF, LMF and LF have a **SHELF** key (lit = shelf, off = bell); HMF and MF have Q. No INVERT, GAIN SCALE or AUTO LISTEN for now.
- **V GAIN** is the console **noise** only, OFF to audible. THD is the density.
- **SATURATION:** DRIVE, HARMONICS (2nd ↔ 3rd, replacing TONE), VCA CLIP and a small MIX; DRIVE keeps the level steady. (Built, step 6.)
- **IN / OUT column:** **INPUT** (-24 to +12 dB, 0 dB at the top of the knob) before everything; **OUTPUT TRIM** (±12 dB); the **fader** (+10 dB down to **silence** at the very bottom; ID `outputLevel`). All three work, with Ø and MUTE.
- **Meters:** IN (L, R) and OUT (L, R) side by side. **Console calibration: 0 on the scale = -20 dBFS**, so +20 is digital full scale. The LEDs run **orange at the bottom to hot pink at the top** (`ParagonColours::meterColour`, owner's request at 0.4.0, replacing white / amber / red), the GR meters too; past full scale shows on the **OVL** lamp. The round button (or OVL) clears the peak marks and OVL. The saturation/colour modules should use the same reference: 0 on the meter is the console's nominal level.
- **LINK** (on the LIMIT panel, ID `dynamics.link`) links the gate, compressor and limiter.
- **Mono maker** 20 to 500 Hz. **Stereo width** 0 to 200%. No PAN (the DAW has one).
- **Finish:** the family finishes only (Pearl White default, White + Neon, Pearl Black), from the settings gear. **Pearl Black is near black**: the plates use PR7-G's own black face gradient (#1a1819 at the lit top edge, #121112, #0a0a0a, #0e0d0e, #070707; `PedalBody.cpp` in Paragon-X1), and nothing lighter is laid over them. With neon keys (Pearl Black, White + Neon) the fader cap is a neon key too. Menus and dialogs are black with white lettering.
- **Stereo mode:** ANALOG (left and right use different channels) and DIGITAL (both use the left channel). Under RANDOM CH.: ONE re-rolls this instance to an unused channel; ALL re-rolls every PR7-C in the session. Default channels 1 / 2, ANALOG.
- **No backward compatibility needed yet** (said at 0.4.0): the owner has no songs or presets saved with any version so far. Don't spend effort keeping older saved projects sounding the same. Still keep parameter IDs stable from here on, as songs saved with 0.4.0 onward will depend on them.
- **Sidechain:** internal only, no external key.
- **Presets:** factory presets and user presets (SAVE). No A/B, no copy/paste.
- **Knobs:** every working knob and the fader show their value while turned; Shift-drag for fine control. `attachKnob` turns on the value popup (`setPopupDisplayEnabled`); do the same for any knob attached another way.

From the reference research, for reference when building the DSP (treat ranges as approximate): the compressor detector is RMS, the limiter peak-detected (no look-ahead documented); the gate's release is exponential by default (LINEAR switches it); channel 1 is the nominal design.

## Names

**No trademarked names or branding** on our modules or controls. Don't use:
- the console's or the plugin maker's names;
- the reference's "TMT" (Tolerance Modelling Technology) label;
- "Pultec", or other product names.

Describe things as "modelled on" or "inspired by", or by what they do. The concept's "TMT INSIDE" badge becomes our own word, e.g. **CHANNEL** or **CONSOLE CHANNEL**.

The owner's manual for the reference is **not** in this repo: it's someone else's copyrighted document.

## Build, test, check

- **Build:** `cmake -B build && cmake --build build -j8`. JUCE 9.0.4 and Catch2 are fetched by CMake.
- **Tests:** `xvfb-run -a build/tests/ParagonPR7CTests`. All must pass. **Test names must be plain ASCII** (no Ø, ∞, ±): CTest passes each name to the test binary on the command line, Windows mangles anything else, and the test then "matches nothing" and fails on CI. New DSP gets tests that measure what it does to the sound: frequency responses, gain reduction at known levels, harmonics, stereo image (mid/side levels), and that defaults pass audio through untouched.
- **pluginval:** `xvfb-run -a <pluginval> --strictness-level 10 --validate-in-process --validate "build/ParagonPR7C_artefacts/Release/VST3/Paragon PR7-C.vst3"`. Check the **exit code**, not just "SUCCESS". CI uses the official pluginval v1.0.4 release.
- **Renders:** `PARAGON_SNAPSHOT=out.png xvfb-run -a build/tests/ParagonPR7CTests "[snapshot]"`.
  - `PARAGON_SNAPSHOT_WIDTH=2896` gives a 2× render. `PARAGON_SNAPSHOT_FINISH=black|neon` picks the finish. `PARAGON_SNAPSHOT_PRESET=name` loads a factory preset.
  - Look, crop and zoom before claiming a visual change is right. Compare against `docs/concept.png`.
- **Real-time safety:** no allocation or locks on the audio thread. Smooth every parameter so changes don't click.
- **Parameters:**
  - IDs live in `source/Parameters.h` and are saved in projects: never rename a shipped one.
  - Choices are saved by **index**: only append.
  - Stepped ranges use `steppedRange` (snaps to whole steps counted from 0), never `NormalisableRange (min, max, step)`: JUCE counts its steps from the minimum, and a fused multiply-add (Apple silicon) lands a hair off, so 0 dB wasn't 0 and the EQ bands never switched off on Macs.
  - The Linux build here can't show Apple-silicon rounding. To check exact (bit-for-bit) tests the way macOS runs them, build the tests separately with `-DCMAKE_CXX_FLAGS="-march=haswell -ffp-contract=fast"`, in a separate build folder and `FETCHCONTENT_BASE_DIR` (sharing `build/_deps` breaks the main build).
- **CI:** after every push, check that the run went green on all three systems (GitHub MCP `actions_list`). A red macOS or Windows run blocks the release, and nothing else says so.
  - Each new parameter gets the next version hint.

## Releases

- Bump `project(ParagonPR7C VERSION x.y.z)` in CMakeLists.txt and add a matching `## x.y.z` section at the top of CHANGELOG.md, then push.
- CI builds Windows, macOS and Linux, runs the tests and pluginval, and publishes a GitHub Release with the three zips (`ParagonPR7-C-vX.Y.Z-<OS>-VST3.zip`), using that changelog section as the notes.
- About 15 minutes after pushing, check the release published all three zips.
- The owner installs from GitHub Releases on **Windows** and works in **BeatMaker**.

## How the owner likes to work

- **Explain the why**, specifically: what was wrong, what changed, and the trade-offs. Plain language; the owner is a musician, not a DSP engineer.
- **Be honest about limits:**
  - Say when something is our design rather than a copy.
  - Say when a detail is a guess.
  - Say what can't be checked here (audio by ear).
  - Ask for source material rather than invent specifics.
- **Show it:** a rendered screenshot of every visual change.
- **"merge it"** means: open a PR from the working branch to main and merge it (merge commit), then reset the working branch to the new main.
- Commit messages explain what and why.
