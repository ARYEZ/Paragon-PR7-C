# Changelog

Each version's notes appear on its GitHub Release page.

## 0.1.0

- **The start of Paragon PR7-C, the channel strip.** This is a skeleton to build on, not a usable strip yet:
  - It's a true-stereo plugin (or mono on a mono track) with its own name and ID, so it sits alongside PR7-G, PR7-B and PR7-V in your DAW.
  - **OUTPUT LEVEL** (-24 to +12 dB) and **input and output meters** work. Everything else passes through untouched.
  - The window shows where each module will go, laid out as in the concept design.
- The modules come next, one at a time: EQ and filters, then the compressor, limiter, gate, saturation, and the stereo features.
