#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>

class IesuProcessor : public juce::AudioProcessor
{
public:
    IesuProcessor();
    ~IesuProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "iesu"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return curProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int scopeSize = 2048;
    float scopeBuf[scopeSize] = {};
    std::atomic<int> scopeWrite { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Svf
    {
        float s1 = 0.f, s2 = 0.f;
        float lp (float x, float g, float k)
        {
            const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            const float v3 = x - s2;
            const float v1 = a1 * s1 + a2 * v3;
            const float v2 = s2 + a2 * s1 + a3 * v3;
            s1 = 2.f * v1 - s1;
            s2 = 2.f * v2 - s2;
            return v2;
        }
    };

    struct Voice
    {
        bool active = false, gate = false;
        int note = -1, stage = 0;
        double pa = 0.0, pb = 0.25, ps = 0.0;
        float f = 440.f, ft = 440.f, env = 0.f;
        unsigned int age = 0;
        Svf fl, fr;
    };

    struct Params
    {
        float detune = 0, mix = 0, sub = 0, width = 0, glide = 0, atk = 0, dec = 0, sus = 0, rel = 0;
        float cut = 0, res = 0, env = 0, drv = 0, echo = 0, echot = 0, space = 0, out = 0;
        bool mono = false;
        int wave = 0;
    };

    void updateParams();
    void noteOn (int note);
    void noteOff (int note);
    void allOff();
    void startVoice (Voice& v, int note, float f0, float f);
    float osc (double& ph, float dt) const;
    void render (float* L, float* R, int start, int num);

    Voice voices[8];
    Params p;
    std::vector<int> monoStack;
    float lastFreq = 0.f;
    unsigned int ageCounter = 0;
    double sr = 44100.0;
    float attInc = 0.01f, decCoef = 0.001f, relCoef = 0.999f, glideCoef = 1.f;

    std::vector<float> dl0, dl1;
    int wp = 0;
    float dsm = 10000.f;
    juce::Reverb reverb;
    int curProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IesuProcessor)
};
