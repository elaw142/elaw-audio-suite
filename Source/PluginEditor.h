/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ForgeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ForgeLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    juce::Font getLabelFont (juce::Label&) override;
};

class TransferCurveDisplay : public juce::Component
{
public:
    explicit TransferCurveDisplay (NewProjectAudioProcessor&);

    void paint (juce::Graphics&) override;

private:
    juce::Colour getModeColour (int mode) const;
    juce::String getModeName (int mode) const;
    float shapeSample (float input, int mode, float bias) const noexcept;

    NewProjectAudioProcessor& audioProcessor;
};

class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (juce::String meterName);

    void setLevel (float newLevel);
    void paint (juce::Graphics&) override;

private:
    juce::String name;
    float level = 0.0f;
};

//==============================================================================
/**
*/
class NewProjectAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    NewProjectAudioProcessorEditor (NewProjectAudioProcessor&);
    ~NewProjectAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void configureKnob (juce::Slider&, juce::Label&, const juce::String& labelText, bool primary = false);
    void layoutKnob (juce::Slider&, juce::Label&, juce::Rectangle<int>, bool primary = false);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    NewProjectAudioProcessor& audioProcessor;

    ForgeLookAndFeel forgeLookAndFeel;
    juce::TooltipWindow tooltipWindow;

    juce::Slider inputSlider;
    juce::Slider driveSlider;
    juce::Slider toneSlider;
    juce::Slider biasSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;

    juce::Label inputLabel;
    juce::Label driveLabel;
    juce::Label toneLabel;
    juce::Label biasLabel;
    juce::Label mixLabel;
    juce::Label outputLabel;

    juce::ComboBox modeBox;
    juce::ToggleButton hqButton;

    TransferCurveDisplay curveDisplay { audioProcessor };
    LevelMeter inputMeter { "IN" };
    LevelMeter outputMeter { "OUT" };

    std::unique_ptr<SliderAttachment> inputAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> toneAttachment;
    std::unique_ptr<SliderAttachment> biasAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<ButtonAttachment> hqAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NewProjectAudioProcessorEditor)
};
