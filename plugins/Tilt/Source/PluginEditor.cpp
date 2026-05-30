#include "PluginProcessor.h"
#include "PluginEditor.h"

TiltAudioProcessorEditor::TiltAudioProcessorEditor (TiltAudioProcessor& processor)
    : WebViewEditor (processor,
                     processor.getValueTreeState(),
                     elaw::EditorBindings {
                         juce::StringArray { "input", "tilt", "pivot", "lowCut", "highCut", "color", "mix", "output" },
                         juce::StringArray { "mode" },
                         juce::StringArray {  },
                         "TiltWebView2",
                         juce::Colour (0xff080907)
                     },
                     elaw::getSiblingWebUiDistDirectory (__FILE__),
                     [&processor]
                     {
                         return std::pair<float, float> { processor.getInputLevel(), processor.getOutputLevel() };
                     })
{
}
