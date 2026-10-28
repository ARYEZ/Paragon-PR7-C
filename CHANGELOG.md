# Changelog

Each version's notes appear on its GitHub Release page.

## Unreleased

- **A fourth compressor character: MODERN.** Our own design, inspired by a well-known modern studio compressor:
  - Very gentle, long knees at low ratios (it eases in as you drive it harder), steeper ones in the middle, a smooth optical feel around 10:1, and near brick-wall limiting at the top.
  - A faster attack than the others, and a release that lets go quickly out of heavy compression, then slows.
  - **Its own distortion lever: CLEAN, 2ND or 2+3.** 2ND adds warm, tube-like 2nd harmonic; 2+3 adds 2nd and 3rd, rounding the peaks like tape. A touch at all times, more the harder it compresses.
- **EDGE** on the SC FILTER: the gate and compressor listen harder to the harsh upper mids (around 6 kHz), so they clamp down on a piercing vocal or an edgy guitar. The audio itself isn't filtered.

## 0.4.1

**Look fixes from your first test of 0.4.0.** The sound is unchanged.

- **Menus and dialogs** (the preset list, the "…" menu, the settings gear, SAVE) are now black with white lettering, instead of grey-blue.
- **The meters** now run orange at the bottom to hot pink at the top, like the buttons, instead of white and yellow. The small GR / COMP / LIMIT meters match. Going past digital full scale still lights OVL.
- **The fader cap** is now the same pink-red-orange as the buttons (on Pearl Black and White + Neon), with a pale line across it to read the level.
- **Pearl Black is darker**, using the exact black of the PR7-G black unit.

## 0.4.0

**New: every module works now, and presets.** The saturation, the console colour (THD, V GAIN and the 72 console channels), the stereo tools (mono maker, stereo width, stereo modes) and factory and user presets. Everything new is off by default, so a fresh PR7-C still leaves your audio untouched.

- **The saturation works.** Our own design, after the idea of a harmonics generator:
  - **DRIVE** (0 to +24 dB) pushes the sound into the saturation. The level stays the same, so it changes the colour, not the loudness. At 0, normal levels are only lightly coloured.
  - **HARMONICS** (where the concept had TONE): turn left for 2nd-harmonic warmth (valve-like), right for 3rd-harmonic grit (transistor-like), or anywhere between.
  - **VCA CLIP** flattens the loudest peaks harder, for crunch.
  - **MIX** (a small knob under VCA CLIP) blends in the dry sound, for parallel saturation.
  - It runs at four times the sample rate, so driving it hard doesn't add harsh aliasing. While it's on, that delays the sound by about 0.1 ms, far too little to hear.
- **The console colour works** (FILTERS / TONE). Our own design:
  - **THD** adds the console's own gentle harmonics: barely there at normal levels, more as you run it hot, like a real console's amplifiers. The level stays the same.
  - **V GAIN** adds the console's noise floor: hiss (different on each side) and a little mains hum. Off by default.
  - **The channel knob** picks one of **72 console channels**, each with its own tiny, fixed differences in the EQ, filters, THD and noise. LEFT shows the left side's channel; the right side uses the next one, so left and right are never quite identical, like a real stereo pair on a console. **Channel 1 is the exact design.** Defaults: 1 / 2.
  - The FILTERS / TONE lamp switches all of it off: both sides are then the exact design.
- **The stereo tools work.** Our own design:
  - **MONO MAKER** (20 to 500 Hz) folds everything below its frequency to mono, for a solid centre and tight low end. Above the frequency, the stereo image is exactly as it was.
  - **STEREO WIDTH** (0 to 200%): 0 is mono, 100 as it was, 200 twice as wide. The centre is never touched, so it always sums to mono the same.
  - Both are off by default.
- **STEREO MODE works.** **ANALOG** (the default): the right side uses the console channel next to the left's, so the sides differ very slightly. **DIGITAL**: both sides use the left's channel, so they're treated identically.
- **RANDOM CH.**: **ONE** moves this PR7-C to a random console channel no other PR7-C in your session is using. **ALL** gives every PR7-C in the session its own new channels, like tracks on a real console.
- **Presets.** 16 factory presets as starting points (vocals, drums, bass, guitars, piano, mix bus, master, console character, lo-fi), and your own:
  - **◀ ▶** step through them; **click the name** for the full list.
  - **SAVE** stores the current settings under a name of your choice. Your presets are files in Documents/Paragon PR7-C/Presets, so you can copy them to another computer.
  - A **\*** after the name means you've changed something since loading it.
  - The **…** menu: back to the default settings, delete one of your presets, or open the presets folder.
  - Loading a preset leaves the fader, the console channel, LISTEN and MUTE as they are, as they belong to the track rather than the sound.
- **Cleaner FET compression.** The FET character's grit is now worked out at twice the sample rate, so pushing it hard on bright sounds no longer adds faint, out-of-tune tones (aliasing). The grit itself sounds the same; VCA and OPTO are untouched.
- The gate's FAST / MED / SLOW keys no longer have tooltips saying they don't work yet. They always worked.
- On Macs, every knob's 0 is now exactly 0. A rounding difference on Apple processors left the EQ gains a hair off 0 dB (far too little to hear), which kept all five bands running at their defaults.

## 0.3.0

**New: the dynamics.** The gate / expander, the SC FILTER, the compressor and the limiter all work now. Everything new is off by default, so a fresh PR7-C still leaves your audio untouched.

**Still to come:** saturation, the console colour (THD, V GAIN, the channel numbers), the stereo tools (mono maker, width, stereo modes) and presets. Their controls are on the panel but don't do anything yet.

