/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
    constexpr auto corner = 8.0f;

    juce::Colour ink()       { return juce::Colour (0xff0f100d); }
    juce::Colour panel()     { return juce::Colour (0xff191a14); }
    juce::Colour panelHi()   { return juce::Colour (0xff25271e); }
    juce::Colour line()      { return juce::Colour (0xff383a2e); }
    juce::Colour text()      { return juce::Colour (0xfff5ecd8); }
    juce::Colour muted()     { return juce::Colour (0xff9d9a89); }
    juce::Colour ember()     { return juce::Colour (0xffff6b35); }
    juce::Colour honey()     { return juce::Colour (0xffffc857); }
    juce::Colour cyan()      { return juce::Colour (0xff33d6c3); }
    juce::Colour leaf()      { return juce::Colour (0xffa6e85b); }
    juce::Colour coral()     { return juce::Colour (0xffff4d74); }

    juce::Colour colourForName (const juce::String& name)
    {
        if (name == "DRIVE")  return ember();
        if (name == "TONE")   return cyan();
        if (name == "BIAS")   return coral();
        if (name == "MIX")    return leaf();
        if (name == "OUTPUT") return honey();

        return muted();
    }

    void drawSoftPanel (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour fill, float radius = corner)
    {
        g.setColour (juce::Colours::black.withAlpha (0.34f));
        g.fillRoundedRectangle (area.translated (0.0f, 5.0f), radius);

        juce::ColourGradient gradient (fill.brighter (0.08f), area.getX(), area.getY(),
                                       fill.darker (0.28f), area.getX(), area.getBottom(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (area, radius);

        g.setColour (juce::Colours::white.withAlpha (0.055f));
        g.drawRoundedRectangle (area.reduced (1.0f), radius, 1.0f);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawRoundedRectangle (area, radius, 1.0f);
    }

    void drawShearMark (juce::Graphics& g, juce::Rectangle<float> area)
    {
        juce::ColourGradient tile (juce::Colour (0xff20251f), area.getX(), area.getY(),
                                   juce::Colour (0xff10110d), area.getRight(), area.getBottom(), false);
        g.setGradientFill (tile);
        g.fillRoundedRectangle (area, 10.0f);

        g.setColour (line().withAlpha (0.65f));
        g.drawRoundedRectangle (area, 10.0f, 1.1f);

        auto mark = area.reduced (9.0f);
        juce::Path leftWave;
        leftWave.startNewSubPath (mark.getX(), mark.getCentreY() + 1.0f);
        leftWave.lineTo (mark.getX() + mark.getWidth() * 0.22f, mark.getY() + 5.0f);
        leftWave.lineTo (mark.getCentreX() - 4.0f, mark.getCentreY() - 3.0f);

        juce::Path rightWave;
        rightWave.startNewSubPath (mark.getCentreX() + 5.0f, mark.getCentreY() + 7.0f);
        rightWave.lineTo (mark.getX() + mark.getWidth() * 0.77f, mark.getBottom() - 5.0f);
        rightWave.lineTo (mark.getRight(), mark.getCentreY() + 1.0f);

        g.setColour (cyan().withAlpha (0.24f));
        g.strokePath (leftWave, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (honey().withAlpha (0.22f));
        g.strokePath (rightWave, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (cyan());
        g.strokePath (leftWave, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (honey());
        g.strokePath (rightWave, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path cut;
        cut.startNewSubPath (mark.getCentreX() + 8.0f, mark.getY() - 3.0f);
        cut.lineTo (mark.getCentreX() + 17.0f, mark.getY() - 3.0f);
        cut.lineTo (mark.getCentreX() - 7.0f, mark.getBottom() + 3.0f);
        cut.lineTo (mark.getCentreX() - 16.0f, mark.getBottom() + 3.0f);
        cut.closeSubPath();

        g.setColour (coral().withAlpha (0.34f));
        g.fillPath (cut);

        juce::Path edge;
        edge.startNewSubPath (mark.getCentreX() + 12.5f, mark.getY() - 2.0f);
        edge.lineTo (mark.getCentreX() - 11.5f, mark.getBottom() + 2.0f);

        g.setColour (coral());
        g.strokePath (edge, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

ForgeLookAndFeel::ForgeLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, ember());
    setColour (juce::Slider::textBoxTextColourId, text());
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff10110d));
    setColour (juce::Slider::textBoxOutlineColourId, line().withAlpha (0.45f));
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::ComboBox::backgroundColourId, panel());
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::arrowColourId, honey());
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff171811));
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ember().withAlpha (0.82f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff120c08));
    setColour (juce::Label::textColourId, text());
}

void ForgeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPosProportional, float rotaryStartAngle,
                                         float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (4.0f);
    const auto isDrive = slider.getName() == "DRIVE";
    const auto accent = colourForName (slider.getName());
    const auto centre = bounds.getCentre();
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - (isDrive ? 16.0f : 8.0f);
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto stroke = isDrive ? 8.0f : 5.5f;

    g.setColour (juce::Colours::black.withAlpha (0.36f));
    g.fillEllipse (bounds.withSizeKeepingCentre (radius * 2.35f, radius * 2.35f).translated (0.0f, 5.0f));

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (line().withAlpha (0.62f));
    g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path active;
    active.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent.withAlpha (0.95f));
    g.strokePath (active, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (isDrive)
    {
        g.setColour (accent.withAlpha (0.09f + sliderPosProportional * 0.18f));
        g.fillEllipse (bounds.withSizeKeepingCentre (radius * 2.52f, radius * 2.52f));
    }

    auto inner = bounds.withSizeKeepingCentre (radius * (isDrive ? 1.48f : 1.58f),
                                               radius * (isDrive ? 1.48f : 1.58f));
    juce::ColourGradient knob (panelHi(), inner.getX(), inner.getY(),
                               ink(), inner.getRight(), inner.getBottom(), false);
    knob.addColour (0.42, juce::Colour (0xff1d1f17));
    g.setGradientFill (knob);
    g.fillEllipse (inner);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillEllipse (inner.reduced (inner.getWidth() * 0.18f).translated (-inner.getWidth() * 0.08f,
                                                                       -inner.getHeight() * 0.10f));

    g.setColour (line().withAlpha (0.85f));
    g.drawEllipse (inner, 1.2f);

    auto pointer = centre.getPointOnCircumference (radius * 0.56f, angle);
    g.setColour (accent);
    g.fillEllipse (juce::Rectangle<float> (pointer.x - 3.6f, pointer.y - 3.6f, 7.2f, 7.2f));

    if (slider.hasKeyboardFocus (false))
    {
        g.setColour (text().withAlpha (0.68f));
        g.drawEllipse (inner.expanded (4.0f), 1.4f);
    }
}

void ForgeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted,
                                         bool shouldDrawButtonAsDown)
{
    auto area = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto active = button.getToggleState();
    const auto accent = active ? cyan() : muted();

    drawSoftPanel (g, area, active ? juce::Colour (0xff17322e) : panel(), 8.0f);

    auto pill = area.reduced (9.0f, 10.0f).removeFromLeft (39.0f);
    g.setColour (juce::Colour (0xff070806));
    g.fillRoundedRectangle (pill, pill.getHeight() * 0.5f);

    auto puck = pill.withWidth (pill.getHeight()).translated (active ? pill.getWidth() - pill.getHeight() : 0.0f, 0.0f);
    g.setColour (accent.withAlpha (active ? 1.0f : 0.5f));
    g.fillEllipse (puck.reduced (3.0f));

    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
    {
        g.setColour (accent.withAlpha (0.16f));
        g.fillRoundedRectangle (area, 8.0f);
    }

    g.setColour (text());
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (44),
                      juce::Justification::centred, 1);
}

void ForgeLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                     int buttonX, int buttonY, int buttonW, int buttonH,
                                     juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH, box);

    auto area = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
    drawSoftPanel (g, area, panel(), 8.0f);

    g.setColour ((isButtonDown ? ember() : honey()).withAlpha (0.88f));
    g.fillRoundedRectangle (area.removeFromLeft (4.0f), 2.0f);

    juce::Path arrow;
    const auto cx = static_cast<float> (width - 18);
    const auto cy = static_cast<float> (height) * 0.5f;
    arrow.addTriangle (cx - 5.0f, cy - 3.0f, cx + 5.0f, cy - 3.0f, cx, cy + 4.0f);
    g.setColour (honey());
    g.fillPath (arrow);
}

juce::Font ForgeLookAndFeel::getLabelFont (juce::Label& label)
{
    juce::ignoreUnused (label);
    return juce::Font (juce::FontOptions (12.5f, juce::Font::bold));
}

TransferCurveDisplay::TransferCurveDisplay (NewProjectAudioProcessor& processor)
    : audioProcessor (processor)
{
}

