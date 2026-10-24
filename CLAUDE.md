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
  - Parameters: `inputGain`, `outputTrim`, `outputLevel` (the fader), `phaseInvert`, `mute`; the HPF / LPF (`hpfOn`, `hpfFreq`, `lpfOn`, `lpfFreq`, `filterPlacement`); the EQ (`<band>On/Freq/Gain` plus `<band>Q` or `<band>Shelf`, bands from `ParamRanges::eqBands`); the compressor (`comp*`) and `dynamicsLink`. Input and output peak meters (console-calibrated, with OVL), and gain-reduction sources (`getGainReduction`) for the GR meters, which stay at 0 until the dynamics exist.
- **`source/ui/MainView`:** a fixed **1448 × 1086** canvas, exactly the concept image's size, so coordinates can be read straight off `docs/concept.png`. The editor scales it to the window.
  - **Step 1 is done:** every module's panel is drawn with all its controls, placed as on the concept. `MainView::modules()` holds each module's rectangle.
  - Controls are registered by ID (`"comp.ratio"`, `"hf.on"`...; see `MainView.cpp`). Controls of unbuilt modules are **inert** (tooltips say which step brings them). Each step creates the parameters and brings its controls to life with `attachKnob` (prints the scale at the true values and turns on the value popup), `attachButton` and `attachSwitch`.
- **`source/dsp/`:** `Biquad` (cookbook filters), `ChannelEq` (`EqBand`, `CutFilters`), `Compressor`.
  - Modules jump straight to their settings on the first block after `prepare`/`reset` (`fresh`), and glide or fade only for changes made while playing.
  - The processor runs the modules in stretches no longer than the prepared block size (`processModules`), as their working buffers are that size. The dynamics' detector hears `sidechain`: the audio at that point, through the HPF / LPF when they're on SC.
- **`PARAGON_SNAPSHOT_DEMO=1`** switches modules on for a render that shows them working (the compressor so far).
  - `source/ui/Controls`: `Knob` (draws itself; its slider only takes the mouse over the knob), `Led`, `LeverSwitch`, `RoundButton`, `GainReductionMeter`. Buttons are `TextButton`s with `ParagonLookAndFeel::consoleButtonProperty`.
- **Carried over from PR7-G:**
  - `ParagonLookAndFeel` (Inter fonts, knobs, LED/neon bars, piano keys, chrome bezels).
  - `Finish` (Pearl White / White + Neon / Pearl Black finishes and the satin grain). Its settings file is "Paragon PR7-C".
  - `LedDisplay` (the orange LED screen; its static `paintScreen` / `paintGlassOverlay` suit the channel-number displays and the meter window). **Its rig/tuner contents are PR7-G's.** Trim them when you build those displays.
  - `Logo` (the PR7 mark, now "-C").
  - `LevelMeter` / `LevelMeterSource`.
  - `dsp/EffectUtils.h` (`BypassFade` etc.).
- **`reference/pr7g/`:** PR7-G's compressor (Daybreak), graphic EQ and drive code. **Not compiled.** See `reference/README.md`.

## The modules (from the concept)

The **order is fixed**: the owner decided the user can't reorder modules. The signal flows:

**input → HPF/LPF → GATE/EXP → COMP → EQ (HF, HMF, MF, LMF) → LIMIT → SATURATION → FILTERS/TONE colour → MONO MAKER / STEREO WIDTH → OUTPUT**

The **SC FILTER** shapes only the dynamics' **sidechain** (detector), never the audio. Every module has an **on LED** in its header.

The behaviour notes below are **modelled on** a classic 1970s/80s British console channel and mastering compressor (the owner supplied a commercial plugin manual as reference). They're described in our own words. **Never use that product's names or trademarks** (see "Names" below).

1. **HPF / LPF. (Built, step 2.)** HPF 20 Hz–2 kHz, LPF 1–20 kHz, 12 dB/octave Butterworth, both off by default. Each fades in/out over 20 ms (on/off and placement changes). In SC they leave the audio alone; the dynamics steps must feed them into the detector (`filterSettings` in the processor).
   - A three-way switch **IN / POST / SC** places them:
     - **IN:** at the input, before everything.
     - **POST:** after the compressor, before the limiter.
     - **SC:** in the dynamics' sidechain only.
   - That's a routing choice inside the fixed order, not reordering.
2. **GATE / EXP.** Gate or expander (**EXP** toggles).
   - **THRESHOLD**, plus a **-30** button that shifts the threshold range down 30 dB for quiet material.
   - **RANGE**, the maximum attenuation; 0 means off.
   - **RATIO**, for expander mode (1.5:1–20:1 on the concept).
   - **ATTACK** knob and **FAST / MED / SLOW**. *See open questions: the concept has both.*
   - **RELEASE**, and **LINEAR**: a linear release curve instead of the default exponential one.
   - Gain-reduction meter: GR.
