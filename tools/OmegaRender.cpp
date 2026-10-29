// Offline impulse-response renderer and analyser for the OmegaReverb engine.
// Renders the wet response to a left and a right impulse, writes them as WAVs and prints objective
// measurements: decay per octave band, echo density buildup, stereo spread, resonance prominence and CPU cost.
//
// Usage: OmegaRender [name=value ...]
//   names: the plugin parameter IDs (decay, size, modDepth, ...), plus rate (Hz) and out (folder for the WAVs)

#include "dsp/HallReverb.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <random>
#include <string>
#include <vector>

#if defined(_M_X64) || defined(__x86_64__)
 #include <xmmintrin.h>
#endif

namespace
{
using omega::HallParams;

constexpr double kTwoPi = 6.283185307179586;

struct Stereo
{
    std::vector<float> l, r;
};

const std::map<std::string, float HallParams::*> kParamNames {
    { "mix", &HallParams::mix },             { "predelay", &HallParams::predelayMs }, { "decay", &HallParams::decaySec },
    { "size", &HallParams::size },           { "earlyDiff", &HallParams::earlyDiff }, { "lateDiff", &HallParams::lateDiff },
    { "modRate", &HallParams::modRateHz },   { "modDepth", &HallParams::modDepth },   { "bassMult", &HallParams::bassMult },
    { "bassXover", &HallParams::bassXover }, { "highMult", &HallParams::highMult },   { "highXover", &HallParams::highXover },
    { "lowCut", &HallParams::lowCut },       { "highCut", &HallParams::highCut },
};

Stereo renderImpulse (const HallParams& params, double rate, int length, int channel)
{
    omega::HallReverb reverb;
    reverb.prepare (rate);
    reverb.setParams (params);

    Stereo out { std::vector<float> ((size_t) length, 0.0f), std::vector<float> ((size_t) length, 0.0f) };
    (channel == 0 ? out.l : out.r)[0] = 1.0f;

    constexpr int block = 512;
    for (int pos = 0; pos < length; pos += block)
        reverb.process (out.l.data() + pos, out.r.data() + pos, std::min (block, length - pos));

    return out;
}

bool writeWav (const std::string& path, const Stereo& s, int rate)
{
    std::ofstream f (path, std::ios::binary);
    if (! f)
        return false;

    const auto frames = (uint32_t) s.l.size();
    const uint32_t dataBytes = frames * 2 * 4;
    auto u32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto u16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };

    f.write ("RIFF", 4); u32 (36 + dataBytes); f.write ("WAVE", 4);
    f.write ("fmt ", 4); u32 (16); u16 (3); u16 (2); u32 ((uint32_t) rate); u32 ((uint32_t) rate * 8); u16 (8); u16 (32);
    f.write ("data", 4); u32 (dataBytes);

    for (uint32_t i = 0; i < frames; ++i)
    {
        f.write ((const char*) &s.l[i], 4);
        f.write ((const char*) &s.r[i], 4);
    }

    return (bool) f;
}

// Reverberation time from the Schroeder energy decay curve: a line fitted between -5 and -35 dB, extrapolated to -60 dB.
// Returns a negative value if the curve never reaches -35 dB.
double reverbTime (const std::vector<double>& energy, double rate)
{
    std::vector<double> edc (energy.size());
    double acc = 0.0;
    for (size_t i = energy.size(); i-- > 0;)
        edc[i] = acc += energy[i];

    if (edc.empty() || edc[0] <= 0.0)
        return -1.0;

    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = 0;
    bool reached = false;

    for (size_t i = 0; i < edc.size(); ++i)
    {
        const double db = 10.0 * std::log10 (std::max (edc[i] / edc[0], 1e-30));
        if (db > -5.0)
            continue;
        if (db < -35.0) { reached = true; break; }

        const double t = (double) i / rate;
        sx += t; sy += db; sxx += t * t; sxy += t * db; ++n;
    }

    if (! reached || n < 2)
        return -1.0;

    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    return -60.0 / slope;
}

std::vector<double> bandEnergy (const Stereo& s, double rate, double centreHz)
{
    omega::Biquad fl, fr;
    fl.set (omega::Biquad::Type::bandPass, (float) centreHz, (float) rate, 1.414f);
    fr = fl;

    std::vector<double> e (s.l.size());
    for (size_t i = 0; i < e.size(); ++i)
    {
        const double l = fl.process (s.l[i]), r = fr.process (s.r[i]);
        e[i] = l * l + r * r;
    }
    return e;
}

