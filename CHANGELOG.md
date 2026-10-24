# Changelog

Each version's notes appear on its GitHub Release page.

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
