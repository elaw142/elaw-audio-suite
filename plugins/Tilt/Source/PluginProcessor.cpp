#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../../../shared/cpp/ElawAudio/SuiteHelpers.h"

TiltAudioProcessor::TiltAudioProcessor()
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

TiltAudioProcessor::~TiltAudioProcessor() = default;

const juce::String TiltAudioProcessor::getName() const { return JucePlugin_Name; }

bool TiltAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TiltAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TiltAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TiltAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int TiltAudioProcessor::getNumPrograms() { return 1; }
int TiltAudioProcessor::getCurrentProgram() { return 0; }
void TiltAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String TiltAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void TiltAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

#ifndef JucePlugin_PreferredChannelConfigurations
bool TiltAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

bool TiltAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* TiltAudioProcessor::createEditor()
{
    return new TiltAudioProcessorEditor (*this);
}

void TiltAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TiltAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}


void TiltAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;

    for (auto* smoother : { &inputGainSmoothed, &tiltSmoothed, &pivotSmoothed, &lowCutSmoothed, &highCutSmoothed, &colorSmoothed, &mixSmoothed, &outputGainSmoothed })
        smoother->reset (sampleRate, 0.04);

    lowCutState.fill (0.0f);
    pivotState.fill (0.0f);
    highCutState.fill (0.0f);
    updateSmoothingTargets();
}

void TiltAudioProcessor::releaseResources() {}

void TiltAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateSmoothingTargets();

    const auto mode = static_cast<Mode> (juce::jlimit (0, 3, static_cast<int> (std::round (parameters.getRawParameterValue ("mode")->load()))));
    double inputSquareSum = 0.0;
    double outputSquareSum = 0.0;
    int sampleCount = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto inputGain = inputGainSmoothed.getNextValue();
        const auto tilt = tiltSmoothed.getNextValue();
        const auto pivot = pivotSmoothed.getNextValue();
        const auto lowCut = lowCutSmoothed.getNextValue();
        const auto highCut = highCutSmoothed.getNextValue();
        const auto color = colorSmoothed.getNextValue();
        const auto mix = mixSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            const auto channelIndex = static_cast<size_t> (juce::jmin (channel, 1));
            const auto dry = channelData[sample] * inputGain;

            const auto lowCutAlpha = elaw::onePoleAlpha (lowCut, currentSampleRate);
            lowCutState[channelIndex] += lowCutAlpha * (dry - lowCutState[channelIndex]);
            auto filtered = dry - lowCutState[channelIndex];

            const auto pivotAlpha = elaw::onePoleAlpha (pivot, currentSampleRate);
            pivotState[channelIndex] += pivotAlpha * (filtered - pivotState[channelIndex]);
            const auto low = pivotState[channelIndex];
            const auto high = filtered - low;

            const auto tiltDb = tilt * 12.0f;
            auto wet = (low * juce::Decibels::decibelsToGain (-tiltDb)) + (high * juce::Decibels::decibelsToGain (tiltDb));

            const auto highCutAlpha = elaw::onePoleAlpha (highCut, currentSampleRate);
            highCutState[channelIndex] += highCutAlpha * (wet - highCutState[channelIndex]);
            wet = highCutState[channelIndex];

            switch (mode)
            {
                case Mode::clean: break;
                case Mode::warm: wet = std::tanh (wet * (1.0f + color * 0.65f)); break;
                case Mode::air: wet += high * color * 0.72f; break;
                case Mode::dark: wet = (wet * (1.0f - color * 0.38f)) + (low * color * 0.38f); break;
            }

            const auto output = ((dry * (1.0f - mix)) + (wet * mix)) * outputGain;
            channelData[sample] = juce::jlimit (-1.0f, 1.0f, output);
            inputSquareSum += static_cast<double> (dry * dry);
            outputSquareSum += static_cast<double> (channelData[sample] * channelData[sample]);
            ++sampleCount;
        }
    }

    elaw::storeRmsLevels (inputLevel, outputLevel, inputSquareSum, outputSquareSum, sampleCount);
}

juce::AudioProcessorValueTreeState::ParameterLayout TiltAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("input", "Input", juce::NormalisableRange<float> (-18.0f, 18.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("tilt", "Tilt", juce::NormalisableRange<float> (-100.0f, 100.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("pivot", "Pivot", juce::NormalisableRange<float> (120.0f, 8000.0f, 0.01f, 0.36f), 1000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("lowCut", "Low Cut", juce::NormalisableRange<float> (20.0f, 420.0f, 0.01f, 0.45f), 20.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("highCut", "High Cut", juce::NormalisableRange<float> (2500.0f, 20000.0f, 0.01f, 0.62f), 18000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("color", "Color", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("mode", "Mode", juce::StringArray { "Clean", "Warm", "Air", "Dark" }, 0));
    return { params.begin(), params.end() };
}

void TiltAudioProcessor::updateSmoothingTargets()
{
    inputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("input")->load()));
    tiltSmoothed.setTargetValue (parameters.getRawParameterValue ("tilt")->load() * 0.01f);
    pivotSmoothed.setTargetValue (parameters.getRawParameterValue ("pivot")->load());
    lowCutSmoothed.setTargetValue (parameters.getRawParameterValue ("lowCut")->load());
    highCutSmoothed.setTargetValue (parameters.getRawParameterValue ("highCut")->load());
    colorSmoothed.setTargetValue (parameters.getRawParameterValue ("color")->load() * 0.01f);
    mixSmoothed.setTargetValue (parameters.getRawParameterValue ("mix")->load() * 0.01f);
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load()));
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TiltAudioProcessor();
}
