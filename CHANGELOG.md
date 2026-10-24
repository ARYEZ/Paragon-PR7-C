# Changelog

Each version's notes appear on its GitHub Release page.

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
