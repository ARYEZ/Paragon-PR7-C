# Changelog

Each version's notes appear on its GitHub Release page.

## Unreleased

- **The full panel, laid out as in the concept design.** Every module is drawn with all its knobs, keys, lamps and switches. Most don't do anything yet: each comes to life in its own step (hover over one to see which).
- **Working now:**
  - **INPUT** (the top knob, -24 to +12 dB) sets the level into the strip.
  - **Ø** flips the polarity, and **MUTE** silences the output. Both glide over 20 ms, so they never click.
  - **OUTPUT TRIM** (±12 dB) and the **channel fader** (+10 to -60 dB) set the output.
    - OUTPUT LEVEL from 0.1.0 *is* the fader now, so your old setting carries over.
  - **The meter window:** input and output, left and right, from +24 to -60 dB (0 = digital full scale). The highest peak stays lit until you click the round button between IN and OUT.
  - **Finishes:** Pearl White, White + Neon or Pearl Black, from the settings gear. Your choice is remembered for every PR7-C.

## 0.1.0

- **The start of Paragon PR7-C, the channel strip.** This is a skeleton to build on, not a usable strip yet:
  - It's a true-stereo plugin (or mono on a mono track) with its own name and ID, so it sits alongside PR7-G, PR7-B and PR7-V in your DAW.
  - **OUTPUT LEVEL** (-24 to +12 dB) and **input and output meters** work. Everything else passes through untouched.
  - The window shows where each module will go, laid out as in the concept design.
- The modules come next, one at a time: EQ and filters, then the compressor, limiter, gate, saturation, and the stereo features.
