# Changelog

## 0.3.0 — 2026-10-29

Large spaces sounded fake. Measurements found four causes, fixed here:
- Output taps: Dattorro's taps read some delay lines twice, so the tail contained a copy of itself (up to 180 ms apart
  at large sizes). Small sizes keep Dattorro's taps; from Size ~40 % up, the output blends into a new tap set that
  reads each plain delay once. Tail self-similarity at Size 100 dropped from 0.30 to ~0.17.
- Density: Size now stretches the long delays fully but the diffusing allpasses only gently, and each tank half has
  one extra diffuser. At Size 100, noise-like density now arrives after ~170 ms (before: not within 1 s).
- Damping: the one-pole band split is replaced by 12 dB/oct shelving filters (S = 1, no overshoot).
- Stepped decay: damping is now applied twice per tank half instead of once. At large sizes, the highs used to sit
  still for up to 0.7 s and then drop in one large step. The 8 kHz decay at Size 100 went from 2.6 s to 1.9 s and is
  now consistent across sizes.
- All presets sound somewhat different because of the extra diffusers and new damping.
- OmegaRender: new periodicity (tail self-similarity) measurement; "expected" decay now uses the engine's filter model.

## 0.2.0 — 2026-10-29

- New interface: a hybrid of 1980s rack hardware and a modern dark UI. Brushed faceplate with screws, knurled metal
  knobs with glowing amber value arcs, amber LED readouts, and recessed sections (Main, Diffusion, Modulation, Damping,
  Output EQ).
- LED display showing RT60 across frequency, computed from the engine's real in-loop filters, plus the wet tone curve.
- Presets: 22 factory presets named after ValhallaVintageVerb's reverb modes. The values are our own approximations
  of each mode's character, to be refined against measured Valhalla IRs. The preset bar has prev/next, a list, a
  modified marker (*) and Save (user presets go to %APPDATA%/OmegaReverb/Presets). Factory presets are also exposed
  as host programs, so they appear in FL Studio's preset menu.
- `OmegaSnapshot` dev tool: renders the editor to a PNG.

## 0.1.0 — 2026-10-29

- Project scaffold: CMake + JUCE 8.0.4, VST3 effect for FL Studio.
- First algorithm, the modulated hall: true-stereo input diffusers into a Dattorro/Lexicon-style figure-8 allpass tank,
  random (Catmull-Rom) modulation on two allpasses and two delays, Hermite-interpolated reads, three-band decay inside
  the loop (Bass/High multipliers with crossovers), 14 output taps, 12 dB/oct Low/High Cut on the wet signal.
- Controls: Mix, Pre-Delay, Decay, Size, Early/Late Diffusion, Mod Rate/Depth, Bass Mult/Xover, High Mult/Xover, Low/High Cut.
- Placeholder editor (generic sliders + version header).
- `OmegaRender` tool: renders impulse responses to WAV and measures per-band RT60, echo density, stereo correlation,
  mono-sum change, tail resonances and CPU.
