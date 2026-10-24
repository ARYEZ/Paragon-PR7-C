# Reference code from Paragon PR7-G

**Not compiled.** These are copies of PR7-G's DSP (repo `ARYEZ/Paragon-X1`), kept here as starting points:

- **`CompGate`**: PR7-G's compressor/gate. Its **Daybreak** type is a soft-knee, program-dependent compressor with auto make-up, mix and a gain-reduction readout. It's a starting point for COMP, and its gate for GATE / EXP.
- **`GraphicEq`**: PR7-G's nine-band graphic EQ (peaking biquads, loudness-matched presets). A starting point for the four-band EQ's filters.
- **`Drive`**: PR7-G's overdrive and fuzz models (diode clipping, tone stacks), for ideas in SATURATION.

They depend on `EffectUtils.h` (`BypassFade` and friends), which is already in `source/dsp/`. When you build a module from one of these, move the code into `source/dsp/`, adapt it to **stereo** (PR7-G is mono), and delete the copy here.
