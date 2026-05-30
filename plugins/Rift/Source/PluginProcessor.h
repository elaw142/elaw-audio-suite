#pragma once

#include <JuceHeader.h>
#include <array>

class RiftAudioProcessor  : public juce::AudioProcessor
{
public:
    RiftAudioProcessor();
    ~RiftAudioProcessor() override;

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
        grain,
        reverse,
        scatter,
        crush
    };

    void updateSmoothingTargets();

    float readBufferSample (int channel, float readPosition) const noexcept;

    juce::LinearSmoothedValue<float> sizeSmoothed;
    juce::LinearSmoothedValue<float> densitySmoothed;
    juce::LinearSmoothedValue<float> pitchSmoothed;
    juce::LinearSmoothedValue<float> jitterSmoothed;
    juce::LinearSmoothedValue<float> feedbackSmoothed;
    juce::LinearSmoothedValue<float> mixSmoothed;
    juce::LinearSmoothedValue<float> outputGainSmoothed;

    juce::AudioBuffer<float> grainBuffer;
    juce::Random random;
    std::array<float, 2> feedbackState {};
    std::array<float, 2> heldSample {};
    float grainPhase = 0.0f;
    float scatterOffset = 0.0f;
    int writePosition = 0;
    int holdCounter = 0;


    juce::AudioProcessorValueTreeState parameters;
    double currentSampleRate = 44100.0;
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RiftAudioProcessor)
};