- **The compressor works.**
  - THRESHOLD, and a RATIO from **SOFT** up to ∞. SOFT squeezes gently on quiet parts and firmer on peaks.
  - **ATTACK**, **RELEASE** and **MAKE UP**, and **AUTO**, which lets go quickly after short peaks and slowly after long compression, so it doesn't pump.
  - Three characters: **VCA** (clean and precise), **OPTO** (smooth, with a soft knee and a release that stretches the longer it works) and **FET** (fast and aggressive, with a touch of grit when it works hard). These are our own designs, not copies of any particular unit.
  - **LINK** (on by default) turns both sides down together, so the stereo image stays put.
  - With the HPF / LPF switch on **SC**, the compressor listens through the filters, so a big kick drum doesn't make the whole mix duck. The audio itself isn't filtered.
  - The **COMP** meters in the meter window show how much it's turning down. The meter under COMP shows the reduction on the left and the make-up gain on the right.
- **The limiter works:**
  - THRESHOLD is a ceiling that nothing gets past (−20 to 0 dBFS). MAKE UP raises the level, and the ceiling with it.
  - MIX blends in the dry sound. Below 100%, some of the original peaks come back through.
  - Three modes: **BRICK**, a hard ceiling; **SOFT**, which eases in from 3 dB below the threshold and lets go gently; and **AUTO**, which lets go quickly after short peaks and slowly under steady limiting.
  - It has no look-ahead, so it adds **no delay** to the track. The trade-off: when it's limiting hard, the very start of each peak is squared off slightly, as on a hardware limiter.
  - **LINK** now works for the limiter too. The **LIMIT** meters show how much it's turning down.
- **The gate / expander works:**
  - THRESHOLD, with **−30** for quiet material, and RANGE: how far it turns down when closed.
  - As a **gate** it opens and shuts, with a 3 dB margin so it doesn't chatter on notes sitting at the threshold. With **EXP** it's an expander: quiet parts are turned down gradually, by the RATIO.
  - **FAST / MED / SLOW** set how quickly it opens. **HOLD** keeps it open after the sound drops. **RELEASE** sets how long it takes to close: quick at first and then easing off, or at a steady rate with **LINEAR**.
  - **LINK** covers the gate too. The **GR** meters show how much it's turning down.
- **The SC FILTER works:**
  - An HPF and LPF on what the gate and compressor listen to. The audio itself isn't filtered.
  - **HIGH Q** makes the filters resonate, so the gate or compressor focuses on the frequencies at the cutoff (a kick's thump, say).
  - **LISTEN** lets you hear what they hear while you set it up.
- **Fixed:** at the start of playback, the filters and EQ faded in over a few milliseconds instead of starting at their settings.

## 0.2.0

**Still to come:** the compressor, limiter, gate, saturation, console colour, stereo tools and presets. Their controls are on the panel but don't do anything yet.

- **The full panel, laid out as in the concept design.** Every module is drawn with all its knobs, keys, lamps and switches. Most don't do anything yet: each comes to life in its own step (hover over one to see which).
- **Changes from the concept:**
  - The EQ has five bands: LF is new, on the panel that was blank.
  - GATE: FAST / MED / SLOW set the attack, and the knob beside RELEASE is HOLD.
  - COMP: RATIO has a SOFT position at the bottom, and there's a MAKE UP knob.
  - V GAIN is the console's noise, from OFF up.
- **The EQ works:** five bands, each ±15 dB.
  - **HF**, **LMF** and **LF** switch between a shelf and a bell (the SHELF key and the curve key do the same thing).
  - **HMF** and **MF** are bells with a Q knob.
  - Like a console EQ, the bells are broad for small boosts and cuts, and get narrower as you push them harder.
- **The HPF and LPF work** (12 dB an octave; HPF 20 Hz–2 kHz, LPF 1–20 kHz). The IN / POST / SC switch moves them:
  - **IN:** at the input.
  - **POST:** after the compressor and EQ.
  - **SC:** out of the audio, ready for the gate and compressor to listen through once those are built.
- Everything glides when you move it, so turning, switching and sweeping never click.
- **Working now:**
  - **INPUT** (the top knob, -24 to +12 dB) sets the level into the strip.
  - **Ø** flips the polarity, and **MUTE** silences the output. Both glide over 20 ms, so they never click.
  - **OUTPUT TRIM** (±12 dB) and the **channel fader** (+10 dB down to silence at the bottom) set the output.
    - OUTPUT LEVEL from 0.1.0 *is* the fader now, so your old setting carries over.
  - Turning a working knob or the fader shows its value. Hold Shift while dragging for fine control.
  - **The meter window:** input and output, left and right, read like a console. 0 is the nominal level (-20 dBFS), and +20 is digital full scale.
    - **OVL** lights if the signal ever goes past full scale.
    - The highest peak stays lit until you click the round button between IN and OUT, which also clears OVL.
  - **Finishes:** Pearl White, White + Neon or Pearl Black (now near black, like the PR7-G black unit), from the settings gear. Your choice is remembered for every PR7-C.

## 0.1.0

- **The start of Paragon PR7-C, the channel strip.** This is a skeleton to build on, not a usable strip yet:
  - It's a true-stereo plugin (or mono on a mono track) with its own name and ID, so it sits alongside PR7-G, PR7-B and PR7-V in your DAW.
  - **OUTPUT LEVEL** (-24 to +12 dB) and **input and output meters** work. Everything else passes through untouched.
  - The window shows where each module will go, laid out as in the concept design.
- The modules come next, one at a time: EQ and filters, then the compressor, limiter, gate, saturation, and the stereo features.
