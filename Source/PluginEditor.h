#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PumpItAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    PumpItAudioProcessorEditor (PumpItAudioProcessor&);
    ~PumpItAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PumpItAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PumpItAudioProcessorEditor)
};
