#include "PluginProcessor.h"
#include "PluginEditor.h"

ShearAudioProcessorEditor::ShearAudioProcessorEditor (ShearAudioProcessor& processor)
    : WebViewEditor (processor,
                     processor.getValueTreeState(),
                     elaw::EditorBindings {
                         juce::StringArray { "input", "drive", "tone", "bias", "mix", "output" },
                         juce::StringArray { "mode" },
                         juce::StringArray { "hq" },
                         "ShearWebView2",
                         juce::Colour (0xff080907)
                     },
                     elaw::getSiblingWebUiDistDirectory (__FILE__),
                     [&processor]
                     {
                         return std::pair<float, float> { processor.getInputLevel(), processor.getOutputLevel() };
                     })
{
}
