#pragma once
#include <JuceHeader.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

class IesuLNF : public juce::LookAndFeel_V4
{
public:
    IesuLNF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
};

class ScopeComp : public juce::Component
{
public:
    explicit ScopeComp (IesuProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    IesuProcessor& proc;
};

class WaveComp : public juce::Component
{
public:
    explicit WaveComp (IesuProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    IesuProcessor& proc;
};

class IesuEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit IesuEditor (IesuProcessor&);
    ~IesuEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider s;
        juce::Label l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        int panel = 0, index = 0;
    };

    IesuProcessor& proc;
    IesuLNF lnf;
    ScopeComp scope;
    WaveComp wave;
    juce::Image img;
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::TextButton waveBtn[4], monoBtn, prevBtn, nextBtn;
    juce::ComboBox presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAtt;
    juce::Rectangle<int> rTop, rLeft, rScope, rRight, rOsc, rP[3];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IesuEditor)
};