size_t onsetIndex (const std::vector<float>& h)
{
    float peak = 0.0f;
    for (float v : h) peak = std::max (peak, std::abs (v));
    for (size_t i = 0; i < h.size(); ++i)
        if (std::abs (h[i]) > peak * 1e-3f)
            return i;
    return 0;
}

// Normalised echo density (Abel & Huang): fraction of samples outside one standard deviation in a
// Hann-weighted window, relative to Gaussian noise. ~1 means the response is as dense as noise.
double echoDensity (const std::vector<float>& h, double rate, size_t centre, double windowMs = 20.0)
{
    const int half = (int) (windowMs * 0.001 * rate * 0.5);
    double wsum = 0.0, sq = 0.0;

    for (int j = -half; j <= half; ++j)
    {
        const auto i = (long long) centre + j;
        if (i < 0 || i >= (long long) h.size()) continue;
        const double w = 0.5 + 0.5 * std::cos (kTwoPi * 0.5 * j / half);
        wsum += w;
        sq += w * (double) h[(size_t) i] * h[(size_t) i];
    }

    if (wsum <= 0.0 || sq <= 0.0)
        return 0.0;

    const double sd = std::sqrt (sq / wsum);
    double outside = 0.0;

    for (int j = -half; j <= half; ++j)
    {
        const auto i = (long long) centre + j;
        if (i < 0 || i >= (long long) h.size()) continue;
        if (std::abs (h[(size_t) i]) > sd)
            outside += 0.5 + 0.5 * std::cos (kTwoPi * 0.5 * j / half);
    }

    return outside / wsum / 0.3173105;   // erfc (1 / sqrt 2)
}

void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }

    for (size_t len = 2; len <= n; len <<= 1)
    {
        const std::complex<double> wl (std::cos (-kTwoPi / (double) len), std::sin (-kTwoPi / (double) len));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

// Welch power spectrum of the late tail, each segment normalised to unit power so the decaying envelope doesn't weight it
std::vector<double> tailSpectrum (const std::vector<const std::vector<float>*>& signals, size_t start, size_t end, size_t n)
{
    std::vector<double> power (n / 2 + 1, 0.0);
    std::vector<std::complex<double>> buf (n);

    for (const auto* sig : signals)
        for (size_t seg = start; seg + n <= std::min (end, sig->size()); seg += n / 2)
        {
            for (size_t i = 0; i < n; ++i)
                buf[i] = (*sig)[seg + i] * (0.5 - 0.5 * std::cos (kTwoPi * (double) i / (double) n));

            fft (buf);
            double total = 0.0;
            for (size_t i = 0; i <= n / 2; ++i) total += std::norm (buf[i]);
            if (total <= 0.0) continue;
            for (size_t i = 0; i <= n / 2; ++i) power[i] += std::norm (buf[i]) / total;
        }

    return power;
}

// What the engine's own in-loop filters should produce at this frequency (same model as the plugin's display)
double expectedRt (const HallParams& p, double hz)
{
    return omega::HallReverb::rt60At (p, (float) hz);
}
} // namespace

