#include "PluginProcessor.h"
#include "PluginEditor.h"

DriftAudioProcessorEditor::DriftAudioProcessorEditor (DriftAudioProcessor& processor)
    : WebViewEditor (processor,
                     processor.getValueTreeState(),
                     elaw::EditorBindings {
                         juce::StringArray { "time", "feedback", "rate", "depth", "spread", "tone", "mix", "output" },
                         juce::StringArray { "mode" },
                         juce::StringArray {  },
                         "DriftWebView2",
                         juce::Colour (0xff080907)
                     },
                     elaw::getSiblingWebUiDistDirectory (__FILE__),
                     [&processor]
                     {
                         return std::pair<float, float> { processor.getInputLevel(), processor.getOutputLevel() };
                     })
{
}
