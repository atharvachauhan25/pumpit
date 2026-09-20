#pragma once

#include <JuceHeader.h>
#include <mutex>

class PumpItAudioProcessor : public juce::AudioProcessor
{
public:
    PumpItAudioProcessor();
    ~PumpItAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    juce::AudioProcessorValueTreeState apvts;

    struct CurveNode {
        float x { 0.0f };
        float y { 0.0f };
        float tension { 0.0f }; // -5.0 to 5.0 for bending
    };

    // Custom Curve Data
    std::vector<CurveNode> customCurvePoints { {0.0f, 0.0f, 0.0f}, {0.2f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f} };
    std::mutex customCurveMutex;
    float getCustomCurveValue(float phase);

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

    // Getters for UI
    float getCurrentPhase() const { return currentPhase; }
    bool getIsPlaying() const { return isPlaying; }

    static constexpr int scopeSize = 1000;
    float inputScope[scopeSize] { 0.0f };
    float outputScope[scopeSize] { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    float currentPhase { 0.0f };
    double hostBpm { 120.0 };
    bool isPlaying { false };

    // Anti-click envelope filter
    float envelopeFilterState { 1.0f };
    float envelopeAlpha { 0.0f };

    juce::LinearSmoothedValue<float> mixSmoother { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PumpItAudioProcessor)
};