int main (int argc, char** argv)
{
#if defined(_M_X64) || defined(__x86_64__)
    _mm_setcsr (_mm_getcsr() | 0x8040);   // flush-to-zero + denormals-are-zero, as ScopedNoDenormals does in the plugin
#endif

    HallParams params;
    double rate = 48000.0;
    std::string outDir = ".";

    for (int a = 1; a < argc; ++a)
    {
        const std::string arg = argv[a];
        const auto eq = arg.find ('=');
        if (eq == std::string::npos) { std::printf ("Expected name=value, got '%s'\n", arg.c_str()); return 1; }

        const auto key = arg.substr (0, eq), value = arg.substr (eq + 1);
        if (key == "rate") rate = std::stod (value);
        else if (key == "out") outDir = value;
        else if (auto it = kParamNames.find (key); it != kParamNames.end()) params.*(it->second) = std::stof (value);
        else { std::printf ("Unknown parameter '%s'\n", key.c_str()); return 1; }
    }

    params.mix = 100.0f;   // measure the wet signal only

    std::printf ("OmegaRender  rate=%.0f\n", rate);
    for (const auto& [name, field] : kParamNames)
        std::printf ("  %-10s %g\n", name.c_str(), params.*field);

    const double maxRt = params.decaySec * std::max (params.bassMult, 1.0f);
    const int length = (int) (rate * std::min (60.0, params.predelayMs * 0.001 + maxRt * 1.3 + 0.5));

    const Stereo irL = renderImpulse (params, rate, length, 0);
    const Stereo irR = renderImpulse (params, rate, length, 1);

    const bool wroteL = writeWav (outDir + "/ir_left_in.wav", irL, (int) rate);
    const bool wroteR = writeWav (outDir + "/ir_right_in.wav", irR, (int) rate);
    std::printf ("\nWAVs: %s\n", wroteL && wroteR ? (outDir + "/ir_left_in.wav, ir_right_in.wav").c_str() : "FAILED to write");

    // Health
    bool finite = true;
    float peak = 0.0f;
    for (const auto* s : { &irL.l, &irL.r, &irR.l, &irR.r })
        for (float v : *s) { finite = finite && std::isfinite (v); peak = std::max (peak, std::abs (v)); }
    std::printf ("\n[health] finite=%s  peak=%.1f dBFS\n", finite ? "yes" : "NO", 20.0 * std::log10 (std::max (peak, 1e-12f)));

    // Decay per band, both impulse responses averaged
    std::printf ("\n[decay] RT60 from T30 (expected = the engine's filter model at the band centre)\n");
    {
        std::vector<double> e (irL.l.size());
        for (size_t i = 0; i < e.size(); ++i)
            e[i] = (double) irL.l[i] * irL.l[i] + (double) irL.r[i] * irL.r[i] + (double) irR.l[i] * irR.l[i] + (double) irR.r[i] * irR.r[i];
        std::printf ("  broadband  %6.2f s\n", reverbTime (e, rate));

        for (double hz : { 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0 })
        {
            auto el = bandEnergy (irL, rate, hz);
            const auto er = bandEnergy (irR, rate, hz);
            for (size_t i = 0; i < el.size(); ++i) el[i] += er[i];
            std::printf ("  %5.0f Hz   %6.2f s   (expected ~%.2f)\n", hz, reverbTime (el, rate), expectedRt (params, hz));
        }
    }

    // Echo density buildup (left output, left impulse)
    const size_t onset = onsetIndex (irL.l);
    std::printf ("\n[density] normalised echo density from first arrival (1.0 = as dense as noise)\n ");
    for (double ms : { 5.0, 10.0, 20.0, 50.0, 100.0, 200.0, 400.0 })
        std::printf (" %3.0fms=%.2f", ms, echoDensity (irL.l, rate, onset + (size_t) (ms * 0.001 * rate)));
    {
        double mixing = -1.0;
        for (double ms = 1.0; ms < 1000.0; ms += 1.0)
            if (echoDensity (irL.l, rate, onset + (size_t) (ms * 0.001 * rate)) >= 0.9) { mixing = ms; break; }
        std::printf ("\n  time to reach 0.9: %s\n", mixing < 0 ? "not within 1 s" : (std::to_string ((int) mixing) + " ms").c_str());
    }

    // Stereo spread
    std::printf ("\n[stereo] (0 correlation = fully decorrelated; mono-sum change 0 dB = no cancellation)\n");
    for (const auto& [label, ir] : { std::pair { "left in ", &irL }, std::pair { "right in", &irR } })
    {
        double ll = 0, rr = 0, lr = 0;
        for (size_t i = onset; i < ir->l.size(); ++i)
        {
            ll += (double) ir->l[i] * ir->l[i];
            rr += (double) ir->r[i] * ir->r[i];
            lr += (double) ir->l[i] * ir->r[i];
        }
        const double balance = 10.0 * std::log10 (ll / rr);
        std::printf ("  %s  L/R correlation %+.3f   mono-sum change %+.2f dB   L-R balance %+.1f dB\n",
                     label, lr / std::sqrt (ll * rr), 10.0 * std::log10 ((ll + rr + 2.0 * lr) / (ll + rr)), balance);
    }

    // Resonances: narrow peaks standing out of the 1/3-octave-smoothed tail spectrum
    {
        constexpr size_t n = 8192;
        const size_t start = onset + (size_t) (0.2 * rate);
        const size_t end = onset + (size_t) (std::min (maxRt * 0.6, 4.0) * rate) + n;
        const auto power = tailSpectrum ({ &irL.l, &irL.r, &irR.l, &irR.r }, start, end, n);
        const double binHz = rate / (double) n;

        std::vector<double> prominence (power.size(), 0.0);
        std::vector<double> prefix (power.size() + 1, 0.0);
        for (size_t i = 0; i < power.size(); ++i) prefix[i + 1] = prefix[i] + power[i];

        for (size_t i = 1; i < power.size(); ++i)
        {
            const auto lo = (size_t) std::max (1.0, i / std::pow (2.0, 1.0 / 6.0));
            const auto hi = std::min (power.size() - 1, (size_t) (i * std::pow (2.0, 1.0 / 6.0)) + 1);
            const double smooth = (prefix[hi + 1] - prefix[lo]) / (double) (hi + 1 - lo);
            if (power[i] > 0.0 && smooth > 0.0)
                prominence[i] = 10.0 * std::log10 (power[i] / smooth);
        }

        std::vector<std::pair<double, double>> peaks;   // (prominence dB, Hz)
        const auto minBin = (size_t) (80.0 / binHz), maxBin = std::min (power.size() - 4, (size_t) (std::min (12000.0, (double) params.highCut) / binHz));
        for (size_t i = std::max<size_t> (minBin, 3); i < maxBin; ++i)
        {
            bool isMax = true;
            for (size_t j = i - 3; j <= i + 3; ++j)
                if (j != i && prominence[j] >= prominence[i]) { isMax = false; break; }
            if (isMax) peaks.push_back ({ prominence[i], (double) i * binHz });
        }

        std::sort (peaks.rbegin(), peaks.rend());
        std::printf ("\n[resonance] strongest narrow peaks in the late tail (>3 dB suggests ringing)\n ");
        for (size_t i = 0; i < std::min<size_t> (5, peaks.size()); ++i)
            std::printf (" %.0fHz:+%.1fdB", peaks[i].second, peaks[i].first);
        std::printf ("\n");
    }

    // Periodicity: autocorrelation of the envelope-flattened tail. Recirculation that the diffusion fails to hide
    // shows up as peaks at the loop period; a noise-like tail stays below ~0.05.
    {
        const size_t start = onset + (size_t) (0.1 * rate);
        const size_t span = std::min ((size_t) (std::min (maxRt * 0.8, 4.0) * rate), irL.l.size() - start);
        const auto win = (size_t) (0.02 * rate);

        std::vector<double> flat (span);
        for (size_t i = 0; i < span; i += win)
        {
            const size_t end = std::min (span, i + win);
            double sq = 0.0;
            for (size_t j = i; j < end; ++j) sq += (double) irL.l[start + j] * irL.l[start + j];
            const double rms = std::sqrt (sq / (double) (end - i)) + 1e-20;
            for (size_t j = i; j < end; ++j) flat[j] = irL.l[start + j] / rms;
        }

        size_t n = 1;
        while (n < span * 2) n <<= 1;
        std::vector<std::complex<double>> buf (n);
        for (size_t i = 0; i < span; ++i) buf[i] = flat[i];
        fft (buf);
        for (auto& v : buf) v = std::norm (v);
        for (auto& v : buf) v = std::conj (v);   // inverse FFT via conjugation
        fft (buf);

        const double zeroLag = buf[0].real();
        std::vector<std::pair<double, double>> peaks;   // (coefficient, lag ms)
        const auto minLag = (size_t) (0.01 * rate), maxLag = std::min (span / 2, (size_t) (1.5 * rate));
        for (size_t lag = minLag + 1; lag + 1 < maxLag; ++lag)
        {
            // unbiased: divide by the overlap length
            auto coef = [&] (size_t l) { return buf[l].real() / zeroLag * (double) span / (double) (span - l); };
            const double c = coef (lag);
            if (c > coef (lag - 1) && c >= coef (lag + 1))
                peaks.push_back ({ c, (double) lag / rate * 1000.0 });
        }

        std::sort (peaks.rbegin(), peaks.rend());
        std::printf ("\n[periodicity] strongest tail self-similarity (noise-like < 0.05)\n ");
        for (size_t i = 0; i < std::min<size_t> (3, peaks.size()); ++i)
            std::printf (" %.0fms:%.3f", peaks[i].second, peaks[i].first);
        std::printf ("\n");
    }

    // CPU: 10 s of stereo noise through the full plugin path (dry/wet mix included)
    {
        const int cpuLength = (int) (rate * 10.0);
        Stereo noise { std::vector<float> ((size_t) cpuLength), std::vector<float> ((size_t) cpuLength) };
        std::mt19937 rng (1);
        std::uniform_real_distribution<float> dist (-0.5f, 0.5f);
        for (int i = 0; i < cpuLength; ++i) { noise.l[(size_t) i] = dist (rng); noise.r[(size_t) i] = dist (rng); }

        omega::HallReverb reverb;
        reverb.prepare (rate);
        HallParams cpuParams = params;
        cpuParams.mix = 35.0f;
        reverb.setParams (cpuParams);

        const auto t0 = std::chrono::steady_clock::now();
        for (int pos = 0; pos < cpuLength; pos += 512)
            reverb.process (noise.l.data() + pos, noise.r.data() + pos, std::min (512, cpuLength - pos));
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();

        std::printf ("\n[cpu] %.2f%% of one core at %.0f Hz (%.0fx realtime)\n", secs / 10.0 * 100.0, rate, 10.0 / secs);
    }

    return finite ? 0 : 2;
}
