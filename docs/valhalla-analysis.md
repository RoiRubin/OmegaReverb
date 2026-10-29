# What makes Valhalla reverbs so good

Design reference for OmegaReverb. Compiled 2026-10-29.

**About the sources:** Valhalla's site blocked direct fetches, so Sean Costello's blog posts were not read in full. Some points come from Valhalla's own pages and Costello's statements quoted in search results; those are marked **[D]** for documented. Points marked **[I]** are inferences from standard reverb DSP and how the plugins behave. Valhalla's exact algorithms are not public.

There's no single secret. Several things stack up, and each one matters.

---

## 1. Where the designs come from

- **[D]** Sean Costello has designed reverbs since 1998. He spent his first ten years learning different ways to arrange delay lines into what are called feedback delay networks (FDNs). One person tuning by ear for over 25 years is a large part of why Valhalla sounds consistent.
- **[D]** He builds on the classic research and hardware: Gerzon's FDN theory (he cites it directly for Supermassive's Orion and Cassiopeia modes), the Lexicon 224 / 224XL / 480L, and the EMT 250.
- **[I]** He also clearly draws on Schroeder, Moorer, Jot (FDNs with filters in the feedback loop) and Dattorro's *Effect Design Part 1*. That paper describes the Griesinger/Lexicon "figure-8" plate: a few allpass filters to smear the input, then a loop of delays and allpasses that circulates the sound. He also wrote `reverbsc`, a reverb in the Csound audio language. It uses 8 delay lines with random modulation and is an early version of his approach.
- **[D]** None of the VintageVerb algorithms copy a specific hardware unit exactly. They are "generically" inspired by those eras: the 1970s mode leans toward the 224 and the EMT 250. **This is important:** he recreates what made the old units sound good, not every detail of how they worked.

## 2. Signal flow

The basic chain is: diffuse the input → feed it into a recirculating "tank" (the part that loops and decays) → take the output from several points in that tank.

- **Diffusion = allpass filters.** **[D]** Costello says that wherever reverb papers say "diffusors", you can read "allpass delays". Several allpasses in series have a flat frequency response, but your ear can still hear them as metallic. That is why he limits and tunes them carefully.
- **Two main families of design** **[I/D]**:
  - Lexicon-style loops of allpasses and delays. VintageVerb uses these, and Narcissus in ValhallaRoom is **[D]** "close in architecture to the VintageVerb algorithms".
  - FDNs, where several delay lines are mixed together through a matrix before feeding back. Supermassive is documented as FDN-based **[D]**, each delay can be up to 2 seconds long, and some modes use an "EchoVerb" layout (Aquarius, Pisces).
- **Early "energy" instead of early reflections (ValhallaRoom)** **[D]**:
  - The early section is not a set of discrete echo taps.
  - It deliberately has **no allpasses**, because short allpasses add coloration.
  - It has a fixed attack/decay shape and a Size control in milliseconds.
  - It can feed into the late section, which raises how dense the tail is at the start.
  - **Lesson:** discrete early taps cause comb filtering, so a smeared, diffuse early burst sounds better.
- **Each mode is its own design, not a preset of one engine** **[D/I]**:
  - VintageVerb has 20+ modes (Concert Hall, Plate, Chamber, Random Space, Chaotic Hall/Chamber/Neutral, Chamber1979, Hall1984, …).
  - Supermassive's modes are named after stars and galaxies because, in Costello's words, they "don't fit into the standard room/hall/chamber/plate categories."
  - The layout of the delay network decides how fast echoes build up, how resonances are spread, and the shape of the decay. Knobs can't change those things, so separate designs are what make each mode sound different.

## 3. Modulation: the single biggest factor

- **[D]** Early hardware (EMT 250, Lexicon 224) had very little delay memory. They used heavy time variation to avoid metallic decays, and that became the classic "lush" sound.
- **[D]** Costello distinguishes between two kinds:
  - **Chorus-style modulation** (224 / 224XL): steady, periodic movement.
  - **Random-Hall modulation** (480L): irregular movement.
