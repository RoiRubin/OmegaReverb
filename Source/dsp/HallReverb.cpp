#include "dsp/HallReverb.h"

#include <cmath>

namespace omega
{
namespace
{
    // Base lengths are in samples at Dattorro's 29761 Hz reference rate, rescaled to the host rate
    constexpr float kDesignRate = 29761.0f;

    constexpr float kInputDiffLen[2][4] = { { 142.0f, 107.0f, 379.0f, 277.0f },
                                            { 151.0f, 113.0f, 367.0f, 289.0f } };
    constexpr float kInputDiffCoef[4] = { 0.75f, 0.75f, 0.625f, 0.625f };

    // Dattorro's tank plus one extra diffuser after each of his tank allpasses (apX1, apX2)
    struct HalfLengths { float ap1, apX1, d1, ap2, apX2, d2; };
    constexpr HalfLengths kTank[2] = { { 672.0f, 1213.0f, 4453.0f, 1800.0f, 1577.0f, 3720.0f },
                                       { 908.0f, 1381.0f, 4217.0f, 2656.0f, 1873.0f, 3163.0f } };

    constexpr float kAp1Coef  = -0.70f;   // scaled by Late Diffusion
    constexpr float kApX1Coef = 0.55f;
    constexpr float kAp2Coef  = 0.50f;
    constexpr float kApX2Coef = -0.45f;

    constexpr float kMaxScale      = 2.0f;
    constexpr float kDiffuserKnee  = 0.6f;   // diffusers follow Size fully up to this long-delay multiplier
    constexpr float kDiffuserSlope = 0.3f;   // ...and as scale^0.3 above it
    constexpr float kMaxPredelayMs = 500.0f;
    constexpr float kAllpassModMs  = 0.6f;   // ap1 excursion at 100 % depth
    constexpr float kDelayModMs    = 0.3f;   // d2 excursion at 100 % depth
    constexpr float kModOvershoot  = 1.3f;   // RandomMod can overshoot [-1, 1] slightly
    constexpr float kOutputGain    = 0.6f;
    constexpr float kModRateSpread[2][2] = { { 1.0f, 0.77f }, { 1.13f, 0.89f } };

    enum Element { D1, AP2, D2 };
    struct Tap { int half; Element element; float pos; float sign; };

    // Dattorro's output taps, in his samples (half 0 = his left tank, half 1 = his right tank).
    // Two taps share B.d1 (L) and A.d1 (R), which correlates the tail with itself at their spacing.
    constexpr Tap kTapsL[] = { { 1, D1, 266.0f, 1.0f },   { 1, D1, 2974.0f, 1.0f }, { 1, AP2, 1913.0f, -1.0f },
                               { 1, D2, 1996.0f, 1.0f },  { 0, D1, 1990.0f, -1.0f }, { 0, AP2, 187.0f, -1.0f },
                               { 0, D2, 1066.0f, -1.0f } };
    constexpr Tap kTapsR[] = { { 0, D1, 353.0f, 1.0f },   { 0, D1, 3627.0f, 1.0f }, { 0, AP2, 1228.0f, -1.0f },
                               { 0, D2, 2673.0f, 1.0f },  { 1, D1, 2111.0f, -1.0f }, { 1, AP2, 335.0f, -1.0f },
                               { 1, D2, 121.0f, -1.0f } };

    // Decorrelated taps, positions as fractions of the line. Only plain delays are read (an allpass's internal line
    // is a feedback comb, which repeats at its length), and every line at most once per channel.
    constexpr Tap kWideTapsL[] = { { 1, D1, 0.37f, 1.0f },  { 1, D2, 0.53f, 1.0f },
                                   { 0, D1, 0.71f, -1.0f }, { 0, D2, 0.19f, -1.0f } };
    constexpr Tap kWideTapsR[] = { { 0, D1, 0.23f, 1.0f },  { 0, D2, 0.64f, 1.0f },
                                   { 1, D1, 0.83f, -1.0f }, { 1, D2, 0.13f, -1.0f } };
    constexpr float kWideTapGain = kOutputGain * 1.32f;   // 7 -> 4 taps: sqrt (7 / 4) keeps the level matched

