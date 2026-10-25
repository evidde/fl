#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace
{
struct Preset
{
    const char* name;
    float detune, mix, sub, width, glide;
    bool mono;
    float atk, dec, sus, rel, cut, res, env, drv, echo, echot, space;
    int wave; // 0 saw, 1 pulse, 2 tri, 3 sine
};

const Preset kPresets[] = {
    { "Factory",                21.8f, 88, 4,  92,  7,   false, 2,   190,  86, 69,   10263, 12, 10, 30, 19, 334, 16, 0 },
    { "Sakura Pad",             28,    90, 0,  92,  0,   false, 420, 600,  80, 900,  3200,  8,  20, 5,  30, 334, 55, 0 },
    { "Moon Lead",              12,    60, 6,  92,  25,  false, 2,   260,  70, 180,  9000,  25, 35, 20, 35, 380, 25, 1 },
    { "Pixie Bass",             4,     30, 60, 92,  30,  false, 2,   200,  50, 90,   900,   35, 55, 55, 0,  334, 4,  0 },
    { "Bell",                   7,     50, 0,  92,  7,   false, 2,   900,  0,  1400, 12000, 10, 10, 0,  40, 260, 50, 3 },
    { "Tokyo Night",            35,    95, 5,  92,  7,   false, 8,   500,  55, 400,  2600,  20, 40, 15, 28, 330, 30, 0 },
    { "Opening Lead (mono)",    25,    85, 10, 92,  40,  true,  2,   190,  86, 220,  11000, 18, 10, 35, 25, 300, 25, 0 },
    { "Seiyuu Glide (mono)",    18,    70, 15, 92,  120, true,  6,   190,  86, 260,  7000,  30, 10, 25, 20, 334, 30, 1 },
    { "Kawaii Pluck",           8,     50, 4,  92,  7,   false, 1,   140,  0,  120,  5000,  15, 60, 5,  30, 250, 20, 1 },
    { "Chip Tune",              0,     0,  0,  0,   7,   false, 1,   80,   70, 40,   16000, 0,  10, 10, 0,  334, 0,  1 },
    { "Dark Sakura Pad",        45,    100,20, 100, 7,   false, 900, 1200, 70, 1800, 1400,  15, 30, 10, 25, 520, 80, 0 },
    { "Lo-Fi Bass (mono)",      3,     20, 45, 92,  60,  true,  3,   300,  60, 120,  700,   10, 10, 30, 0,  334, 5,  2 },
    { "Judo Stab",              20,    80, 4,  92,  7,   false, 1,   120,  0,  100,  4000,  40, 80, 40, 20, 240, 15, 0 },
    { "Moon E-Piano",           4,     60, 15, 92,  7,   false, 2,   900,  20, 700,  6000,  8,  10, 8,  18, 340, 40, 3 },
    { "Yamato Flute (mono)",    6,     30, 0,  92,  120, true,  80,  300,  80, 300,  5000,  5,  10, 0,  25, 380, 40, 3 },
    { "Shibuya Rain Pad",       30,    90, 4,  92,  7,   false, 600, 1500, 60, 1500, 1800,  25, 50, 5,  40, 600, 90, 0 },
    { "Henkei Chime",           12,    45, 0,  100, 7,   false, 1,   1200, 0,  1600, 14000, 12, 15, 0,  45, 300, 55, 2 },
    { "Jazz Rhodes",            6,     40, 10, 92,  0,   false, 3,   700,  35, 600,  3800,  5,  25, 12, 22, 320, 35, 2 },
    { "Koto",                   3,     25, 0,  92,  0,   false, 2,   500,  0,  1200, 6000,  15, 40, 0,  30, 300, 45, 0 },
};
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

inline float blep (float t, float dt)
{
    if (t < dt)       { t /= dt;           return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}
constexpr float kTwoPi = 6.2831853f;
}

juce::AudioProcessorValueTreeState::ParameterLayout IesuProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, float skew = 1.f)
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                                                      NormalisableRange<float> (lo, hi, 0.01f, skew), def));
    };
    F ("detune",  "Detune",     0,   100,   21.8f);
    F ("mix",     "Osc mix",    0,   100,   88);
    F ("sub",     "Sub",        0,   100,   4);
    F ("width",   "Width",      0,   100,   92);
    F ("glide",   "Glide",      0,   500,   7);
    F ("attack",  "Attack",     1,   2000,  2,     0.4f);
    F ("decay",   "Decay",      5,   2000,  190,   0.4f);
    F ("sustain", "Sustain",    0,   100,   86);
    F ("release", "Release",    5,   3000,  69,    0.4f);
    F ("cutoff",  "Cutoff",     40,  18000, 10263, 0.3f);
    F ("res",     "Resonance",  0,   100,   12);
    F ("envamt",  "Env amount", 0,   100,   10);
    F ("drive",   "Drive",      0,   100,   30);
    F ("echo",    "Echo",       0,   100,   19);
    F ("echot",   "Echo time",  30,  900,   334);
    F ("space",   "Space",      0,   100,   16);
    F ("out",     "Output",     0,   100,   62);
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "mono", 1 }, "Mono", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "wave", 1 }, "Wave",
                                                   StringArray { "Saw", "Pulse", "Tri", "Sine" }, 0));
    return l;
}

