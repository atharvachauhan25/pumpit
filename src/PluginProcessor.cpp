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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "SHIFT", "Shift",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 1.0f), 0.0f));

    // Hidden parameter purely to force the DAW to recognize non-parameter state changes
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "DIRTY", "Dirty",
        juce::NormalisableRange<float> (0.0f, 1.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "DIVISION", "Division",
        juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2)); // Default to 1/4 note

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "SHAPE", "Shape",
        juce::StringArray { "Standard", "Tight", "Heavy", "Extreme", "Linear", "Classic SC", "Sine", "Triangle", "Soft Gate 25", "Soft Gate 50", "Hard Gate 25", "Hard Gate 50", "Reverse", "Staircase", "Double Pump", "Custom (Draw)" }, 0)); // Added Custom

    return { params.begin(), params.end() };
}

float PumpItAudioProcessor::getCustomCurveValue(float phase)
{
    std::lock_guard<std::mutex> lock(customCurveMutex);
    
    if (customCurvePoints.empty()) return 1.0f;
    if (phase <= customCurvePoints.front().x) return customCurvePoints.front().y;
    if (phase >= customCurvePoints.back().x) return customCurvePoints.back().y;

    for (size_t i = 0; i < customCurvePoints.size() - 1; ++i)
    {
        if (phase >= customCurvePoints[i].x && phase <= customCurvePoints[i+1].x)
        {
            float t = (phase - customCurvePoints[i].x) / (customCurvePoints[i+1].x - customCurvePoints[i].x);
            
            // Apply tension bending
            float tension = customCurvePoints[i].tension;
            if (std::abs(tension) > 0.01f)
            {
                t = (std::exp(t * tension) - 1.0f) / (std::exp(tension) - 1.0f);
            }
            
            return customCurvePoints[i].y + t * (customCurvePoints[i+1].y - customCurvePoints[i].y);
        }
    }
    return 1.0f;
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

    float shiftPercent = apvts.getRawParameterValue ("SHIFT")->load();
    float phaseOffset = shiftPercent / 100.0f; // -1.0 to 1.0

    int divIndex = static_cast<int> (apvts.getRawParameterValue ("DIVISION")->load());
    float divisionMultiplier = 1.0f; // Default 1/4 note
    switch (divIndex) {
        case 0: divisionMultiplier = 4.0f; break; // 1/1
        case 1: divisionMultiplier = 2.0f; break; // 1/2
        case 2: divisionMultiplier = 1.0f; break; // 1/4
        case 3: divisionMultiplier = 0.5f; break; // 1/8
        case 4: divisionMultiplier = 0.25f; break; // 1/16
        case 5: divisionMultiplier = 0.125f; break; // 1/32
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
        
        // Apply phase shift
        hostPhase -= phaseOffset;
        while (hostPhase < 0.0f) hostPhase += 1.0f;
        while (hostPhase >= 1.0f) hostPhase -= 1.0f;
        
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
        // Fetch raw curve value (0.0 to 1.0)
        float curveVal = (shapeIndex == 15) ? getCustomCurveValue(currentPhase) : PumpItCurves::getCurveValue(shapeIndex, currentPhase);

        // In professional volume-shapers (like Kickstart / LFO Tool), the visual Y-axis 
        // maps directly to linear amplitude. The "groove" comes from drawing the perfect 
        // 3. Evaluate Curve at current phase
        float targetDuck = 1.0f;
        if (shapeIndex == 15) targetDuck = getCustomCurveValue(currentPhase);
        else targetDuck = PumpItCurves::getCurveValue(shapeIndex, currentPhase);

        // Apply 1-pole filter to prevent clicking from sudden jumps
        envelopeFilterState = envelopeFilterState * envelopeAlpha + targetDuck * (1.0f - envelopeAlpha);

        // Calculate dry/wet mix
        float finalMultiplier = (1.0f - currentMix) + (currentMix * envelopeFilterState);

        // Get mono input for oscilloscope
        float monoIn = 0.0f;
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            monoIn += buffer.getReadPointer(channel)[s];
        }
        monoIn /= (float)totalNumInputChannels;

        // Apply gain to audio buffers
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            channelData[s] *= finalMultiplier;
        }

        // Write to oscilloscope
        if (isPlaying)
        {
            int scopeIndex = (int)(currentPhase * (scopeSize - 1));
            scopeIndex = juce::jlimit(0, scopeSize - 1, scopeIndex);
            
            float inEnv = std::abs(monoIn);
            float outEnv = std::abs(monoIn * finalMultiplier);
            
            // Peak follower
            inputScope[scopeIndex] = std::max(inputScope[scopeIndex], inEnv);
            outputScope[scopeIndex] = std::max(outputScope[scopeIndex], outEnv);
            
            // Clear ahead of playhead
            int clearIndex = (scopeIndex + 10) % scopeSize;
            inputScope[clearIndex] = 0.0f;
            outputScope[clearIndex] = 0.0f;
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
    
    // Save custom curve points as a simple string attribute
    juce::String curveString;
    {
        std::lock_guard<std::mutex> lock(customCurveMutex);
        for (const auto& pt : customCurvePoints) {
            curveString << pt.x << "," << pt.y << "," << pt.tension << ";";
        }
    }
    if (xml != nullptr) {
        xml->setAttribute("CUSTOM_CURVE_STRING", curveString);
        copyXmlToBinary (*xml, destData);
    }
}

void PumpItAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        // Unconditionally replace state to avoid tag-name mismatch bugs in some hosts
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        
        // Load custom curve points from the string attribute
        juce::String curveString = xmlState->getStringAttribute("CUSTOM_CURVE_STRING", "");
        if (curveString.isNotEmpty())
        {
            std::vector<CurveNode> loadedPoints;
            juce::StringArray parts;
            parts.addTokens(curveString, ";", "");
            for (auto part : parts)
            {
                if (part.isEmpty()) continue;
                juce::StringArray vals;
                vals.addTokens(part, ",", "");
                if (vals.size() >= 3)
                {
                    loadedPoints.push_back({ vals[0].getFloatValue(), vals[1].getFloatValue(), vals[2].getFloatValue() });
                }
            }
            
            if (!loadedPoints.empty())
            {
                std::lock_guard<std::mutex> lock(customCurveMutex);
                customCurvePoints = loadedPoints;
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PumpItAudioProcessor();
}
