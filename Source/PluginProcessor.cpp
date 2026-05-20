/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NewProjectAudioProcessor::NewProjectAudioProcessor()
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

NewProjectAudioProcessor::~NewProjectAudioProcessor()
{
}

//==============================================================================
const juce::String NewProjectAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NewProjectAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NewProjectAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NewProjectAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NewProjectAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NewProjectAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NewProjectAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String NewProjectAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void NewProjectAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void NewProjectAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;

    inputGainSmoothed.reset (sampleRate, 0.035);
    driveSmoothed.reset (sampleRate, 0.035);
    toneSmoothed.reset (sampleRate, 0.035);
    biasSmoothed.reset (sampleRate, 0.035);
    mixSmoothed.reset (sampleRate, 0.035);
    outputGainSmoothed.reset (sampleRate, 0.035);

    toneState.fill (0.0f);
    previousSample.fill (0.0f);
    updateSmoothingTargets();

    inputGainSmoothed.setCurrentAndTargetValue (inputGainSmoothed.getTargetValue());
    driveSmoothed.setCurrentAndTargetValue (driveSmoothed.getTargetValue());
    toneSmoothed.setCurrentAndTargetValue (toneSmoothed.getTargetValue());
    biasSmoothed.setCurrentAndTargetValue (biasSmoothed.getTargetValue());
    mixSmoothed.setCurrentAndTargetValue (mixSmoothed.getTargetValue());
    outputGainSmoothed.setCurrentAndTargetValue (outputGainSmoothed.getTargetValue());
}

void NewProjectAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NewProjectAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void NewProjectAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateSmoothingTargets();

    const auto* modeParameter = parameters.getRawParameterValue ("mode");
    const auto* hqParameter = parameters.getRawParameterValue ("hq");
    const auto mode = static_cast<DistortionMode> (juce::jlimit (0, 3, static_cast<int> (std::round (modeParameter->load()))));
    const auto hqEnabled = hqParameter->load() > 0.5f;

    auto inputSquareSum = 0.0;
    auto outputSquareSum = 0.0;
    auto sampleCount = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto inputGain = inputGainSmoothed.getNextValue();
        const auto drive = driveSmoothed.getNextValue();
        const auto tone = toneSmoothed.getNextValue();
        const auto bias = biasSmoothed.getNextValue();
        const auto mix = mixSmoothed.getNextValue();
        const auto outputGain = outputGainSmoothed.getNextValue();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            const auto channelIndex = static_cast<size_t> (juce::jmin (channel, static_cast<int> (toneState.size() - 1)));

            const auto dry = channelData[sample] * inputGain;
            auto wet = dry;

            if (hqEnabled)
            {
                const auto halfway = (previousSample[channelIndex] + dry) * 0.5f;
                const auto shapedA = processDistortionSample ((halfway * drive) + bias, mode, bias);
                const auto shapedB = processDistortionSample ((dry * drive) + bias, mode, bias);
                wet = (shapedA + shapedB) * 0.5f;
            }
            else
            {
                wet = processDistortionSample ((dry * drive) + bias, mode, bias);
            }

            previousSample[channelIndex] = dry;

            const auto cutoff = juce::jmap (tone, 0.0f, 1.0f, 650.0f, 18000.0f);
            const auto alpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff / static_cast<float> (currentSampleRate));
            toneState[channelIndex] += alpha * (wet - toneState[channelIndex]);

            const auto brightBlend = juce::jmap (tone, 0.0f, 1.0f, 0.08f, 0.82f);
            wet = (toneState[channelIndex] * (1.0f - brightBlend)) + (wet * brightBlend);

            const auto output = ((dry * (1.0f - mix)) + (wet * mix)) * outputGain;
            channelData[sample] = juce::jlimit (-1.0f, 1.0f, output);

            inputSquareSum += static_cast<double> (dry * dry);
            outputSquareSum += static_cast<double> (channelData[sample] * channelData[sample]);
            ++sampleCount;
        }
    }

    if (sampleCount > 0)
    {
        inputLevel.store (static_cast<float> (std::sqrt (inputSquareSum / static_cast<double> (sampleCount))));
        outputLevel.store (static_cast<float> (std::sqrt (outputSquareSum / static_cast<double> (sampleCount))));
    }
}

//==============================================================================
bool NewProjectAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NewProjectAudioProcessor::createEditor()
{
    return new NewProjectAudioProcessorEditor (*this);
}

//==============================================================================
void NewProjectAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NewProjectAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout NewProjectAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> ("input", "Input",
                                                                   juce::NormalisableRange<float> (-18.0f, 18.0f, 0.01f),
                                                                   0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("drive", "Drive",
                                                                   juce::NormalisableRange<float> (1.0f, 28.0f, 0.01f, 0.42f),
                                                                   5.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("tone", "Tone",
                                                                   juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                                   62.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("bias", "Bias",
                                                                   juce::NormalisableRange<float> (-50.0f, 50.0f, 0.01f),
                                                                   0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Mix",
                                                                   juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f),
                                                                   100.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> ("output", "Output",
                                                                   juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f),
                                                                   -3.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("mode", "Mode",
                                                                    juce::StringArray { "Warm", "Hard", "Fold", "Crush" },
                                                                    0));
    params.push_back (std::make_unique<juce::AudioParameterBool> ("hq", "HQ", true));

    return { params.begin(), params.end() };
}

void NewProjectAudioProcessor::updateSmoothingTargets()
{
    inputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("input")->load()));
    driveSmoothed.setTargetValue (parameters.getRawParameterValue ("drive")->load());
    toneSmoothed.setTargetValue (parameters.getRawParameterValue ("tone")->load() * 0.01f);
    biasSmoothed.setTargetValue (parameters.getRawParameterValue ("bias")->load() * 0.01f);
    mixSmoothed.setTargetValue (parameters.getRawParameterValue ("mix")->load() * 0.01f);
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("output")->load()));
}

float NewProjectAudioProcessor::processDistortionSample (float input, DistortionMode mode, float bias) const noexcept
{
    const auto centredBias = std::tanh (bias * 2.3f) * 0.38f;
    auto x = juce::jlimit (-8.0f, 8.0f, input);
    auto y = 0.0f;

    switch (mode)
    {
        case DistortionMode::warmClip:
            y = std::tanh (x * 1.15f);
            break;

        case DistortionMode::hardClip:
            y = juce::jlimit (-1.0f, 1.0f, x * 0.74f);
            break;

        case DistortionMode::foldback:
            x = std::fmod (x + 3.0f, 4.0f);
            if (x < 0.0f)
                x += 4.0f;
            y = 2.0f - std::abs (x - 2.0f);
            y = (y * 2.0f) - 1.0f;
            break;

        case DistortionMode::bitCrush:
        {
            const auto limited = juce::jlimit (-1.0f, 1.0f, std::tanh (x * 0.8f));
            constexpr auto levels = 24.0f;
            y = std::round (limited * levels) / levels;
            break;
        }
    }

    return juce::jlimit (-1.0f, 1.0f, y - centredBias);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NewProjectAudioProcessor();
}
