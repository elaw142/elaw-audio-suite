#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../../shared/cpp/ElawAudio/SuiteHelpers.h"

DriftAudioProcessor::DriftAudioProcessor()
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

DriftAudioProcessor::~DriftAudioProcessor() = default;

const juce::String DriftAudioProcessor::getName() const { return JucePlugin_Name; }

bool DriftAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DriftAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DriftAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DriftAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DriftAudioProcessor::getNumPrograms() { return 1; }
int DriftAudioProcessor::getCurrentProgram() { return 0; }
void DriftAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String DriftAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void DriftAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool DriftAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

bool DriftAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* DriftAudioProcessor::createEditor()
{
    return new DriftAudioProcessorEditor (*this);
}

void DriftAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DriftAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}


void DriftAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    for (auto* smoother : { &timeSmoothed, &feedbackSmoothed, &rateSmoothed, &depthSmoothed, &spreadSmoothed, &toneSmoothed, &mixSmoothed, &outputGainSmoothed })
        smoother->reset (sampleRate, 0.035);

    delayBuffer.setSize (2, static_cast<int> (std::ceil (sampleRate * 2.0)));
    delayBuffer.clear();
    delayWritePosition = 0;
    lfoPhase = 0.0f;
    toneState.fill (0.0f);
    updateSmoothingTargets();
}

void DriftAudioProcessor::releaseResources()
{
    delayBuffer.setSize (0, 0);
}

void DriftAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (delayBuffer.getNumSamples() == 0)
        return;

    updateSmoothingTargets();

    const auto mode = static_cast<Mode> (juce::jlimit (0, 3, static_cast<int> (std::round (parameters.getRawParameterValue ("mode")->load()))));
    const auto delayBufferSamples = delayBuffer.getNumSamples();
    double inputSquareSum = 0.0;
    double outputSquareSum = 0.0;
    int sampleCount = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        auto timeMs = timeSmoothed.getNextValue();
        auto feedback = feedbackSmoothed.getNextValue();
        const auto rate = rateSmoothed.getNextValue();
        const auto depth = depthSmoothed.getNextValue();
        const auto spread = spreadSmoothed.getNextValue();
        const auto tone = toneSmoothed.getNextValue();
        const auto mix = mixSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();

        if (mode == Mode::flange) timeMs = juce::jlimit (1.0f, 18.0f, timeMs * 0.18f);
        if (mode == Mode::dub) { timeMs *= 1.65f; feedback = juce::jmin (0.92f, feedback + 0.18f); }

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            const auto channelIndex = static_cast<size_t> (juce::jmin (channel, 1));
            const auto phaseOffset = (channel == 0 ? -spread : spread) * 0.25f;
            const auto lfo = std::sin ((lfoPhase + phaseOffset) * juce::MathConstants<float>::twoPi);
            const auto modMs = depth * (mode == Mode::flange ? 7.0f : 32.0f) * lfo;
            const auto delaySamples = juce::jlimit (1.0f, static_cast<float> (delayBufferSamples - 4), (timeMs + modMs) * 0.001f * static_cast<float> (currentSampleRate));

            const auto dry = channelData[sample];
            auto delayed = readDelaySample (channel, delaySamples);
            const auto toneCutoff = juce::jmap (tone, 0.0f, 1.0f, 850.0f, 16000.0f);
            toneState[channelIndex] += elaw::onePoleAlpha (toneCutoff, currentSampleRate) * (delayed - toneState[channelIndex]);
            delayed = (delayed * (0.3f + tone * 0.7f)) + (toneState[channelIndex] * (0.7f - tone * 0.35f));

            auto wet = delayed;
            if (mode == Mode::chorus) wet = (dry * 0.42f) + (delayed * 0.78f);
            if (mode == Mode::wide && channel == 1) wet = -wet;

            delayBuffer.setSample (channel, delayWritePosition, juce::jlimit (-1.0f, 1.0f, dry + delayed * feedback));
            const auto output = ((dry * (1.0f - mix)) + (wet * mix)) * outputGain;
            channelData[sample] = juce::jlimit (-1.0f, 1.0f, output);
            inputSquareSum += static_cast<double> (dry * dry);
            outputSquareSum += static_cast<double> (channelData[sample] * channelData[sample]);
            ++sampleCount;
        }

        if (++delayWritePosition >= delayBufferSamples)
            delayWritePosition = 0;

        lfoPhase += rate / static_cast<float> (currentSampleRate);
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }

    elaw::storeRmsLevels (inputLevel, outputLevel, inputSquareSum, outputSquareSum, sampleCount);
}

juce::AudioProcessorValueTreeState::ParameterLayout DriftAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("time", "Time", juce::NormalisableRange<float> (1.0f, 800.0f, 0.01f, 0.42f), 42.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback", juce::NormalisableRange<float> (0.0f, 95.0f, 0.01f), 27.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("rate", "Rate", juce::NormalisableRange<float> (0.05f, 8.0f, 0.01f, 0.36f), 0.75f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("depth", "Depth", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 42.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("spread", "Spread", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 36.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("tone", "Tone", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 58.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 45.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("mode", "Mode", juce::StringArray { "Chorus", "Flange", "Dub", "Wide" }, 0));
    return { params.begin(), params.end() };
}

void DriftAudioProcessor::updateSmoothingTargets()
{
    timeSmoothed.setTargetValue (parameters.getRawParameterValue ("time")->load());
    feedbackSmoothed.setTargetValue (parameters.getRawParameterValue ("feedback")->load() * 0.01f);
    rateSmoothed.setTargetValue (parameters.getRawParameterValue ("rate")->load());
    depthSmoothed.setTargetValue (parameters.getRawParameterValue ("depth")->load() * 0.01f);
    spreadSmoothed.setTargetValue (parameters.getRawParameterValue ("spread")->load() * 0.01f);
    toneSmoothed.setTargetValue (parameters.getRawParameterValue ("tone")->load() * 0.01f);
    mixSmoothed.setTargetValue (parameters.getRawParameterValue ("mix")->load() * 0.01f);
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load()));
}

float DriftAudioProcessor::readDelaySample (int channel, float delaySamples) const noexcept
{
    if (delayBuffer.getNumSamples() == 0)
        return 0.0f;

    auto readPosition = static_cast<float> (delayWritePosition) - delaySamples;
    const auto bufferSamples = delayBuffer.getNumSamples();
    while (readPosition < 0.0f)
        readPosition += static_cast<float> (bufferSamples);

    const auto indexA = static_cast<int> (readPosition) % bufferSamples;
    const auto indexB = (indexA + 1) % bufferSamples;
    const auto fraction = readPosition - static_cast<float> (indexA);
    return juce::jmap (fraction, delayBuffer.getSample (channel, indexA), delayBuffer.getSample (channel, indexB));
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriftAudioProcessor();
}