- **[D]** VintageVerb first used a random walk to drive the modulation. Later versions replaced it with a more "chaotic" modulation source.
- **[I]** Why modulation works:
  - Every fixed delay network has resonant modes: ringing frequencies at fixed pitches. Slowly varying the delay lengths keeps moving those frequencies, so no resonance stays in one place long enough to ring.
  - It also makes the left and right channels differ, which widens the image.
  - It makes a small delay network sound much bigger than it is.
- **[I]** How to do it well:
  - Use random or chaotic modulation sources, not plain sine waves, so there's no audible repeating pattern.
  - Give each delay line its own rate, with rates that aren't simple multiples of each other.
  - Modulate only some of the delay lines.
  - Tune the modulation depth per mode. Too much and you get audible pitch wobble on piano and sustained notes.
- **[I]** The way the delays are read at fractional positions (interpolation) changes the tone:
  - Linear interpolation dulls high frequencies, and the amount changes as the delay moves.
  - Allpass interpolation keeps the highs but misbehaves when the delay moves fast.
  - Cubic or Lagrange interpolation is a compromise.
  - Some of the "vintage" character is literally these side effects, which is the basis of the Color modes below.

## 4. Decay and tone shaping

- **[D/I]** Decay is set per frequency band, like Lexicon: a Bass Multiplier with a crossover frequency, and a high shelf or high multiplier with its own frequency, all applied **inside the feedback loop**. Separate High Cut and Low Cut filters sit outside the loop.
- **[D]** Supermassive's Leo mode spreads the EQ filters throughout the network instead of putting them in one spot.
- **[I]** Decay is in real seconds and stays the same when you change Size. The loop gain for each delay is calculated from its length: g = 10^(−3·d / (RT60·fs)), where d is the delay length in samples and fs is the sample rate. This is a basic requirement, and Valhalla's controls behave as you'd expect because of it.
- **[I]** High frequencies dying faster than lows (like air absorption) is a large part of why the tails sound expensive and don't fight the dry signal.

## 5. Grit and band-limiting as a feature

- **[D]** VintageVerb's Color modes:
  - **1970s:** about 10 kHz bandwidth (one Valhalla announcement says 8 kHz), and it **downsamples internally** to reproduce the artifacts of a low sample rate.
  - **1980s:** full sample rate, but still dark, with "noisy" modulation artifacts. One announcement says 15 kHz.
  - **NOW:** clean modulation with no added color.
- **[D]** ValhallaRoom's Dark Room mode is deliberately low-fi: nothing above 11 kHz, and a low starting echo density.
- **Lesson:** a band-limited, slightly dirty tail sits *behind* the mix. The flaws of old hardware are part of why people love it, and Valhalla lets you choose how much of them you get.

## 6. Stereo

- **[D]** Room's early and late sections are both stereo in and stereo out. Each algorithm responds differently to the left and right inputs, and how quickly energy fills the stereo image is a designed property.
- **[I]** Outputs are taken from several separate points in the tank, as in Dattorro, so the left and right channels are wide and different from each other. It's tuned so it doesn't collapse badly when summed to mono.

## 7. Controls and UX

- **[D]** On ValhallaPlate, Costello said: "as many controls as necessary to sculpt beautiful plate reverberation, but no more."
- **[D]** Plate's Depth control trades realism for smoothness: at 0% it sounds most like a real plate, and higher values reduce metallic artifacts.
- **[I]** A consistent set of controls across products:
  - Mix, PreDelay, Decay, Size
  - Early/Late Diffusion
  - Mod Rate / Mod Depth
  - Bass Multiplier / crossover, high shelf
  - Low/High Cut
  - Mode selector
- **[I]** All controls fit on one screen with no hidden pages. Knob ranges are scaled musically, and Decay doesn't interact with Size.
- **[I]** Parameter changes are smoothed, so there is no zipper noise. In the delay- and FDN-based plugins, changing Size live produces a tape-like pitch bend that sounds intentional.
- **[I]** Low diffusion settings are useful on purpose: they give grainy, echoey vintage textures.

## 8. Engineering quality

- **[I/D]** CPU use is very low, so you can run dozens of instances. Narcissus is documented as Room's lightest mode.
- **[I]** It stays stable at very long or infinite decay, with no blow-ups or denormal CPU spikes (tiny numbers that slow the CPU down). It sounds the same at 44.1 kHz and 96 kHz. Output level stays roughly constant as you change settings.
- **[D, secondhand]** Supermassive's newer modes use "very fast attack and filtered exponential delay". That lets them stay dense but transparent even at extreme settings.

