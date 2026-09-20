#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CurveEditorComponent.h"

class PumpItAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    PumpItAudioProcessorEditor (PumpItAudioProcessor&);
    ~PumpItAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    PumpItAudioProcessor& audioProcessor;

    juce::Slider mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;

    juce::ComboBox divisionBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;

    juce::ComboBox shapeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;

    juce::Label mixLabel;
    juce::Label divisionLabel;
    juce::Label shapeLabel;

    CurveEditorComponent curveEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PumpItAudioProcessorEditor)
};
