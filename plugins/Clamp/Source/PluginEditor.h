#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../../shared/cpp/ElawAudio/SuiteWebEditor.h"

class ClampAudioProcessorEditor final : public elaw::WebViewEditor
{
public:
    explicit ClampAudioProcessorEditor (ClampAudioProcessor&);
    ~ClampAudioProcessorEditor() override = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClampAudioProcessorEditor)
};
