#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../../shared/cpp/ElawAudio/SuiteWebEditor.h"

class ShearAudioProcessorEditor final : public elaw::WebViewEditor
{
public:
    explicit ShearAudioProcessorEditor (ShearAudioProcessor&);
    ~ShearAudioProcessorEditor() override = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShearAudioProcessorEditor)
};
