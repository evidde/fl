#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

namespace
{
const juce::Colour kPink (0xffff7eb6), kCyan (0xff7fe3ff), kLilac (0xffb79cff), kBlack (0xff050408);

juce::Font jp (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions ("Yu Gothic UI", h, bold ? juce::Font::bold : juce::Font::plain));
}
juce::Font ui (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}
juce::String J (const char* utf8) { return juce::String::fromUTF8 (utf8); }

// draw image cropped to fill dest, centred on (fx,fy) in 0..1, zoomed
void drawCover (juce::Graphics& g, const juce::Image& im, juce::Rectangle<int> d, float fx, float fy, float zoom)
{
    if (! im.isValid() || d.isEmpty()) return;
    const float iw = (float) im.getWidth(), ih = (float) im.getHeight();
    float sw = iw / zoom, sh = sw * (float) d.getHeight() / (float) d.getWidth();
    if (sh > ih) { sh = ih; sw = sh * (float) d.getWidth() / (float) d.getHeight(); }
    const float sx = juce::jlimit (0.f, iw - sw, fx * iw - sw / 2.f);
    const float sy = juce::jlimit (0.f, ih - sh, fy * ih - sh / 2.f);

    juce::Path clip;
    clip.addRoundedRectangle (d.toFloat(), 10.f);
    g.saveState();
    g.reduceClipRegion (clip);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (im, d.getX(), d.getY(), d.getWidth(), d.getHeight(),
                 (int) sx, (int) sy, (int) sw, (int) sh);
    g.restoreState();
    g.setColour (kLilac.withAlpha (0.6f));
    g.drawRoundedRectangle (d.toFloat(), 10.f, 1.5f);
}

void drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto b = r.toFloat();
    g.setColour (juce::Colour (0xff030206));
    g.drawRoundedRectangle (b.expanded (4), 20.f, 3.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17111f), b.getX(), b.getY(),
                                             juce::Colour (0xff0a0710), b.getRight(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 16.f);
    g.setColour (kPink.withAlpha (0.8f));
    g.drawRoundedRectangle (b, 16.f, 1.5f);
    g.setColour (kPink);
    g.fillEllipse (b.getX() + 12, b.getY() - 3, 6, 6);
    g.setColour (kCyan);
    g.fillEllipse (b.getRight() - 18, b.getY() - 3, 6, 6);
    g.setColour (kPink);
    g.setFont (jp (15.f, true));
    g.drawText (title, r.getX() + 16, r.getY() + 6, r.getWidth() - 32, 22, juce::Justification::left);
}

float shapeAt (int wave, float t)
{
    switch (wave)
    {
        case 0:  return 2.f * t - 1.f;
        case 1:  return t < 0.5f ? 1.f : -1.f;
        case 2:  return t < 0.5f ? 4.f * t - 1.f : 3.f - 4.f * t;
        default: return std::sin (6.2831853f * t);
    }
}
}

//==============================================================================
IesuLNF::IesuLNF()
{
    setColour (juce::Slider::textBoxTextColourId, kCyan);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xfffff1f6));
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xfffff1f6));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff2a0b22));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff08060d));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xfffff1f6));
    setColour (juce::ComboBox::outlineColourId, kPink);
    setColour (juce::ComboBox::arrowColourId, kPink);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0d0912));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xfffff1f6));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kPink);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff2a0b22));
}

void IesuLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) / 2.f;
    const float ang = startAngle + pos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2f1d4d));
    g.strokePath (track, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (pos > 0.005f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, r, r, 0.f, startAngle, ang, true);
        g.setColour (kPink);
        g.strokePath (val, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float ri = r * 0.72f;
    g.setColour (kBlack);
    g.fillEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2);
    g.setColour (juce::Colour (0xff2f1d4d));
    g.drawEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2, 2.f);

    const float sx = std::sin (ang), sy = -std::cos (ang);
    g.setColour (kCyan);
    g.drawLine (c.x + sx * ri * 0.25f, c.y + sy * ri * 0.25f, c.x + sx * ri * 0.9f, c.y + sy * ri * 0.9f, 3.f);
}

void IesuLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    g.setColour (on ? kPink : juce::Colour (0xff08060d));
    g.fillRoundedRectangle (r, 9.f);
    g.setColour (on || hi ? kPink : kLilac);
    g.drawRoundedRectangle (r, 9.f, 1.6f);
}

juce::Font IesuLNF::getTextButtonFont (juce::TextButton&, int) { return ui (13.f, true); }