    constexpr float kWideTapStart = 0.5f, kWideTapEnd = 1.2f;   // long-delay multiplier range of the blend

    float decayGain (float loopSamples, float rt60, float sampleRate)
    {
        return std::pow (10.0f, -3.0f * loopSamples / (std::max (rt60, 0.05f) * sampleRate));
    }

    // Each half damps in two stages (after d1 and after d2), each for its own share of the loop, so the decay
    // happens in steps half as long and half as deep; at large sizes one step per half was audible.
    float stageALength (const HalfLengths& h, float k, float kd) { return (h.ap1 + h.apX1) * kd + h.d1 * k; }
    float stageBLength (const HalfLengths& h, float k, float kd) { return (h.ap2 + h.apX2) * kd + h.d2 * k; }

    float elementLength (const HalfLengths& h, Element e) { return e == D1 ? h.d1 : (e == AP2 ? h.ap2 : h.d2); }

    void setDecay (BandDecay& decay, const HallParams& p, float loopLen, float fs)
    {
        decay.set (decayGain (loopLen, p.decaySec * p.bassMult, fs), decayGain (loopLen, p.decaySec, fs),
                   decayGain (loopLen, p.decaySec * p.highMult, fs), p.bassXover, p.highXover, fs);
    }
} // namespace

float HallReverb::sizeToScale (float sizePercent)
{
    return 0.25f * std::pow (8.0f, std::clamp (sizePercent, 0.0f, 100.0f) * 0.01f);
}

float HallReverb::diffuserScale (float scale)
{
    return scale <= kDiffuserKnee ? scale : kDiffuserKnee * std::pow (scale / kDiffuserKnee, kDiffuserSlope);
}

float HallReverb::wideTapWeight (float scale)
{
    const float t = std::clamp ((scale - kWideTapStart) / (kWideTapEnd - kWideTapStart), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

float HallReverb::rt60At (const HallParams& p, float hz)
{
    constexpr float fs = 48000.0f;
    const float scale = sizeToScale (p.size);
    const float k = scale * fs / kDesignRate;
    const float kd = diffuserScale (scale) * fs / kDesignRate;

    // Average over both halves of: loop length and the combined gain of the two damping stages
    float loopLen = 0.0f, logMag = 0.0f;
    for (const auto& half : kTank)
    {
        BandDecay a, b;
        setDecay (a, p, stageALength (half, k, kd), fs);
        setDecay (b, p, stageBLength (half, k, kd), fs);
        loopLen += 0.5f * (stageALength (half, k, kd) + stageBLength (half, k, kd));
        logMag += 0.5f * std::log10 (std::clamp (a.magnitudeAt (hz, fs) * b.magnitudeAt (hz, fs), 1e-6f, 0.999999f));
    }

    return -3.0f * loopLen / (fs * logMag);
}

float HallReverb::wetToneDbAt (const HallParams& p, float hz)
{
    constexpr float fs = 48000.0f;
    Biquad low, high;
    low.set (Biquad::Type::highPass, p.lowCut, fs);
    high.set (Biquad::Type::lowPass, p.highCut, fs);
    return 20.0f * std::log10 (std::max (low.magnitudeAt (hz, fs) * high.magnitudeAt (hz, fs), 1e-6f));
}

void HallReverb::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    const float fs = (float) sampleRate;
    fsRatio = fs / kDesignRate;

    const int predelayMax = (int) std::ceil (kMaxPredelayMs * 0.001f * fs) + 8;
    preL.allocate (predelayMax);
    preR.allocate (predelayMax);

    for (int ch = 0; ch < 2; ++ch)
        for (int j = 0; j < 4; ++j)
            inputDiff[(size_t) ch][(size_t) j].line.allocate ((int) std::ceil (kInputDiffLen[ch][j] * fsRatio) + 8);

    const float maxK = kMaxScale * fsRatio;
    const int apExc = (int) std::ceil (kAllpassModMs * 0.001f * fs * kModOvershoot);
    const int dlExc = (int) std::ceil (kDelayModMs * 0.001f * fs * kModOvershoot);
    auto lengthFor = [maxK] (float base) { return (int) std::ceil (base * maxK) + 8; };

    for (int h = 0; h < 2; ++h)
    {
        auto& half = tank[(size_t) h];
        const auto& len = kTank[h];
        half.ap1.line.allocate (lengthFor (len.ap1) + apExc);
        half.apX1.line.allocate (lengthFor (len.apX1));
        half.d1.line.allocate (lengthFor (len.d1));
        half.ap2.line.allocate (lengthFor (len.ap2));
        half.apX2.line.allocate (lengthFor (len.apX2));
        half.d2.line.allocate (lengthFor (len.d2) + dlExc);
        half.mod1.seed (0x2545F491u + 7919u * (uint32_t) h);
        half.mod2.seed (0x68E31DA4u + 104729u * (uint32_t) h);
    }

    scaleSm.setTime (80.0f, fs);
    predelaySm.setTime (80.0f, fs);
    mixSm.setTime (20.0f, fs);
    depthSm.setTime (50.0f, fs);
    earlySm.setTime (20.0f, fs);
    lateSm.setTime (20.0f, fs);

    lastLowCut = lastHighCut = -1.0f;
    snapSmoothers = true;
    updateTargets();
    reset();
}

void HallReverb::reset()
{
    preL.clear();
    preR.clear();

    for (auto& chain : inputDiff)
        for (auto& ap : chain)
            ap.line.clear();

    for (auto& half : tank)
    {
        for (auto* ap : { &half.ap1, &half.apX1, &half.ap2, &half.apX2 })
            ap->line.clear();
        half.d1.line.clear();
        half.d2.line.clear();
        half.decayA.reset();
        half.decayB.reset();
        half.out = 0.0f;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        lowCut[(size_t) ch].reset();
        highCut[(size_t) ch].reset();
    }
}

void HallReverb::setParams (const HallParams& newParams)
{
    params = newParams;
    updateTargets();
}

void HallReverb::updateTargets()
{
    const float fs = (float) sampleRate;
    targetScale    = sizeToScale (params.size);
    targetPredelay = std::clamp (params.predelayMs * 0.001f * fs, 1.0f, kMaxPredelayMs * 0.001f * fs);
    targetMix      = std::clamp (params.mix * 0.01f, 0.0f, 1.0f);
    targetDepth    = std::clamp (params.modDepth * 0.01f, 0.0f, 1.0f);
    targetEarly    = std::clamp (params.earlyDiff * 0.01f, 0.0f, 1.0f);
    targetLate     = std::clamp (params.lateDiff * 0.01f, 0.0f, 1.0f);
}

void HallReverb::updateBlockState()
{
    const float fs = (float) sampleRate;

    if (snapSmoothers)
    {
        scaleSm.value    = targetScale;
        predelaySm.value = targetPredelay;
        mixSm.value      = targetMix;
        depthSm.value    = targetDepth;
        earlySm.value    = targetEarly;
        lateSm.value     = targetLate;
        snapSmoothers = false;
    }

    // Loop gains follow the current (smoothed) size so the decay time stays put while Size moves
    const float k = scaleSm.value * fsRatio;
    const float kd = diffuserScale (scaleSm.value) * fsRatio;

    for (int h = 0; h < 2; ++h)
    {
        auto& half = tank[(size_t) h];
        // An allpass's average group delay equals its delay length, so the allpasses count towards the loop time
        setDecay (half.decayA, params, stageALength (kTank[h], k, kd), fs);
        setDecay (half.decayB, params, stageBLength (kTank[h], k, kd), fs);

        half.mod1.setRate (params.modRateHz * kModRateSpread[h][0], fs);
        half.mod2.setRate (params.modRateHz * kModRateSpread[h][1], fs);
    }

    if (params.lowCut != lastLowCut)
    {
        for (auto& f : lowCut)
            f.set (Biquad::Type::highPass, params.lowCut, fs);
        lastLowCut = params.lowCut;
    }

    if (params.highCut != lastHighCut)
    {
        for (auto& f : highCut)
            f.set (Biquad::Type::lowPass, params.highCut, fs);
        lastHighCut = params.highCut;
    }
}

float HallReverb::runHalf (int h, float x, float k, float kd, float apExc, float dlExc, float late)
{
    auto& half = tank[(size_t) h];
    const auto& len = kTank[h];

    float y = half.ap1.process (x, len.ap1 * kd + half.mod1.process() * apExc, kAp1Coef * late);
    y = half.apX1.process (y, len.apX1 * kd, kApX1Coef * late);
    y = half.d1.process (y, len.d1 * k);
    y = half.decayA.process (y);
    y = half.ap2.process (y, len.ap2 * kd, kAp2Coef * late);
    y = half.apX2.process (y, len.apX2 * kd, kApX2Coef * late);
    y = half.d2.process (y, len.d2 * k + half.mod2.process() * dlExc);
    return half.decayB.process (y);
}

void HallReverb::process (float* left, float* right, int numSamples)
{
    updateBlockState();

    const float fs = (float) sampleRate;
    const float apExcPerDepth = kAllpassModMs * 0.001f * fs;
    const float dlExcPerDepth = kDelayModMs * 0.001f * fs;

    auto lineOf = [this] (const Tap& tap) -> const DelayLine&
    {
        const auto& half = tank[(size_t) tap.half];
        return tap.element == D1 ? half.d1.line : (tap.element == AP2 ? half.ap2.line : half.d2.line);
    };

    // Lengths along each element: delays follow k, the tapped allpass (ap2) follows kd
    auto readDattorro = [&] (const auto& taps, float k, float kd)
    {
        float sum = 0.0f;
        for (const auto& tap : taps)
            sum += tap.sign * lineOf (tap).read (tap.pos * (tap.element == AP2 ? kd : k));
        return sum;
    };

    auto readWide = [&] (const auto& taps, float k, float kd)
    {
        float sum = 0.0f;
        for (const auto& tap : taps)
            sum += tap.sign * lineOf (tap).read (tap.pos * elementLength (kTank[tap.half], tap.element) * (tap.element == AP2 ? kd : k));
        return sum;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const float scale    = scaleSm.process (targetScale);
        const float k        = scale * fsRatio;
        const float kd       = diffuserScale (scale) * fsRatio;
        const float wide     = wideTapWeight (scale);
        const float predelay = predelaySm.process (targetPredelay);
        const float mix      = mixSm.process (targetMix);
        const float depth    = depthSm.process (targetDepth);
        const float early    = earlySm.process (targetEarly);
        const float late     = lateSm.process (targetLate);

        const float dryL = left[i];
        const float dryR = right[i];

        preL.write (dryL);
        preR.write (dryR);
        float x[2] = { preL.read (predelay), preR.read (predelay) };

        for (int ch = 0; ch < 2; ++ch)
            for (int j = 0; j < 4; ++j)
                x[ch] = inputDiff[(size_t) ch][(size_t) j].process (x[ch], kInputDiffLen[ch][j] * fsRatio, kInputDiffCoef[j] * early);

        // The left input drives half 1, whose taps dominate the left output (and vice versa),
        // so a source keeps its side of the image as the tail builds
        const float in0 = x[1] + tank[1].out;
        const float in1 = x[0] + tank[0].out;
        const float apExc = depth * apExcPerDepth;
        const float dlExc = depth * dlExcPerDepth;
        tank[0].out = runHalf (0, in0, k, kd, apExc, dlExc, late);
        tank[1].out = runHalf (1, in1, k, kd, apExc, dlExc, late);

        float tapL = 0.0f, tapR = 0.0f;
        if (wide < 1.0f)
        {
            tapL += (1.0f - wide) * kOutputGain * readDattorro (kTapsL, k, kd);
            tapR += (1.0f - wide) * kOutputGain * readDattorro (kTapsR, k, kd);
        }
        if (wide > 0.0f)
        {
            tapL += wide * kWideTapGain * readWide (kWideTapsL, k, kd);
            tapR += wide * kWideTapGain * readWide (kWideTapsR, k, kd);
        }

        const float wetL = highCut[0].process (lowCut[0].process (tapL));
        const float wetR = highCut[1].process (lowCut[1].process (tapR));

        left[i]  = dryL + mix * (wetL - dryL);
        right[i] = dryR + mix * (wetR - dryR);
    }
}
} // namespace omega
