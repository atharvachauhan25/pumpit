#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PumpItAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PumpItAudioProcessorEditor (PumpItAudioProcessor&);
    ~PumpItAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    PumpItAudioProcessor& audioProcessor;

    // UI Components
    juce::Slider mixSlider;
    juce::ComboBox divisionBox;
    juce::ComboBox shapeBox;
    juce::Label mixLabel;
    juce::Label divisionLabel;
    juce::Label shapeLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PumpItAudioProcessorEditor)
};
