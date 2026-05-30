#include "PluginProcessor.h"
#include "PluginEditor.h"

ClampAudioProcessorEditor::ClampAudioProcessorEditor (ClampAudioProcessor& processor)
    : WebViewEditor (processor,
                     processor.getValueTreeState(),
                     elaw::EditorBindings {
                         juce::StringArray { "input", "threshold", "ratio", "attack", "release", "punch", "mix", "output" },
                         juce::StringArray { "mode" },
                         juce::StringArray { "autoGain" },
                         "ClampWebView2",
                         juce::Colour (0xff080907)
                     },
                     elaw::getSiblingWebUiDistDirectory (__FILE__),
                     [&processor]
                     {
                         return std::pair<float, float> { processor.getInputLevel(), processor.getOutputLevel() };
                     })
{
}