## 9. Why it works in a mix (psychoacoustics, [I])

These are the effects of everything above that people actually hear:

1. **Smooth tails:** no metallic ringing and no flutter. It aims for an idealized space, not a realistic one.
2. **Density without harshness:** transients don't splatter, and the tail builds up evenly.
3. **The tail darkens over time,** so it doesn't mask the dry signal.
4. **The tail sounds alive** because of modulation, without obvious chorusing.
5. **Wide but mono-safe.**
6. **Controls behave predictably,** so you get a good sound in seconds.

The goal is not realism. It's the reverb people remember from records.

## 10. Reasons outside the DSP

- About $50 per plugin, no iLok or dongle, and free updates for life.
- Free plugins (Supermassive, FreqEcho, SpaceModulator) bring in a huge number of users.
- Years of free updates that add new modes, plus an educational blog.
- One consistent voice, from a single designer with a clear taste.

---

## What this means for OmegaReverb

**Things we need to get right:**

1. A recirculating tank (allpass loop or FDN) with decay that is accurate, independent of Size, and set per frequency band.
2. Random or chaotic modulation on separate delay lines, with the interpolation method chosen on purpose.
3. A diffuse early section with no short allpasses, which can feed the late tail.
4. True stereo, with outputs taken from several points in the tank.
5. Optional band-limiting / lower internal sample rate as a "color" control.
6. Several distinct designs rather than one engine with presets.
7. A small, musical set of controls with full parameter smoothing.
8. Low CPU, stable at any decay, and the same sound at every sample rate.

**How we can measure ourselves against Valhalla:**

- **Echo-density buildup:** run an impulse through both and see how quickly the echoes thicken.
- **Decay per frequency band:** measure how long each band takes to fall away.
- **Hidden resonances:** use long FFTs and spectrograms of the impulse response to spot ringing frequencies.
- **Stereo width:** measure how similar the left and right channels are (interaural cross-correlation).
- **Mono-sum check:** confirm it holds up when summed to mono.
- **Blind A/B listening:** on drums, vocals, piano and pads.

**Open question:** what should make Omega different from Valhalla? Matching it is the baseline. Being different is the reason to build it.

## Further reading

Costello's posts on valhalladsp.com: his reverb diffusion post, "Early Reflections versus Early Energy", "The Physics and Psychophysics of Plates", "ValhallaPlate: On Diffusion", "The Philosophy of ValhallaSupermassive" and the Supermassive modes post.

## Sources

- [Valhalla VintageVerb](https://valhalladsp.com/?p=60)
- [VintageVerb 3.0 (gearnews)](https://www.gearnews.com/valhalla-vintageverb-3-0-expanded-compatibility-and-a-new-algorithm/)
- [VintageVerb 1.7.1 Chaotic modes (KVR)](https://www.kvraudio.com/forum/viewtopic.php?p=5191283)
- [ValhallaRoom: Unnatural Hall Reverbs](https://valhalladsp.com/2011/05/27/valhallaroom-tips-and-tricks-unnatural-hall-reverbs/)
- [ValhallaRoom: Early Reflections vs Early Energy](https://valhalladsp.com/?p=746)
- [Valhalla Room](https://valhalladsp.com/?p=20)
- [The Philosophy of ValhallaSupermassive](https://valhalladsp.com/?p=427377)
- [Supermassive: The Modes](https://valhalladsp.com/2020/05/07/valhallasupermassive-the-modes/)
- [Supermassive 2.0 (synthanatomy)](https://synthanatomy.com/2022/06/valhallasupermassive-2-0-update-with-two-new-reverb-modes-aquarius-and-pisces.html)
- [Valhalla Plate](https://valhalladsp.com/?p=69730)
- [ValhallaPlate review (Audiofanzine)](https://en.audiofanzine.com/algorithmic-reverb/Valhalla-DSP/valhallaplate/editorial/reviews/what-s-on-your-plate.html)
- [What are your favorite non-Valhalla reverbs (KVR)](https://www.kvraudio.com/forum/viewtopic.php?p=5527046)