IesuProcessor::IesuProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool IesuProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void IesuProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    const int n = (int) (sr * 2.0) + 8;
    dl0.assign ((size_t) n, 0.f);
    dl1.assign ((size_t) n, 0.f);
    wp = 0;
    dsm = apvts.getRawParameterValue ("echot")->load() * 0.001f * (float) sr;
    reverb.setSampleRate (sr);
    reverb.reset();
    allOff();
    for (auto& v : voices) v.active = false;
}

void IesuProcessor::updateParams()
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    p.detune = g ("detune"); p.mix = g ("mix"); p.sub = g ("sub"); p.width = g ("width"); p.glide = g ("glide");
    p.atk = g ("attack"); p.dec = g ("decay"); p.sus = g ("sustain") / 100.f; p.rel = g ("release");
    p.cut = g ("cutoff"); p.res = g ("res"); p.env = g ("envamt");
    p.drv = g ("drive"); p.echo = g ("echo"); p.echot = g ("echot"); p.space = g ("space"); p.out = g ("out");
    p.mono = g ("mono") > 0.5f;
    p.wave = (int) (g ("wave") + 0.5f);

    const float fs = (float) sr;
    attInc    = 1.f / (juce::jmax (1.f, p.atk) * 0.001f * fs);
    decCoef   = 1.f - std::exp (-1.f / (p.dec * 0.001f / 3.f * fs));
    relCoef   = std::exp (-1.f / (p.rel * 0.001f / 3.f * fs));
    glideCoef = p.glide > 0.5f ? 1.f - std::exp (-1.f / (p.glide * 0.001f / 2.5f * fs)) : 1.f;

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.75f; rp.damping = 0.5f; rp.width = 1.f; rp.freezeMode = 0.f;
    rp.dryLevel = 0.5f;                       // JUCE Reverb scales dry x2 => unity
    rp.wetLevel = p.space / 100.f * 0.15f;    // wet x3
    reverb.setParameters (rp);
}

float IesuProcessor::osc (double& ph, float dt) const
{
    const float t = (float) ph;
    float y;
    switch (p.wave)
    {
        case 0:  y = 2.f * t - 1.f - blep (t, dt); break;
        case 1:  y = (t < 0.5f ? 1.f : -1.f) + blep (t, dt) - blep (std::fmod (t + 0.5f, 1.f), dt); break;
        case 2:  y = t < 0.5f ? 4.f * t - 1.f : 3.f - 4.f * t; break;
        default: y = std::sin (kTwoPi * t); break;
    }
    ph += dt;
    if (ph >= 1.0) ph -= 1.0;
    return y;
}

