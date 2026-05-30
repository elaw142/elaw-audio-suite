#pragma once

#include <JuceHeader.h>
#include <array>

class ShearAudioProcessor  : public juce::AudioProcessor
{
public:
    ShearAudioProcessor();
    ~ShearAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return parameters; }

    float getInputLevel() const noexcept { return inputLevel.load(); }
    float getOutputLevel() const noexcept { return outputLevel.load(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    enum class Mode
    {
        warm,
        hard,
        fold,
        crush
    };

    void updateSmoothingTargets();

    float processDistortionSample (float input, Mode mode, float bias) const noexcept;

    juce::LinearSmoothedValue<float> inputGainSmoothed;
    juce::LinearSmoothedValue<float> driveSmoothed;
    juce::LinearSmoothedValue<float> toneSmoothed;
    juce::LinearSmoothedValue<float> biasSmoothed;
    juce::LinearSmoothedValue<float> mixSmoothed;
    juce::LinearSmoothedValue<float> outputGainSmoothed;

    std::array<float, 2> toneState {};
    std::array<float, 2> previousSample {};


    juce::AudioProcessorValueTreeState parameters;
    double currentSampleRate = 44100.0;
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShearAudioProcessor)
};
