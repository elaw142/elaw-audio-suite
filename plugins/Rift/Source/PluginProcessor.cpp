#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../../shared/cpp/ElawAudio/SuiteHelpers.h"

RiftAudioProcessor::RiftAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
#else
     : parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

RiftAudioProcessor::~RiftAudioProcessor() = default;

const juce::String RiftAudioProcessor::getName() const { return JucePlugin_Name; }

bool RiftAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RiftAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RiftAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RiftAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int RiftAudioProcessor::getNumPrograms() { return 1; }
int RiftAudioProcessor::getCurrentProgram() { return 0; }
void RiftAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String RiftAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void RiftAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool RiftAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

bool RiftAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* RiftAudioProcessor::createEditor()
{
    return new RiftAudioProcessorEditor (*this);
}

void RiftAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void RiftAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}


void RiftAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    for (auto* smoother : { &sizeSmoothed, &densitySmoothed, &pitchSmoothed, &jitterSmoothed, &feedbackSmoothed, &mixSmoothed, &outputGainSmoothed })
        smoother->reset (sampleRate, 0.03);

    grainBuffer.setSize (2, static_cast<int> (std::ceil (sampleRate * 4.0)));
    grainBuffer.clear();
    feedbackState.fill (0.0f);
    heldSample.fill (0.0f);
    writePosition = 0;
    grainPhase = 0.0f;
    scatterOffset = 0.0f;
    holdCounter = 0;
    updateSmoothingTargets();
}

void RiftAudioProcessor::releaseResources()
{
    grainBuffer.setSize (0, 0);
}

void RiftAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (grainBuffer.getNumSamples() == 0)
        return;

    updateSmoothingTargets();

    const auto mode = static_cast<Mode> (juce::jlimit (0, 3, static_cast<int> (std::round (parameters.getRawParameterValue ("mode")->load()))));
    const auto freeze = parameters.getRawParameterValue ("freeze")->load() > 0.5f;
    const auto bufferSamples = grainBuffer.getNumSamples();
    double inputSquareSum = 0.0;
    double outputSquareSum = 0.0;
    int sampleCount = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto sizeMs = sizeSmoothed.getNextValue();
        const auto density = densitySmoothed.getNextValue();
        const auto pitch = pitchSmoothed.getNextValue();
        const auto jitter = jitterSmoothed.getNextValue();
        const auto feedback = feedbackSmoothed.getNextValue();
        const auto mix = mixSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();
        const auto grainSamples = juce::jlimit (8.0f, static_cast<float> (bufferSamples - 4), sizeMs * 0.001f * static_cast<float> (currentSampleRate));
        const auto speed = std::pow (2.0f, pitch / 12.0f);

        grainPhase += speed / grainSamples;
        if (grainPhase >= 1.0f)
        {
            grainPhase -= std::floor (grainPhase);
            scatterOffset = random.nextFloat() * jitter * grainSamples;
        }

        if (--holdCounter <= 0)
            holdCounter = juce::jmax (1, static_cast<int> (juce::jmap (density, 0.0f, 1.0f, 36.0f, 2.0f)));

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            const auto channelIndex = static_cast<size_t> (juce::jmin (channel, 1));
            const auto dry = channelData[sample];

            if (! freeze)
                grainBuffer.setSample (channel, writePosition, juce::jlimit (-1.0f, 1.0f, dry + feedbackState[channelIndex] * feedback));

            auto offset = grainSamples * grainPhase;
            if (mode == Mode::reverse)
                offset = grainSamples * (1.0f - grainPhase);
            if (mode == Mode::scatter)
                offset += scatterOffset;
            if (mode == Mode::crush)
                offset = std::floor (offset / 12.0f) * 12.0f;

            auto readPosition = static_cast<float> (writePosition) - offset - (jitter * 0.25f * scatterOffset);
            while (readPosition < 0.0f)
                readPosition += static_cast<float> (bufferSamples);

            auto wet = readBufferSample (channel, readPosition);
            if (mode == Mode::crush)
            {
                if (holdCounter == 1)
                    heldSample[channelIndex] = std::round (wet * 18.0f) / 18.0f;
                wet = heldSample[channelIndex];
            }

            feedbackState[channelIndex] = wet;
            const auto output = ((dry * (1.0f - mix)) + (wet * mix)) * outputGain;
            channelData[sample] = juce::jlimit (-1.0f, 1.0f, output);
            inputSquareSum += static_cast<double> (dry * dry);
            outputSquareSum += static_cast<double> (channelData[sample] * channelData[sample]);
            ++sampleCount;
        }

        if (! freeze && ++writePosition >= bufferSamples)
            writePosition = 0;
    }

    elaw::storeRmsLevels (inputLevel, outputLevel, inputSquareSum, outputSquareSum, sampleCount);
}

juce::AudioProcessorValueTreeState::ParameterLayout RiftAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("size", "Size", juce::NormalisableRange<float> (10.0f, 500.0f, 0.01f, 0.45f), 165.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("density", "Density", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 48.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("pitch", "Pitch", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("jitter", "Jitter", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 33.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback", juce::NormalisableRange<float> (0.0f, 95.0f, 0.01f), 18.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("mode", "Mode", juce::StringArray { "Grain", "Reverse", "Scatter", "Crush" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("freeze", "Freeze", false));
    return { params.begin(), params.end() };
}

void RiftAudioProcessor::updateSmoothingTargets()
{
    sizeSmoothed.setTargetValue (parameters.getRawParameterValue ("size")->load());
    densitySmoothed.setTargetValue (parameters.getRawParameterValue ("density")->load() * 0.01f);
    pitchSmoothed.setTargetValue (parameters.getRawParameterValue ("pitch")->load());
    jitterSmoothed.setTargetValue (parameters.getRawParameterValue ("jitter")->load() * 0.01f);
    feedbackSmoothed.setTargetValue (parameters.getRawParameterValue ("feedback")->load() * 0.01f);
    mixSmoothed.setTargetValue (parameters.getRawParameterValue ("mix")->load() * 0.01f);
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load()));
}

float RiftAudioProcessor::readBufferSample (int channel, float readPosition) const noexcept
{
    const auto bufferSamples = grainBuffer.getNumSamples();
    const auto indexA = static_cast<int> (readPosition) % bufferSamples;
    const auto indexB = (indexA + 1) % bufferSamples;
    const auto fraction = readPosition - static_cast<float> (indexA);
    return juce::jmap (fraction, grainBuffer.getSample (channel, indexA), grainBuffer.getSample (channel, indexB));
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RiftAudioProcessor();
}