void IesuProcessor::startVoice (Voice& v, int note, float f0, float f)
{
    const bool was = v.active;
    v.active = true; v.gate = true; v.note = note;
    v.f = f0; v.ft = f; v.stage = 0; v.age = ++ageCounter;
    if (! was) { v.env = 0.f; v.pa = 0.0; v.pb = 0.25; v.ps = 0.0; v.fl = {}; v.fr = {}; }
    lastFreq = f;
}

void IesuProcessor::noteOn (int note)
{
    const float f = 440.f * std::pow (2.f, (float) (note - 69) / 12.f);
    const float start = (lastFreq > 0.f && p.glide > 0.5f) ? lastFreq : f;

    if (p.mono)
    {
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        monoStack.push_back (note);
        for (int i = 1; i < 8; ++i) if (voices[i].active) { voices[i].gate = false; voices[i].stage = 3; }
        Voice& v = voices[0];
        if (v.active && v.gate) { v.ft = f; v.note = note; lastFreq = f; return; } // legato
        startVoice (v, note, start, f);
        return;
    }

    Voice* v = nullptr;
    for (auto& x : voices) if (x.active && x.note == note) { v = &x; break; }
    if (v == nullptr) for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr)
    {
        float best = 1e9f;
        for (auto& x : voices)
        {
            const float score = (x.gate ? 10.f : 0.f) + x.env;
            if (score < best) { best = score; v = &x; }
        }
    }
    startVoice (*v, note, start, f);
}

void IesuProcessor::noteOff (int note)
{
    if (p.mono)
    {
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        Voice& v = voices[0];
        if (! v.active) return;
        if (! monoStack.empty())
        {
            const int n = monoStack.back();
            v.note = n;
            v.ft = 440.f * std::pow (2.f, (float) (n - 69) / 12.f);
            lastFreq = v.ft;
        }
        else if (v.gate) { v.gate = false; v.stage = 3; }
        return;
    }
    for (auto& v : voices)
        if (v.active && v.gate && v.note == note) { v.gate = false; v.stage = 3; }
}

void IesuProcessor::allOff()
{
    monoStack.clear();
    for (auto& v : voices) if (v.active) { v.gate = false; v.stage = 3; }
}

void IesuProcessor::render (float* L, float* R, int start, int num)
{
    if (num <= 0) return;
    const float fs = (float) sr;
    const float dk = 1.f + p.drv / 100.f * 9.f, dkn = 1.f / std::tanh (dk);
    const float fbk = 0.15f + p.echo / 100.f * 0.45f, wet = p.echo / 100.f * 0.7f;
    const float dtarget = p.echot * 0.001f * fs;
    const float detA = std::pow (2.f, -p.detune / 2400.f), detB = std::pow (2.f, p.detune / 2400.f);
    const float w = p.width / 100.f, pi4 = juce::MathConstants<float>::pi / 4.f;
    const float gAL = std::cos ((1.f - w) * pi4), gAR = std::sin ((1.f - w) * pi4);
    const float gBL = std::cos ((1.f + w) * pi4), gBR = std::sin ((1.f + w) * pi4);
    const float mixB = p.mix / 100.f, subG = p.sub / 100.f * 1.2f * 0.7071f;
    const float k = 1.f / (0.7f + p.res / 100.f * 12.f);
    const int dsz = (int) dl0.size();
    if (dsz < 16) return;

    for (int i = start; i < start + num; ++i)
    {
        float l = 0.f, r = 0.f;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            v.f += (v.ft - v.f) * glideCoef;

            if (v.stage == 0)      { v.env += attInc; if (v.env >= 1.f) { v.env = 1.f; v.stage = 1; } }
            else if (v.stage == 1) { v.env += (p.sus - v.env) * decCoef;
                                     if (p.sus <= 0.f && v.env < 1e-4f) { v.active = false; continue; } }
            else                   { v.env *= relCoef; if (v.env < 1e-4f) { v.active = false; continue; } }

            const float da = v.f * detA / fs, db = v.f * detB / fs, ds = v.f * 0.5f / fs;
            const float a = osc (v.pa, da);
            const float b = osc (v.pb, db) * mixB;
            const float s = std::sin (kTwoPi * (float) v.ps) * subG;
            v.ps += ds; if (v.ps >= 1.0) v.ps -= 1.0;

            const float l0 = a * gAL + b * gBL + s;
            const float r0 = a * gAR + b * gBR + s;
            const float fc = juce::jlimit (20.f, fs * 0.45f, p.cut * (1.f + p.env / 100.f * 10.f * v.env));
            const float g = std::tan (juce::MathConstants<float>::pi * fc / fs);
            l += v.fl.lp (l0, g, k) * v.env * 0.28f;
            r += v.fr.lp (r0, g, k) * v.env * 0.28f;
        }

        l = std::tanh (l * dk) * dkn;
        r = std::tanh (r * dk) * dkn;

        dsm += (dtarget - dsm) * 0.0004f;
        float rp = (float) wp - dsm;
        if (rp < 0.f) rp += (float) dsz;
        const int i0 = juce::jlimit (0, dsz - 1, (int) rp), i1 = (i0 + 1) % dsz;
        const float fr = rp - (float) i0;
        const float eL = dl0[(size_t) i0] + (dl0[(size_t) i1] - dl0[(size_t) i0]) * fr;
        const float eR = dl1[(size_t) i0] + (dl1[(size_t) i1] - dl1[(size_t) i0]) * fr;
        dl0[(size_t) wp] = l + eR * fbk;      // ping-pong
        dl1[(size_t) wp] = r + eL * fbk;
        wp = (wp + 1) % dsz;

        L[i] = l + eL * wet;
        R[i] = r + eR * wet;
    }
}

void IesuProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buf.clear();
    updateParams();

    const int n = buf.getNumSamples();
    float* L = buf.getWritePointer (0);
    float* R = buf.getWritePointer (buf.getNumChannels() > 1 ? 1 : 0);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (pos, n, meta.samplePosition);
        render (L, R, pos, t - pos);
        pos = t;
        const auto m = meta.getMessage();
        if (m.isNoteOn())                               noteOn (m.getNoteNumber());
        else if (m.isNoteOff())                         noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) allOff();
    }
    render (L, R, pos, n - pos);

    reverb.processStereo (L, R, n);

    const float og = std::pow (p.out / 100.f, 1.5f) * 0.9f;
    int w = scopeWrite.load();
    for (int i = 0; i < n; ++i)
    {
        L[i] = juce::jlimit (-1.f, 1.f, L[i] * og);
        R[i] = juce::jlimit (-1.f, 1.f, R[i] * og);
        scopeBuf[w] = 0.5f * (L[i] + R[i]);
        w = (w + 1) & (scopeSize - 1);
    }
    scopeWrite.store (w);
}

int IesuProcessor::getNumPrograms() { return kNumPresets; }

const juce::String IesuProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, kNumPresets) ? juce::String (kPresets[i].name) : juce::String();
}

void IesuProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets)) return;
    curProgram = index;
    const Preset& s = kPresets[index];
    auto set = [this] (const char* id, float v)
    {
        if (auto* pr = apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (v));
    };
    set ("detune", s.detune); set ("mix", s.mix); set ("sub", s.sub); set ("width", s.width); set ("glide", s.glide);
    set ("mono", s.mono ? 1.f : 0.f);
    set ("attack", s.atk); set ("decay", s.dec); set ("sustain", s.sus); set ("release", s.rel);
    set ("cutoff", s.cut); set ("res", s.res); set ("envamt", s.env);
    set ("drive", s.drv); set ("echo", s.echo); set ("echot", s.echot); set ("space", s.space);
    set ("out", 62.f);
    set ("wave", (float) s.wave);
}

void IesuProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", curProgram, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void IesuProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            curProgram = juce::jlimit (0, kNumPresets - 1, (int) vt.getProperty ("program", 0));
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* IesuProcessor::createEditor() { return new IesuEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new IesuProcessor(); }