//==============================================================================
void ScopeComp::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff040307));
    g.fillRoundedRectangle (b, 10.f);
    g.setColour (juce::Colour (0xff22153a));
    for (int i = 1; i < 14; ++i) g.drawVerticalLine ((int) (b.getWidth() * i / 14.f), 0.f, b.getHeight());
    for (int i = 1; i < 6; ++i)  g.drawHorizontalLine ((int) (b.getHeight() * i / 6.f), 0.f, b.getWidth());

    constexpr int N = IesuProcessor::scopeSize;
    float snap[N];
    const int w = proc.scopeWrite.load();
    for (int i = 0; i < N; ++i) snap[i] = proc.scopeBuf[(w + i) & (N - 1)];

    int s = 0;
    while (s < N / 2 && ! (snap[s] < 0.f && snap[s + 1] >= 0.f)) ++s;
    float pk = 0.05f;
    for (int i = 0; i < N / 2; ++i) pk = juce::jmax (pk, std::abs (snap[s + i]));

    juce::Path path;
    const int W = (int) b.getWidth();
    for (int x = 0; x < W; ++x)
    {
        const float v = snap[s + (int) ((float) x / (float) W * (N / 2 - 1))] / pk;
        const float y = b.getHeight() * 0.5f - v * b.getHeight() * 0.42f;
        if (x == 0) path.startNewSubPath (0.f, y); else path.lineTo ((float) x, y);
    }
    g.setColour (kCyan.withAlpha (0.3f));
    g.strokePath (path, juce::PathStrokeType (5.f));
    g.setColour (kCyan);
    g.strokePath (path, juce::PathStrokeType (2.f));
}

void WaveComp::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff040307));
    g.fillRoundedRectangle (b, 10.f);
    g.setColour (juce::Colour (0xff25173a));
    for (int i = 1; i < 24; ++i) g.drawVerticalLine ((int) (b.getWidth() * i / 24.f), 0.f, b.getHeight());
    g.drawHorizontalLine ((int) (b.getHeight() / 2), 0.f, b.getWidth());

    const int wv = (int) (proc.apvts.getRawParameterValue ("wave")->load() + 0.5f);
    juce::Path path;
    const int W = (int) b.getWidth();
    for (int x = 0; x < W; ++x)
    {
        const float t = std::fmod ((float) x / (float) W * 2.f, 1.f);
        const float y = b.getHeight() * 0.5f - shapeAt (wv, t) * b.getHeight() * 0.38f;
        if (x == 0) path.startNewSubPath (0.f, y); else path.lineTo ((float) x, y);
    }
    g.setColour (kPink.withAlpha (0.3f));
    g.strokePath (path, juce::PathStrokeType (6.f));
    g.setColour (kPink);
    g.strokePath (path, juce::PathStrokeType (2.5f));
}

//==============================================================================
IesuEditor::IesuEditor (IesuProcessor& p)
    : AudioProcessorEditor (&p), proc (p), scope (p), wave (p)
{
    setLookAndFeel (&lnf);
    setSize (1000, 660);
    img = juce::ImageCache::getFromMemory (BinaryData::girl_jpg, BinaryData::girl_jpgSize);

    addAndMakeVisible (scope);
    addAndMakeVisible (wave);

    struct KS { const char* id; const char* label; const char* suffix; int dec; int panel; int idx; };
    const KS specs[] = {
        { "detune", "Detune", " ct", 1, 0, 0 }, { "mix", "Osc mix", " %", 0, 0, 1 }, { "sub", "Sub", " %", 0, 0, 2 },
        { "width", "Width", " %", 0, 0, 3 },    { "glide", "Glide", " ms", 0, 0, 4 },
        { "attack", "Attack", " ms", 0, 1, 0 }, { "decay", "Decay", " ms", 0, 1, 1 }, { "sustain", "Sustain", " %", 0, 1, 2 },
        { "release", "Release", " ms", 0, 1, 3 }, { "cutoff", "Cutoff", " Hz", 0, 1, 4 }, { "envamt", "Env amount", " %", 0, 1, 5 },
        { "res", "Resonance", " %", 0, 2, 0 },  { "drive", "Drive", " %", 0, 2, 1 },   { "echo", "Echo", " %", 0, 2, 2 },
        { "echot", "Echo time", " ms", 0, 2, 3 }, { "space", "Space", " %", 0, 2, 4 }, { "out", "Output", " %", 0, 2, 5 },
    };
    for (const auto& s : specs)
    {
        auto k = std::make_unique<Knob>();
        k->panel = s.panel; k->index = s.idx;
        k->s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        k->s.setTextValueSuffix (s.suffix);
        k->s.setNumDecimalPlacesToDisplay (s.dec);
        k->s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k->l.setText (s.label, juce::dontSendNotification);
        k->l.setJustificationType (juce::Justification::centred);
        k->l.setFont (ui (13.f, true));
        addAndMakeVisible (k->s);
        addAndMakeVisible (k->l);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, s.id, k->s);
        knobs.push_back (std::move (k));
    }

    monoBtn.setButtonText ("MONO");
    monoBtn.setClickingTogglesState (true);
    addAndMakeVisible (monoBtn);
    monoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "mono", monoBtn);

    const char* names[] = { "SAW", "PULSE", "TRI", "SINE" };
    for (int i = 0; i < 4; ++i)
    {
        waveBtn[i].setButtonText (names[i]);
        addAndMakeVisible (waveBtn[i]);
        waveBtn[i].onClick = [this, i]
        {
            if (auto* prm = proc.apvts.getParameter ("wave"))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
                prm->endChangeGesture();
            }
        };
    }

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevBtn.setButtonText ("<");
    nextBtn.setButtonText (">");
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    const int total = proc.getNumPrograms();
    prevBtn.onClick = [this, total]
    {
        const int id = presetBox.getSelectedId();
        presetBox.setSelectedId (id <= 1 ? total : id - 1, juce::sendNotificationSync);
    };
    nextBtn.onClick = [this, total]
    {
        const int id = presetBox.getSelectedId();
        presetBox.setSelectedId (id >= total ? 1 : id + 1, juce::sendNotificationSync);
    };

    startTimerHz (30);
}

