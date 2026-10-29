#pragma once

#include <algorithm>
#include <cmath>
#include <complex>

namespace omega
{
constexpr float kPi = 3.14159265358979f;

struct OnePoleLowPass
{
    float a = 1.0f, z = 0.0f;

    void setCutoff (float hz, float sampleRate) { a = 1.0f - std::exp (-2.0f * kPi * hz / sampleRate); }
    float process (float x) { return z += a * (x - z); }
    void reset() { z = 0.0f; }
};

// RBJ biquad, transposed direct form II
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    enum class Type { lowPass, highPass, bandPass };

    void set (Type type, float hz, float sampleRate, float q = 0.70710678f)
    {
        hz = std::clamp (hz, 5.0f, 0.45f * sampleRate);
        const float w0 = 2.0f * kPi * hz / sampleRate;
        const float c = std::cos (w0);
        const float alpha = std::sin (w0) / (2.0f * q);
        const float a0 = 1.0f + alpha;

        switch (type)
        {
            case Type::lowPass:  b0 = 0.5f * (1.0f - c); b1 = 1.0f - c;    b2 = b0; break;
            case Type::highPass: b0 = 0.5f * (1.0f + c); b1 = -(1.0f + c); b2 = b0; break;
            case Type::bandPass: b0 = alpha;             b1 = 0.0f;        b2 = -alpha; break;
        }

        b0 /= a0; b1 /= a0; b2 /= a0;
        a1 = -2.0f * c / a0;
        a2 = (1.0f - alpha) / a0;
    }

    // Shelf with slope S = 1: the steepest (12 dB/oct) transition that has no overshoot
    void setShelf (bool high, float hz, float sampleRate, float gainDb)
    {
        hz = std::clamp (hz, 5.0f, 0.45f * sampleRate);
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = 2.0f * kPi * hz / sampleRate;
        const float c = std::cos (w0);
        const float beta = 2.0f * std::sqrt (A) * std::sin (w0) / std::sqrt (2.0f);
        const float s = high ? -1.0f : 1.0f;   // the high shelf mirrors the low shelf's cos terms

        const float a0 = (A + 1.0f) + s * (A - 1.0f) * c + beta;
        b0 = A * ((A + 1.0f) - s * (A - 1.0f) * c + beta) / a0;
        b1 = s * 2.0f * A * ((A - 1.0f) - s * (A + 1.0f) * c) / a0;
        b2 = A * ((A + 1.0f) - s * (A - 1.0f) * c - beta) / a0;
        a1 = -s * 2.0f * ((A - 1.0f) + s * (A + 1.0f) * c) / a0;
        a2 = ((A + 1.0f) + s * (A - 1.0f) * c - beta) / a0;
    }

    float magnitudeAt (float hz, float sampleRate) const
    {
        const auto z1c = std::polar (1.0f, -2.0f * kPi * hz / sampleRate);
        const auto z2c = z1c * z1c;
        return std::abs ((b0 + b1 * z1c + b2 * z2c) / (1.0f + a1 * z1c + a2 * z2c));
    }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() { z1 = z2 = 0.0f; }
};

// Frequency-dependent loop gain: gMid, with a low shelf to gLow below the bass crossover and a high shelf to gHigh
// above the high crossover. S = 1 shelves never overshoot, so the gain stays within [gHigh, gLow] and the loop stable.
struct BandDecay
{
    Biquad lowShelf, highShelf;
    float gMid = 0.0f;

    void set (float gLow, float newGMid, float gHigh, float bassXover, float highXover, float sampleRate)
    {
        gMid = newGMid;
        lowShelf.setShelf (false, bassXover, sampleRate, 20.0f * std::log10 (gLow / gMid));
        highShelf.setShelf (true, highXover, sampleRate, 20.0f * std::log10 (gHigh / gMid));
    }

    float magnitudeAt (float hz, float sampleRate) const
    {
        return gMid * lowShelf.magnitudeAt (hz, sampleRate) * highShelf.magnitudeAt (hz, sampleRate);
    }

    float process (float x) { return gMid * highShelf.process (lowShelf.process (x)); }
    void reset() { lowShelf.reset(); highShelf.reset(); }
};

// One-pole parameter smoother
struct Smoother
{
    float value = 0.0f, coef = 1.0f;

    void setTime (float ms, float sampleRate) { coef = 1.0f - std::exp (-1.0f / (ms * 0.001f * sampleRate)); }
    float process (float target) { return value += coef * (target - value); }
};
} // namespace omega
