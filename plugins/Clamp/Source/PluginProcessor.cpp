#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../../shared/cpp/ElawAudio/SuiteHelpers.h"

ClampAudioProcessor::ClampAudioProcessor()
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

ClampAudioProcessor::~ClampAudioProcessor() = default;

const juce::String ClampAudioProcessor::getName() const { return JucePlugin_Name; }

bool ClampAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ClampAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ClampAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ClampAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ClampAudioProcessor::getNumPrograms() { return 1; }
int ClampAudioProcessor::getCurrentProgram() { return 0; }
void ClampAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String ClampAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void ClampAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool ClampAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

bool ClampAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ClampAudioProcessor::createEditor()
{
    return new ClampAudioProcessorEditor (*this);
}

void ClampAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ClampAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}


void ClampAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    for (auto* smoother : { &inputGainSmoothed, &thresholdSmoothed, &ratioSmoothed, &attackSmoothed, &releaseSmoothed, &punchSmoothed, &mixSmoothed, &outputGainSmoothed })
        smoother->reset (sampleRate, 0.025);

    envelope.fill (0.0f);
    previousSample.fill (0.0f);
    updateSmoothingTargets();
}

void ClampAudioProcessor::releaseResources() {}

void ClampAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateSmoothingTargets();

    const auto mode = static_cast<Mode> (juce::jlimit (0, 3, static_cast<int> (std::round (parameters.getRawParameterValue ("mode")->load()))));
    const auto autoGainEnabled = parameters.getRawParameterValue ("autoGain")->load() > 0.5f;
    double inputSquareSum = 0.0;
    double outputSquareSum = 0.0;
    int sampleCount = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto inputGain = inputGainSmoothed.getNextValue();
        auto threshold = thresholdSmoothed.getNextValue();
        auto ratio = ratioSmoothed.getNextValue();
        const auto attackMs = attackSmoothed.getNextValue();
        const auto releaseMs = releaseSmoothed.getNextValue();
        auto punch = punchSmoothed.getNextValue();
        const auto mix = mixSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();

        if (mode == Mode::snap) { threshold += 2.0f; punch += 0.18f; }
        if (mode == Mode::crush) { ratio = juce::jmax (ratio, 10.0f); threshold -= 4.0f; punch += 0.12f; }
        if (mode == Mode::limit) { ratio = 20.0f; threshold = juce::jmin (threshold, -2.0f); }

        const auto attackCoeff = std::exp (-1.0f / (0.001f * attackMs * static_cast<float> (currentSampleRate)));
        const auto releaseCoeff = std::exp (-1.0f / (0.001f * releaseMs * static_cast<float> (currentSampleRate)));

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            const auto channelIndex = static_cast<size_t> (juce::jmin (channel, 1));
            const auto dry = channelData[sample] * inputGain;
            const auto detector = std::abs (dry);
            const auto coeff = detector > envelope[channelIndex] ? attackCoeff : releaseCoeff;
            envelope[channelIndex] = (coeff * envelope[channelIndex]) + ((1.0f - coeff) * detector);

            const auto envDb = juce::Decibels::gainToDecibels (envelope[channelIndex] + 0.000001f);
            const auto overDb = envDb - threshold;
            auto gainDb = overDb > 0.0f ? -overDb * (1.0f - (1.0f / ratio)) : 0.0f;
            if (autoGainEnabled)
                gainDb += juce::jlimit (0.0f, 12.0f, (-threshold) * 0.22f * (1.0f - (1.0f / ratio)));

            auto wet = dry * juce::Decibels::decibelsToGain (gainDb);
            const auto transient = dry - previousSample[channelIndex];
            previousSample[channelIndex] = dry;
            wet += transient * punch * 0.16f;

            if (mode == Mode::crush)
                wet = std::tanh (wet * 1.35f) * 0.92f;
            else if (mode == Mode::limit)
                wet = juce::jlimit (-0.98f, 0.98f, wet);

            const auto output = ((dry * (1.0f - mix)) + (wet * mix)) * outputGain;
            channelData[sample] = juce::jlimit (-1.0f, 1.0f, output);
            inputSquareSum += static_cast<double> (dry * dry);
            outputSquareSum += static_cast<double> (channelData[sample] * channelData[sample]);
            ++sampleCount;
        }
    }

    elaw::storeRmsLevels (inputLevel, outputLevel, inputSquareSum, outputSquareSum, sampleCount);
}

juce::AudioProcessorValueTreeState::ParameterLayout ClampAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("input", "Input", juce::NormalisableRange<float> (-18.0f, 18.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("threshold", "Threshold", juce::NormalisableRange<float> (-48.0f, 0.0f, 0.01f), -22.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("ratio", "Ratio", juce::NormalisableRange<float> (1.0f, 20.0f, 0.01f, 0.38f), 4.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("attack", "Attack", juce::NormalisableRange<float> (1.0f, 100.0f, 0.01f, 0.42f), 10.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("release", "Release", juce::NormalisableRange<float> (20.0f, 1000.0f, 0.01f, 0.45f), 250.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("punch", "Punch", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 38.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("mode", "Mode", juce::StringArray { "Glue", "Snap", "Crush", "Limit" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("autoGain", "Auto", true));
    return { params.begin(), params.end() };
}

void ClampAudioProcessor::updateSmoothingTargets()
{
    inputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("input")->load()));
    thresholdSmoothed.setTargetValue (parameters.getRawParameterValue ("threshold")->load());
    ratioSmoothed.setTargetValue (parameters.getRawParameterValue ("ratio")->load());
    attackSmoothed.setTargetValue (parameters.getRawParameterValue ("attack")->load());
    releaseSmoothed.setTargetValue (parameters.getRawParameterValue ("release")->load());
    punchSmoothed.setTargetValue (parameters.getRawParameterValue ("punch")->load() * 0.01f);
    mixSmoothed.setTargetValue (parameters.getRawParameterValue ("mix")->load() * 0.01f);
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load()));
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClampAudioProcessor();
}