IesuEditor::~IesuEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void IesuEditor::timerCallback()
{
    scope.repaint();
    wave.repaint();
    const int idx = (int) (proc.apvts.getRawParameterValue ("wave")->load() + 0.5f);
    for (int i = 0; i < 4; ++i) waveBtn[i].setToggleState (i == idx, juce::dontSendNotification);
}

void IesuEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c0a18), 0.f, 0.f,
                                             kBlack, (float) getWidth() * 0.5f, (float) getHeight() * 0.5f, false));
    g.fillAll();
    g.setColour (juce::Colour (0xff050408).withAlpha (0.55f));
    g.fillRect (getLocalBounds());
    g.setColour (kPink.withAlpha (0.10f));
    for (int y = 0; y < getHeight(); y += 8)
        for (int x = ((y / 8) % 2) * 4; x < getWidth(); x += 8)
            if ((x + y) < 420 || (x + y) > getWidth() + getHeight() - 420) g.fillEllipse ((float) x, (float) y, 1.8f, 1.8f);

    drawPanel (g, rTop, {});
    g.setFont (ui (40.f, true));
    g.setColour (kPink);
    g.drawText ("iesu", rTop.getX() + 23, rTop.getY() + 3, 200, 50, juce::Justification::left);
    g.setColour (juce::Colours::white);
    g.drawText ("iesu", rTop.getX() + 20, rTop.getY(), 200, 50, juce::Justification::left);
    g.setColour (kCyan);
    g.setFont (jp (12.f));
    g.drawText (J ("シンセサイザー"), rTop.getX() + 160, rTop.getY() + 22, 150, 20, juce::Justification::left);

    drawPanel (g, rLeft,  J ("ヒロイン"));
    drawPanel (g, rScope, J ("出力"));
    drawPanel (g, rRight, J ("表情"));
    drawPanel (g, rOsc,   J ("オシレーター"));
    drawPanel (g, rP[0],  J ("ボイス"));
    drawPanel (g, rP[1],  J ("フィルター / エンベロープ"));
    drawPanel (g, rP[2],  J ("カラー + FX"));

    auto inner = [] (juce::Rectangle<int> r) { return r.reduced (10).withTrimmedTop (22); };
    drawCover (g, img, inner (rLeft),  0.5f,  0.35f, 1.0f);
    drawCover (g, img, inner (rRight), 0.53f, 0.27f, 2.3f);

    g.setColour (juce::Colour (0xffb9a3d6));
    g.setFont (ui (11.f));
    g.drawText ("double-click a knob = reset", 15, getHeight() - 22, 400, 16, juce::Justification::left);
    g.drawText ("iesu v0.1", getWidth() - 215, getHeight() - 22, 200, 16, juce::Justification::right);
}

void IesuEditor::resized()
{
    rTop   = { 15, 10, 970, 58 };
    rLeft  = { 15, 78, 250, 200 };
    rScope = { 275, 78, 450, 200 };
    rRight = { 735, 78, 250, 200 };
    rOsc   = { 15, 288, 970, 112 };
    for (int i = 0; i < 3; ++i) rP[i] = { 15 + i * 327, 410, 316, 226 };

    presetBox.setBounds (rTop.getX() + 330, rTop.getY() + 14, 380, 30);
    prevBtn.setBounds   (rTop.getX() + 720, rTop.getY() + 14, 40, 30);
    nextBtn.setBounds   (rTop.getX() + 765, rTop.getY() + 14, 40, 30);

    scope.setBounds (rScope.reduced (10).withTrimmedTop (22));
    wave.setBounds  (rOsc.getX() + 12, rOsc.getY() + 32, rOsc.getWidth() - 24, rOsc.getHeight() - 42);
    for (int i = 0; i < 4; ++i)
        waveBtn[i].setBounds (rOsc.getRight() - 12 - (4 - i) * 78, rOsc.getY() + 5, 72, 24);

    for (auto& k : knobs)
    {
        const auto& r = rP[k->panel];
        const int cw = r.getWidth() / 3, ch = (r.getHeight() - 34) / 2;
        const int cx = r.getX() + (k->index % 3) * cw, cy = r.getY() + 32 + (k->index / 3) * ch;
        k->l.setBounds (cx, cy, cw, 16);
        k->s.setBounds (cx + (cw - 80) / 2, cy + 16, 80, 78);
    }
    {
        const auto& r = rP[0];
        const int cw = r.getWidth() / 3, ch = (r.getHeight() - 34) / 2;
        monoBtn.setBounds (r.getX() + 2 * cw + (cw - 76) / 2, r.getY() + 32 + ch + 28, 76, 32);
    }
}