juce::Colour TransferCurveDisplay::getModeColour (int mode) const
{
    switch (mode)
    {
        case 1:  return coral();
        case 2:  return cyan();
        case 3:  return leaf();
        default: return ember();
    }
}

juce::String TransferCurveDisplay::getModeName (int mode) const
{
    switch (mode)
    {
        case 1:  return "HARD";
        case 2:  return "FOLD";
        case 3:  return "CRUSH";
        default: return "WARM";
    }
}

float TransferCurveDisplay::shapeSample (float input, int mode, float bias) const noexcept
{
    auto x = juce::jlimit (-8.0f, 8.0f, input);
    auto y = 0.0f;

    switch (mode)
    {
        case 1:
            y = juce::jlimit (-1.0f, 1.0f, x * 0.74f);
            break;

        case 2:
            x = std::fmod (x + 3.0f, 4.0f);
            if (x < 0.0f)
                x += 4.0f;
            y = (2.0f - std::abs (x - 2.0f)) * 2.0f - 1.0f;
            break;

        case 3:
        {
            constexpr auto levels = 24.0f;
            y = std::round (juce::jlimit (-1.0f, 1.0f, std::tanh (x * 0.8f)) * levels) / levels;
            break;
        }

        default:
            y = std::tanh (x * 1.15f);
            break;
    }

    return juce::jlimit (-1.0f, 1.0f, y - std::tanh (bias * 2.3f) * 0.38f);
}

void TransferCurveDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    drawSoftPanel (g, bounds, juce::Colour (0xff12140f), 10.0f);

    auto header = getLocalBounds().removeFromTop (34);
    auto graph = getLocalBounds().reduced (22, 45).toFloat();

    auto& state = audioProcessor.getValueTreeState();
    const auto drive = state.getRawParameterValue ("drive")->load();
    const auto bias = state.getRawParameterValue ("bias")->load() * 0.01f;
    const auto mix = state.getRawParameterValue ("mix")->load() * 0.01f;
    const auto mode = juce::jlimit (0, 3, static_cast<int> (std::round (state.getRawParameterValue ("mode")->load())));
    const auto accent = getModeColour (mode);

    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.setColour (muted());
    g.drawFittedText ("TRANSFER", header.withTrimmedLeft (18), juce::Justification::centredLeft, 1);

    g.setColour (accent);
    g.drawFittedText (getModeName (mode), header.withTrimmedRight (18), juce::Justification::centredRight, 1);

    g.setColour (line().withAlpha (0.42f));
    for (int i = 0; i <= 4; ++i)
    {
        const auto x = graph.getX() + graph.getWidth() * static_cast<float> (i) / 4.0f;
        const auto y = graph.getY() + graph.getHeight() * static_cast<float> (i) / 4.0f;
        g.drawVerticalLine (static_cast<int> (x), graph.getY(), graph.getBottom());
        g.drawHorizontalLine (static_cast<int> (y), graph.getX(), graph.getRight());
    }

    g.setColour (muted().withAlpha (0.38f));
    g.drawLine (graph.getX(), graph.getCentreY(), graph.getRight(), graph.getCentreY(), 1.4f);
    g.drawLine (graph.getCentreX(), graph.getY(), graph.getCentreX(), graph.getBottom(), 1.4f);

    juce::Path diagonal;
    diagonal.startNewSubPath (graph.getX(), graph.getBottom());
    diagonal.lineTo (graph.getRight(), graph.getY());
    g.setColour (text().withAlpha (0.20f));
    g.strokePath (diagonal, juce::PathStrokeType (1.2f));

    juce::Path curve;
    constexpr auto points = 220;

    for (int i = 0; i < points; ++i)
    {
        const auto normalised = static_cast<float> (i) / static_cast<float> (points - 1);
        const auto dry = (normalised * 2.0f) - 1.0f;
        const auto wet = shapeSample ((dry * drive) + bias, mode, bias);
        const auto shaped = juce::jlimit (-1.0f, 1.0f, dry * (1.0f - mix) + wet * mix);
        const auto px = graph.getX() + normalised * graph.getWidth();
        const auto py = juce::jmap (shaped, -1.0f, 1.0f, graph.getBottom(), graph.getY());

        if (i == 0)
            curve.startNewSubPath (px, py);
        else
            curve.lineTo (px, py);
    }

    g.setColour (accent.withAlpha (0.16f));
    g.strokePath (curve, juce::PathStrokeType (7.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (curve, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setFont (juce::FontOptions (10.5f, juce::Font::plain));
    g.setColour (muted().withAlpha (0.78f));
    g.drawFittedText ("DRIVE " + juce::String (drive, 1) + "X", getLocalBounds().removeFromBottom (26).withTrimmedLeft (18),
                      juce::Justification::centredLeft, 1);
    g.drawFittedText ("MIX " + juce::String (mix * 100.0f, 0) + "%", getLocalBounds().removeFromBottom (26).withTrimmedRight (18),
                      juce::Justification::centredRight, 1);
}

LevelMeter::LevelMeter (juce::String meterName)
    : name (std::move (meterName))
{
}

void LevelMeter::setLevel (float newLevel)
{
    level = (level * 0.70f) + (juce::jlimit (0.0f, 1.0f, newLevel) * 0.30f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    drawSoftPanel (g, bounds, juce::Colour (0xff12140f), 8.0f);

    auto meter = bounds.reduced (13.0f, 40.0f);
    g.setColour (juce::Colour (0xff070806));
    g.fillRoundedRectangle (meter, 4.0f);

    const auto db = juce::Decibels::gainToDecibels (level + 0.00001f);
    const auto normalised = juce::jlimit (0.0f, 1.0f, (db + 54.0f) / 54.0f);
    constexpr auto segments = 16;
    const auto lit = static_cast<int> (std::ceil (normalised * static_cast<float> (segments)));
    const auto gap = 2.0f;
    const auto segmentHeight = (meter.getHeight() - gap * static_cast<float> (segments - 1)) / static_cast<float> (segments);

    for (int i = 0; i < segments; ++i)
    {
        const auto y = meter.getBottom() - (static_cast<float> (i + 1) * segmentHeight) - (static_cast<float> (i) * gap);
        auto segment = juce::Rectangle<float> (meter.getX() + 5.0f, y, meter.getWidth() - 10.0f, segmentHeight);
        auto colour = i > 13 ? coral()
                             : (i > 10 ? honey() : cyan());

        g.setColour (i < lit ? colour : colour.withAlpha (0.12f));
        g.fillRoundedRectangle (segment, 2.0f);
    }

    g.setColour (line().withAlpha (0.75f));
    g.drawRoundedRectangle (meter, 4.0f, 1.0f);

    g.setColour (text());
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawFittedText (name, getLocalBounds().removeFromTop (28), juce::Justification::centred, 1);

    g.setColour (muted().withAlpha (0.78f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawFittedText (juce::String (juce::Decibels::gainToDecibels (level + 0.00001f), 0) + " dB",
                      getLocalBounds().removeFromBottom (28), juce::Justification::centred, 1);
}

//==============================================================================
NewProjectAudioProcessorEditor::NewProjectAudioProcessorEditor (NewProjectAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&forgeLookAndFeel);

    configureKnob (inputSlider, inputLabel, "INPUT");
    configureKnob (driveSlider, driveLabel, "DRIVE", true);
    configureKnob (toneSlider, toneLabel, "TONE");
    configureKnob (biasSlider, biasLabel, "BIAS");
    configureKnob (mixSlider, mixLabel, "MIX");
    configureKnob (outputSlider, outputLabel, "OUTPUT");

    modeBox.addItemList (juce::StringArray { "Warm", "Hard", "Fold", "Crush" }, 1);
    modeBox.setJustificationType (juce::Justification::centred);
    modeBox.setTooltip ("Distortion mode");
    addAndMakeVisible (modeBox);

    hqButton.setButtonText ("HQ");
    hqButton.setTooltip ("Two-stage internal shaping");
    addAndMakeVisible (hqButton);

    addAndMakeVisible (curveDisplay);
    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    auto& state = audioProcessor.getValueTreeState();
    inputAttachment = std::make_unique<SliderAttachment> (state, "input", inputSlider);
    driveAttachment = std::make_unique<SliderAttachment> (state, "drive", driveSlider);
    toneAttachment = std::make_unique<SliderAttachment> (state, "tone", toneSlider);
    biasAttachment = std::make_unique<SliderAttachment> (state, "bias", biasSlider);
    mixAttachment = std::make_unique<SliderAttachment> (state, "mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment> (state, "output", outputSlider);
    modeAttachment = std::make_unique<ComboBoxAttachment> (state, "mode", modeBox);
    hqAttachment = std::make_unique<ButtonAttachment> (state, "hq", hqButton);

    setResizable (false, false);
    setSize (900, 560);
    startTimerHz (30);
}

NewProjectAudioProcessorEditor::~NewProjectAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void NewProjectAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ink());

    auto full = getLocalBounds().toFloat();
    juce::ColourGradient background (juce::Colour (0xff15160f), 0.0f, 0.0f,
                                     juce::Colour (0xff080907), 0.0f, full.getBottom(), false);
    background.addColour (0.55, juce::Colour (0xff11120d));
    g.setGradientFill (background);
    g.fillRect (full);

    for (int x = 24; x < getWidth(); x += 28)
    {
        g.setColour (juce::Colours::white.withAlpha (0.018f));
        g.drawVerticalLine (x, 16.0f, static_cast<float> (getHeight() - 16));
    }

    auto body = getLocalBounds().toFloat().reduced (18.0f);
    drawSoftPanel (g, body, panel(), 12.0f);

    drawShearMark (g, juce::Rectangle<float> (44.0f, 31.0f, 54.0f, 54.0f));

    g.setFont (juce::FontOptions (31.0f, juce::Font::bold));
    g.setColour (text());
    g.drawFittedText ("SHEAR", juce::Rectangle<int> (114, 29, 152, 36), juce::Justification::centredLeft, 1);

    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (muted());
    g.drawFittedText ("WAVEFORM CUTTER / DISTORTION SHAPER", juce::Rectangle<int> (116, 64, 270, 18), juce::Justification::centredLeft, 1);

    g.setColour (line().withAlpha (0.75f));
    g.drawHorizontalLine (98, 40.0f, static_cast<float> (getWidth() - 40));
}

void NewProjectAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (36, 28);
    auto header = bounds.removeFromTop (74);

    modeBox.setBounds (header.removeFromRight (172).withTrimmedTop (10).withTrimmedBottom (10));
    header.removeFromRight (14);
    hqButton.setBounds (header.removeFromRight (112).withTrimmedTop (10).withTrimmedBottom (10));

    auto main = bounds.removeFromTop (260);
    inputMeter.setBounds (main.removeFromLeft (70).withTrimmedTop (14).withTrimmedBottom (14));
    main.removeFromLeft (18);

    auto driveArea = main.removeFromRight (286);
    outputMeter.setBounds (driveArea.removeFromRight (70).withTrimmedTop (14).withTrimmedBottom (14));
    driveArea.removeFromRight (16);
    layoutKnob (driveSlider, driveLabel, driveArea.reduced (2), true);

    curveDisplay.setBounds (main.reduced (8, 14));

    auto strip = bounds.reduced (0, 12);
    const auto knobWidth = strip.getWidth() / 5;

    layoutKnob (inputSlider, inputLabel, strip.removeFromLeft (knobWidth).reduced (8));
    layoutKnob (toneSlider, toneLabel, strip.removeFromLeft (knobWidth).reduced (8));
    layoutKnob (biasSlider, biasLabel, strip.removeFromLeft (knobWidth).reduced (8));
    layoutKnob (mixSlider, mixLabel, strip.removeFromLeft (knobWidth).reduced (8));
    layoutKnob (outputSlider, outputLabel, strip.reduced (8));
}

void NewProjectAudioProcessorEditor::timerCallback()
{
    inputMeter.setLevel (audioProcessor.getInputLevel());
    outputMeter.setLevel (audioProcessor.getOutputLevel());
    curveDisplay.repaint();
}

void NewProjectAudioProcessorEditor::configureKnob (juce::Slider& slider, juce::Label& label,
                                                    const juce::String& labelText, bool primary)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.18f,
                                juce::MathConstants<float>::pi * 2.82f,
                                true);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, primary ? 80 : 72, 22);
    slider.setMouseDragSensitivity (primary ? 220 : 170);
    slider.setWantsKeyboardFocus (true);
    slider.setName (labelText);
    slider.setTooltip (labelText);

    slider.textFromValueFunction = [labelText] (double value)
    {
        if (labelText == "DRIVE")
            return juce::String (value, 1) + "x";

        if (labelText == "INPUT" || labelText == "OUTPUT")
            return juce::String (value, 1) + " dB";

        return juce::String (value, 0) + "%";
    };

    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    label.setColour (juce::Label::textColourId, colourForName (labelText));
    addAndMakeVisible (label);
}

void NewProjectAudioProcessorEditor::layoutKnob (juce::Slider& slider, juce::Label& label,
                                                 juce::Rectangle<int> area, bool primary)
{
    label.setBounds (area.removeFromTop (primary ? 30 : 26));
    area.removeFromTop (primary ? 4 : 2);
    slider.setBounds (area.reduced (primary ? 0 : 1));
}
