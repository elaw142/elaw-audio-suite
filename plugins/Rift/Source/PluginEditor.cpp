#include "PluginProcessor.h"
#include "PluginEditor.h"

RiftAudioProcessorEditor::RiftAudioProcessorEditor (RiftAudioProcessor& processor)
    : WebViewEditor (processor,
                     processor.getValueTreeState(),
                     elaw::EditorBindings {
                         juce::StringArray { "size", "density", "pitch", "jitter", "feedback", "mix", "output" },
                         juce::StringArray { "mode" },
                         juce::StringArray { "freeze" },
                         "RiftWebView2",
                         juce::Colour (0xff080907)
                     },
                     elaw::getSiblingWebUiDistDirectory (__FILE__),
                     [&processor]
                     {
                         return std::pair<float, float> { processor.getInputLevel(), processor.getOutputLevel() };
                     })
{
}