3. **SC FILTER.** HPF (about 20–200 Hz) and LPF (about 1–20 kHz) in the gate/comp detector.
   - **LISTEN** lets you hear the sidechain signal.
   - **HIGH Q** gives resonant filters that emphasise the frequencies at the cutoff.
4. **COMP. (Built, step 3; all of it our design.)** Off by default. THRESHOLD -50 to 0 dBFS; RATIO SOFT, 1.5:1 to ∞ (marks evenly round the knob, `compRatioMarks`); ATTACK 0.1–30 ms; RELEASE 50 ms–3 s; AUTO; MAKE UP 0–20 dB; LINK (`dynamicsLink`, on by default) uses the louder channel's level for both.
   - **SOFT:** the output over the threshold is 4·over/(4+over): about 3:1 at 8 dB over, 6:1 at 20, 10:1 at 36, a smooth knee built in.
   - **VCA:** RMS detector (5 ms), 1 dB knee. **OPTO:** RMS (20 ms), 12 dB knee, attack ×2, stretching release. **FET:** peak detector, 3 dB knee, third-harmonic grit ∝ gain reduction (about 1% at 20 dB), measured against the signal's own peak so it doesn't depend on level. The grit is a gentle cubic and isn't oversampled (aliasing tiny); revisit with the SATURATION step. RMS readings get +3 dB, so a sine's threshold is the same in every character.
   - **Release stretch** (OPTO always, VCA/FET with AUTO): 0.35× after a short peak up to 3× after long compression (a 1.5 s memory of the reduction).
   - Meters: the meter window's COMP pair per channel; COMP's own gain-change meter (its own source: each reading empties a source) shows reduction left, make-up right.
