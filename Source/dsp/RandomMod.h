#pragma once

#include <cstdint>

namespace omega
{
// Smooth random modulation source: a Catmull-Rom curve through random points in [-1, 1].
// Each segment's length is jittered so the motion never settles into an audible periodic pattern.
// Output can overshoot [-1, 1] slightly (up to ~1.25).
class RandomMod
{
public:
    void seed (uint32_t s)
    {
        state = s != 0 ? s : 0x9E3779B9u;
        p0 = nextPoint(); p1 = nextPoint(); p2 = nextPoint(); p3 = nextPoint();
        phase = 0.0f;
        jitter = nextJitter();
    }

    void setRate (float hz, float sampleRate) { baseInc = hz / sampleRate; }

    float process()
    {
        phase += baseInc * jitter;

        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            p0 = p1; p1 = p2; p2 = p3; p3 = nextPoint();
            jitter = nextJitter();
        }

        const float t = phase;
        return 0.5f * (2.0f * p1
                       + (p2 - p0) * t
                       + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t
                       + (3.0f * (p1 - p2) + p3 - p0) * t * t * t);
    }

private:
    float uniform()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) (state >> 8) * (1.0f / 16777216.0f);
    }

    float nextPoint() { return uniform() * 2.0f - 1.0f; }
    float nextJitter() { return 0.75f + 0.5f * uniform(); }

    uint32_t state = 0x9E3779B9u;
    float p0 = 0.0f, p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
    float phase = 0.0f, baseInc = 0.0f, jitter = 1.0f;
};
} // namespace omega
