#pragma once

#include <algorithm>
#include <vector>

namespace omega
{
// Circular buffer with a power-of-two size. Reads are "samples ago" relative to the most recent write.
class DelayLine
{
public:
    void allocate (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 4)
            size <<= 1;

        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); }

    void write (float x)
    {
        writePos = (writePos + 1) & mask;
        buffer[(size_t) writePos] = x;
    }

    float readInt (int samplesAgo) const { return buffer[(size_t) ((writePos - samplesAgo) & mask)]; }

    // 4-point Hermite interpolation; samplesAgo must be >= 1
    float read (float samplesAgo) const
    {
        const int i = (int) samplesAgo;
        const float f = samplesAgo - (float) i;

        const float xm1 = readInt (i - 1);
        const float x0  = readInt (i);
        const float x1  = readInt (i + 1);
        const float x2  = readInt (i + 2);

        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }

private:
    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
};

// Plain delay; delay must be >= 2 samples
struct Delay
{
    DelayLine line;

    float process (float x, float delay)
    {
        const float y = line.read (delay - 1.0f);
        line.write (x);
        return y;
    }
};

// Schroeder allpass, H(z) = (z^-D - g) / (1 - g z^-D); delay must be >= 2 samples.
// The line holds the internal (w) signal, which is what the reverb's output taps read.
struct Allpass
{
    DelayLine line;

    float process (float x, float delay, float g)
    {
        const float delayed = line.read (delay - 1.0f);
        const float w = x + g * delayed;
        line.write (w);
        return delayed - g * w;
    }
};
} // namespace omega