5. **EQ: five bands. (Built, step 2.)** Each band has an on LED, frequency (even travel per octave), **GAIN ±15 dB**.
   - **HF** 2–20 kHz, high shelf (default) or bell. **LMF** 40–600 Hz, bell (default) or low shelf. **LF** 20–200 Hz, low shelf (default) or bell. The SHELF key and the curve key are the same switch. As bells they use Q 0.9.
   - **HMF** 400 Hz–5 kHz and **MF** 150 Hz–2.5 kHz: bells with **Q** 0.5–4 (the small knob turns 9 to 3 o'clock). The concept's curve keys on these two were removed: they'd have done nothing.
   - Bells use **proportional Q** (our design): Q × 2^((|gain| − 6) / 18), so the knob reads true at ±6 dB, broader for small moves, narrower for big ones. Shelves are the cookbook's slope-1 shelves (no overshoot), the frequency at half the gain.
   - Changes glide (30 ms; coefficients every 32 samples); shelf↔bell fades the band out and in; a band at 0 dB stops running once its filter has rung out, so defaults are bit-exact.
6. **LIMIT.**
   - **THRESHOLD** and **MAKE UP**.
   - **BRICK / SOFT / AUTO** modes: brick-wall, soft-knee, and programme-dependent release.
   - **MIX** (0–100%).
   - **LINK** gives stereo-linked gain reduction: the same reduction on both channels, from the louder one.
   - A peak (not RMS) detector. Report latency if it looks ahead.
7. **SATURATION.**
   - **DRIVE** (0–100%).
   - **VCA CLIP:** a soft-to-hard clipper, like overdriving the console's VCA.
   - **TONE** (±12). Oversample the non-linear parts to avoid aliasing.
8. **FILTERS / TONE**, the console colour module, which also holds the meter window:
   - **THD:** harmonic saturation and density added to the EQ'd signal; OFF to 100.
   - **V GAIN:** console noise floor (hiss/hum), off to audible. The concept scale reads -15 to +15 dB; *confirm with the owner*.
   - **The channel knob with two LED number displays, LEFT and RIGHT** (e.g. 28 / 29). Each number picks one of **72 console channels**, each with slightly different component tolerances: tiny, repeatable differences in EQ frequencies and gains, filter corners, saturation and noise.
     - In stereo, left and right are adjacent channels.
     - **Channel 1 is the exact nominal design.**
     - Seed the variation from the channel number so it's identical every time.
   - **The meter window:** IN/OUT peak meters, L and R, scale +24 to -60. An **IN / OUT** button chooses whether they show the input or output. Small **GR / COMP / LIMIT** gain-reduction meters run along the bottom, for L and R.
9. **MONO MAKER / STEREO WIDTH.**
   - **MONO MAKER** (about 20–200 Hz) folds everything below the frequency to mono.
   - **STEREO WIDTH** (0 to well over 100%) widens with mid/side, keeping the centre and staying mono-compatible.
   - Each has an on LED.
10. **OUTPUT.**
    - **OUTPUT LEVEL** knob (-24 to +12 dB).
    - **Ø** (phase invert) and **MUTE**.
11. **STEREO MODE.**
    - **ANALOG:** left and right use different channels (the two numbers).
    - **RANDOM CH.**, **ONE**, **ALL** follow the reference:
      - **ONE** picks a random unused channel for this instance.
      - **ALL** re-picks for every PR7-C instance in the session, so each track has its own.
      - The instances need to know about each other through a shared, in-process registry.
    - *Confirm with the owner exactly what each button does, and whether there's a "digital" (both sides the same channel) mode.*
12. **OUTPUT TRIM** knob (±12 dB) and the **output fader** (+10 to -60 dB). *See open questions.*
13. **Header:**
    - The preset name with ◀ ▶.
    - **A / B** compare (with an LED).
    - **SAVE**.
    - A "…" menu.
    - The settings gear (a natural place for the **FINISH** choice).

## Build order (agreed with the owner)

One module at a time. Each step is built, tested and **rendered**, and the owner sees the render before you move on.

1. **The panel layout and meters:**
   - The concept's look: brushed light panels with screws, black knobs with glowing orange rings, LED buttons, the header and footer.
   - Every module's panel drawn, with its controls (inert until its DSP exists).
   - Working meters.
2. **EQ (the four bands) and the HPF/LPF filters.**
3. **Compressor.**
4. **Limiter.**
5. **Gate / expander** (and SC FILTER).
6. **Saturation**, then the FILTERS / TONE colour (THD, V GAIN, channels).
7. **Stereo features:** mono maker, stereo width, stereo modes.
8. **Presets, A/B and the header.** *Ask the owner where this fits.*

## Owner decisions

Settled with the owner during step 1 (after reading a research summary of the reference). Follow these; they override the module notes above where they differ.

- **GATE:** FAST / MED / SLOW are the **attack** (as on the reference). The knob that was ATTACK on the concept is **HOLD** (how long the gate stays open before closing). No INV button.
- **COMP:** keep ATTACK, RELEASE, AUTO and VCA / OPTO / FET (our own design). RATIO has a **SOFT** position at the bottom (the ratio rises gently with level, roughly 3:1 on normal material towards 6:1 and more on peaks), then about 1.5:1 to ∞:1. Added **MAKE UP** (0 to +20 dB). No MIX: it didn't fit the panel cleanly.
- **EQ: five bands.** LF sits on the concept's blank plate. HF, LMF and LF have a **SHELF** key (lit = shelf, off = bell); HMF and MF have Q. No INVERT, GAIN SCALE or AUTO LISTEN for now.
- **V GAIN** is the console **noise** only, OFF to audible. THD is the density.
- **IN / OUT column:** **INPUT** (-24 to +12 dB, 0 dB at the top of the knob) before everything; **OUTPUT TRIM** (±12 dB); the **fader** (+10 dB down to **silence** at the very bottom; ID `outputLevel`). All three work, with Ø and MUTE.
- **Meters:** IN (L, R) and OUT (L, R) side by side. **Console calibration: 0 on the scale = -20 dBFS**, so +20 is digital full scale. White up to 0, amber above (hot), red past +20. **OVL** lamp for anything past full scale. The round button (or OVL) clears the peak marks and OVL. The saturation/colour modules should use the same reference: 0 on the meter is the console's nominal level.
- **LINK** (on the LIMIT panel, ID `dynamics.link`) links the gate, compressor and limiter.
- **Mono maker** 20 to 500 Hz. **Stereo width** 0 to 200%. No PAN (the DAW has one).
- **Finish:** the family finishes only (Pearl White default, White + Neon, Pearl Black), from the settings gear. **Pearl Black is near black**, like the PR7-G black unit (face about #0d0d0d to #131010).
- **Stereo mode:** ANALOG (left and right use different channels) and DIGITAL (both use the left channel). Under RANDOM CH.: ONE re-rolls this instance to an unused channel; ALL re-rolls every PR7-C in the session. Default channels 1 / 2, ANALOG.
- **Sidechain:** internal only, no external key.
- **Presets:** factory presets and user presets (SAVE). No A/B, no copy/paste.
- **Knobs:** every working knob and the fader show their value while turned; Shift-drag for fine control. Enable the value popup (`setPopupDisplayEnabled`) on each knob as its step attaches it.

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
  - `PARAGON_SNAPSHOT_WIDTH=2896` gives a 2× render. `PARAGON_SNAPSHOT_FINISH=black|neon` picks the finish.
  - Look, crop and zoom before claiming a visual change is right. Compare against `docs/concept.png`.
- **Real-time safety:** no allocation or locks on the audio thread. Smooth every parameter so changes don't click.
- **Parameters:**
  - IDs live in `source/Parameters.h` and are saved in projects: never rename a shipped one.
  - Choices are saved by **index**: only append.
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
