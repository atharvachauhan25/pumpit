#include "PluginProcessor.h"
#include "PluginEditor.h"

PumpItAudioProcessorEditor::PumpItAudioProcessorEditor (PumpItAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 400);
}

PumpItAudioProcessorEditor::~PumpItAudioProcessorEditor()
{
}

void PumpItAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("PumpIt - Coming Soon!", getLocalBounds(), juce::Justification::centred, 1);
}

void PumpItAudioProcessorEditor::resized()
{
}
