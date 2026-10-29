#pragma once

#include "dsp/DelayLine.h"
#include "dsp/Filters.h"
#include "dsp/RandomMod.h"

#include <array>

namespace omega
{
// Parameter values in the plugin's user-facing units; the defaults are the plugin's defaults
struct HallParams
{
    float mix        = 35.0f;     // %
    float predelayMs = 10.0f;
    float decaySec   = 2.5f;      // RT60 of the mid band
    float size       = 60.0f;     // %
    float earlyDiff  = 75.0f;     // %
    float lateDiff   = 70.0f;     // %
    float modRateHz  = 0.6f;
    float modDepth   = 40.0f;     // %
    float bassMult   = 1.2f;      // low-band RT60 = decay * bassMult
    float bassXover  = 300.0f;    // Hz
    float highMult   = 0.5f;      // high-band RT60 = decay * highMult
    float highXover  = 4500.0f;   // Hz
    float lowCut     = 60.0f;     // Hz, on the wet output
    float highCut    = 10000.0f;  // Hz, on the wet output
};

// Modulated hall: true-stereo input diffusers feeding a Dattorro/Lexicon-style figure-8 allpass tank,
// with random-modulated allpasses and delays and three-band shelving decay inside the loop.
// Size stretches the long delays fully but the diffusing allpasses only gently, so big spaces stay dense.
// The output blends from Dattorro's taps (small sizes) to a decorrelated tap set (large sizes), because
// Dattorro reads some lines twice, which leaves an audible repeat once the delays get long.
class HallReverb
{
public:
    void prepare (double sampleRate);
    void reset();
    void setParams (const HallParams& newParams);

    // In place; the output is the dry/wet mix
    void process (float* left, float* right, int numSamples);

    // Size % -> long-delay length multiplier (0.25x .. 2x, exponential)
    static float sizeToScale (float sizePercent);
    // Long-delay multiplier -> diffusing allpass multiplier (equal up to mid sizes, then growing much more slowly)
    static float diffuserScale (float scale);
    // Weight of the decorrelated output taps for a long-delay multiplier (0 = Dattorro's taps)
    static float wideTapWeight (float scale);

    // For displays: RT60 at a frequency, from the actual in-loop band filters, and the wet Low/High Cut response
    static float rt60At (const HallParams& p, float hz);
    static float wetToneDbAt (const HallParams& p, float hz);

private:
    // Signal path: ap1 (modulated) -> apX1 -> d1 -> decayA -> ap2 -> apX2 -> d2 (modulated) -> decayB
    struct TankHalf
    {
        Allpass ap1, apX1, ap2, apX2;
        Delay d1, d2;
        BandDecay decayA, decayB;
        RandomMod mod1, mod2;
        float out = 0.0f;
    };

    float runHalf (int h, float x, float k, float kd, float apExc, float dlExc, float late);
    void updateTargets();
    void updateBlockState();

    double sampleRate = 48000.0;
    float fsRatio = 1.0f;
    HallParams params;

    float targetScale = 1.0f, targetPredelay = 1.0f, targetMix = 0.0f;
    float targetDepth = 0.0f, targetEarly = 0.0f, targetLate = 0.0f;
    bool snapSmoothers = true;
    Smoother scaleSm, predelaySm, mixSm, depthSm, earlySm, lateSm;

    DelayLine preL, preR;
    std::array<std::array<Allpass, 4>, 2> inputDiff;
    std::array<TankHalf, 2> tank;
    std::array<Biquad, 2> lowCut, highCut;
    float lastLowCut = -1.0f, lastHighCut = -1.0f;
};
} // namespace omega
