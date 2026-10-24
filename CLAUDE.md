# Paragon PR7-C: notes for Claude

**Paragon PR7-C** is a console **channel strip** plugin (AGPLv3, JUCE, VST3). It's the fourth member of the Paragon PR7 family:

| Plugin | Repo |
|---|---|
| PR7-G (guitar) | `ARYEZ/Paragon-X1` |
| PR7-B (bass) | `ARYEZ/Paragon-PR7-B` |
| PR7-V (vocals) | `ARYEZ/Paragon-PR7-V` |

Unlike PR7-B and PR7-V, it is **not** a copy of PR7-G. It was seeded lean: PR7-G's build, release and test setup, its look-and-feel pieces, and an empty true-stereo processor. Read this file, then look at **`docs/concept.png`**, the owner's concept design. The UI is built to match it.

## What's here (the seed, 0.1.0)

- **The plugin:** true stereo (stereo in/out, or mono in/out on a mono track).
  - Its own identity: `Paragon PR7-C`, plugin code `Pr7c`, bundle `com.paragon.paragonpr7c`, maker code `Prgn`.
  - One parameter, `outputLevel`, plus input and output peak meters.
- **`source/ui/MainView`:** a fixed **1448 × 1086** canvas, exactly the concept image's size, so coordinates can be read straight off `docs/concept.png`. The editor scales it to the window.
  - `MainView::modules()` holds every module's rectangle, measured off the concept and drawn as dashed placeholders. Each module replaces its placeholder as it's built.
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

1. **HPF / LPF.** HPF about 20 Hz–20 kHz (concept scale 40–1.2k), LPF about 1 kHz–30 kHz (scale 3k–30k), 12 dB/octave.
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
4. **COMP.**
   - **THRESHOLD** and **RATIO**. Ratio runs about 1.5:1–20:1; consider a **soft-knee** position at the bottom of its range, where the ratio rises smoothly with level, as on the reference.
   - **VCA / OPTO / FET** character switch, a new idea for this design:
     - **VCA:** clean, precise, hard knee.
     - **OPTO:** slow, programme-dependent, smooth release.
     - **FET:** fast, aggressive, adds harmonics.
   - **ATTACK**, **RELEASE** and an **AUTO** release LED.
   - **GR meter** from -20 to +20 (a "gain change" meter: reduction left, make-up right).
   - Start from `reference/pr7g/CompGate` (the Daybreak type).
5. **EQ: four bands.** Each band has an on LED, frequency, **GAIN ±15 dB**, and a **shape button** (shelf ↔ bell).
   - **HF:** about 2–20 kHz; shelf or bell; no Q on the concept.
   - **HMF:** about 400 Hz–5 kHz; with **Q** (0.5–4).
   - **MF:** about 150 Hz–2.5 kHz; with **Q** (0.5–4).
   - **LMF:** about 20–300 Hz; shelf or bell; no Q on the concept.
   - Musical, broad, "console" curves. Gain-dependent Q (proportional Q) suits this style.
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

## Open questions: ask the owner, don't guess

- **GATE:** the concept has both an **ATTACK knob** and **FAST / MED / SLOW** buttons. What does each do?
- **OUTPUT:** there's **OUTPUT LEVEL** at the top, and **OUTPUT TRIM** plus a **fader** lower down. Should the top one be an **INPUT** gain, as on the reference console? How do trim and fader differ?
- **V GAIN:** the scale reads -15 to +15 dB on the concept. Is it a noise level, or an input "drive" gain?
- **STEREO MODE:** exactly what ANALOG, RANDOM CH., ONE and ALL do, and whether there's a "same channel both sides" mode.
- **External sidechain:** does the gate/comp need a key input from another track?
- **Finish:** should PR7-C offer the family's Pearl White / White + Neon / Pearl Black finishes for its panels? The concept looks like light brushed metal.
- **Presets:** factory presets (vocals, drums, bass, mix bus...)? A/B, copy/paste?

## Names

**No trademarked names or branding** on our modules or controls. Don't use:
- the console's or the plugin maker's names;
- the reference's "TMT" (Tolerance Modelling Technology) label;
- "Pultec", or other product names.

Describe things as "modelled on" or "inspired by", or by what they do. The concept's "TMT INSIDE" badge becomes our own word, e.g. **CHANNEL** or **CONSOLE CHANNEL**.

The owner's manual for the reference is **not** in this repo: it's someone else's copyrighted document.

## Build, test, check

- **Build:** `cmake -B build && cmake --build build -j8`. JUCE 9.0.4 and Catch2 are fetched by CMake.
- **Tests:** `xvfb-run -a build/tests/ParagonPR7CTests`. All must pass. New DSP gets tests that measure what it does to the sound: frequency responses, gain reduction at known levels, harmonics, stereo image (mid/side levels), and that defaults pass audio through untouched.
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
