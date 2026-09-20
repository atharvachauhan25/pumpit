#include "PluginProcessor.h"
#include "PluginEditor.h"

PumpItAudioProcessorEditor::PumpItAudioProcessorEditor (PumpItAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Mix Slider
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);
    
    mixLabel.setText ("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mixLabel);

    // Division ComboBox
    divisionBox.addItemList (juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 1);
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "DIVISION", divisionBox);

    divisionLabel.setText ("Division", juce::dontSendNotification);
    divisionLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (divisionLabel);

    // Configure Shape ComboBox
    shapeBox.addItemList (juce::StringArray { "Standard", "Tight", "Heavy", "Extreme", "Linear", "Classic SC", "Sine", "Triangle", "Soft Gate 25", "Soft Gate 50", "Hard Gate 25", "Hard Gate 50", "Reverse", "Staircase", "Double Pump", "Custom (Draw)" }, 1);
    addAndMakeVisible (shapeBox);
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SHAPE", shapeBox);

    shapeLabel.setText ("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shapeLabel);

    // Curve Editor Component
    addAndMakeVisible (curveEditor);
    
    // Sync points when custom curve changes
    curveEditor.onCurveChanged = [this]() {
        std::lock_guard<std::mutex> lock(audioProcessor.customCurveMutex);
        audioProcessor.customCurvePoints = curveEditor.points;
    };

    setSize (600, 400);
    startTimerHz(30); 
}

PumpItAudioProcessorEditor::~PumpItAudioProcessorEditor()
{
    stopTimer();
}

void PumpItAudioProcessorEditor::timerCallback()
{
    int shapeIndex = audioProcessor.apvts.getRawParameterValue("SHAPE")->load();
    curveEditor.shapeIndex = shapeIndex;

    if (audioProcessor.getIsPlaying())
    {
        curveEditor.setPlayheadPhase (audioProcessor.getCurrentPhase());
    }
    else
    {
        curveEditor.setPlayheadPhase (-1.0f); // Hide playhead
    }

    curveEditor.repaint();
}

void PumpItAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
}

void PumpItAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Position Visualizer in the top half
    curveEditor.setBounds (area.removeFromTop(240).reduced(20));
    
    // Bottom area for controls
    auto controlsArea = area;
    int sectionWidth = controlsArea.getWidth() / 3;

    // Mix (Left)
    auto mixArea = controlsArea.removeFromLeft(sectionWidth);
    mixLabel.setBounds (mixArea.removeFromTop(40));
    mixSlider.setBounds (mixArea.reduced(10));

    // Division (Middle)
    auto divArea = controlsArea.removeFromLeft(sectionWidth);
    divisionLabel.setBounds (divArea.removeFromTop(40));
    divisionBox.setBounds (divArea.withSizeKeepingCentre(100, 30));

    // Shape (Right)
    auto shapeArea = controlsArea;
    shapeLabel.setBounds (shapeArea.removeFromTop(40));
    shapeBox.setBounds (shapeArea.withSizeKeepingCentre(120, 30));
}
