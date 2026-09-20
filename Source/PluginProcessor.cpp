#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Curves.h"

PumpItAudioProcessor::PumpItAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

PumpItAudioProcessor::~PumpItAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout PumpItAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "MIX", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "DIVISION", "Division",
        juce::StringArray { "1/8", "1/4", "1/2", "1/1" }, 1)); // Default to 1/4 note

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "SHAPE", "Shape",
        juce::StringArray { "Standard", "Tight", "Heavy", "Extreme", "Linear", "Classic SC", "Sine", "Triangle", "Soft Gate 25", "Soft Gate 50", "Hard Gate 25", "Hard Gate 50", "Reverse", "Staircase", "Double Pump" }, 0)); // Default to Standard

    return { params.begin(), params.end() };
}

const juce::String PumpItAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PumpItAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PumpItAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PumpItAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PumpItAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PumpItAudioProcessor::getNumPrograms()
{
    return 1;
}

int PumpItAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PumpItAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PumpItAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PumpItAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PumpItAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    mixSmoother.reset (sampleRate, 0.05); // 50ms smoothing for mix parameter

    // ~2ms smoothing to remove wrap-around clicks and pops
    if (sampleRate > 0)
        envelopeAlpha = static_cast<float> (std::exp (-1.0 / (sampleRate * 0.002)));
}

void PumpItAudioProcessor::releaseResources()
{
}

bool PumpItAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PumpItAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    // Get parameters
    float mixTarget = apvts.getRawParameterValue ("MIX")->load() / 100.0f;
    mixSmoother.setTargetValue (mixTarget);

    int divIndex = static_cast<int> (apvts.getRawParameterValue ("DIVISION")->load());
    float divisionMultiplier = 1.0f; // Default 1/4 note
    switch (divIndex) {
        case 0: divisionMultiplier = 0.5f; break; // 1/8
        case 1: divisionMultiplier = 1.0f; break; // 1/4
        case 2: divisionMultiplier = 2.0f; break; // 1/2
        case 3: divisionMultiplier = 4.0f; break; // 1/1
    }

    int shapeIndex = static_cast<int> (apvts.getRawParameterValue ("SHAPE")->load());

    // Host Sync Tracking
    auto* playhead = getPlayHead();
    juce::AudioPlayHead::CurrentPositionInfo positionInfo;

    if (playhead != nullptr && playhead->getCurrentPosition (positionInfo))
    {
        isPlaying = positionInfo.isPlaying;
        if (positionInfo.bpm > 0)
            hostBpm = positionInfo.bpm;
    }
    else
    {
        isPlaying = true; // Fallback: always play if host doesn't provide transport info
    }

    // Process audio samples
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();
    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0) sampleRate = 44100.0;

    // Calculate how much phase advances per sample based on BPM and Division
    double cycleLengthSeconds = (60.0 / hostBpm) * divisionMultiplier;
    double phaseIncrement = 1.0 / (cycleLengthSeconds * sampleRate);

    if (isPlaying && playhead != nullptr && playhead->getCurrentPosition(positionInfo))
    {
        double ppq = positionInfo.ppqPosition;
        double wrappedPpq = std::fmod (ppq, static_cast<double>(divisionMultiplier));
        float hostPhase = static_cast<float> (wrappedPpq / divisionMultiplier);
        
        // Prevent block-boundary pops: Only snap our internal phase to the host phase 
        // if they drift too far apart (e.g. user clicked a new spot on the timeline).
        float phaseDiff = std::abs (currentPhase - hostPhase);
        if (phaseDiff > 0.05f && phaseDiff < 0.95f) 
        {
            currentPhase = hostPhase;
        }
    }

    // For each sample, apply ducking
    for (int s = 0; s < numSamples; ++s)
    {
        float currentMix = mixSmoother.getNextValue();
        
        // Always apply ducking! When playing, it's synced to the grid. 
        // When stopped, it free-runs at the same tempo so you can play live.
        float targetDuck = PumpItCurves::getCurveValue(shapeIndex, currentPhase);

        // Apply 1-pole anti-click filter (smooths out the instant 1.0 -> 0.0 wrap)
        envelopeFilterState = (envelopeAlpha * envelopeFilterState) + ((1.0f - envelopeAlpha) * targetDuck);

        // Blend dry and wet based on mix
        float finalVolumeMultiplier = (1.0f - currentMix) + (currentMix * envelopeFilterState);

        // Apply to all channels
        for (int c = 0; c < totalNumInputChannels; ++c)
        {
            auto* channelData = buffer.getWritePointer (c);
            channelData[s] *= finalVolumeMultiplier;
        }

        // Always advance phase
        currentPhase += static_cast<float>(phaseIncrement);
        if (currentPhase >= 1.0f) currentPhase -= 1.0f;
    }
}

bool PumpItAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PumpItAudioProcessor::createEditor()
{
    return new PumpItAudioProcessorEditor (*this);
}

void PumpItAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PumpItAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PumpItAudioProcessor();
}
