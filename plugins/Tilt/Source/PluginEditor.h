#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../../shared/cpp/ElawAudio/SuiteWebEditor.h"

class TiltAudioProcessorEditor final : public elaw::WebViewEditor
{
public:
    explicit TiltAudioProcessorEditor (TiltAudioProcessor&);
    ~TiltAudioProcessorEditor() override = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TiltAudioProcessorEditor)
};
