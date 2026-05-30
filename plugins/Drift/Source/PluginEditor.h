#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../../shared/cpp/ElawAudio/SuiteWebEditor.h"

class DriftAudioProcessorEditor final : public elaw::WebViewEditor
{
public:
    explicit DriftAudioProcessorEditor (DriftAudioProcessor&);
    ~DriftAudioProcessorEditor() override = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DriftAudioProcessorEditor)
};
