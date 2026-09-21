#include "PluginProcessor.h"
#include "PluginEditor.h"

PumpItAudioProcessorEditor::PumpItAudioProcessorEditor (PumpItAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    // Mix Slider
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);
    
    mixLabel.setText ("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (mixLabel);

    // Shift Slider
    shiftSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    shiftSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (shiftSlider);
    shiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "SHIFT", shiftSlider);
    
    shiftLabel.setText ("Shift", juce::dontSendNotification);
    shiftLabel.setJustificationType (juce::Justification::centred);
    shiftLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (shiftLabel);

    // Division ComboBox
    divisionBox.addItemList (juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 1);
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "DIVISION", divisionBox);

    divisionLabel.setText ("Division", juce::dontSendNotification);
    divisionLabel.setJustificationType (juce::Justification::centred);
    divisionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (divisionLabel);

    // Configure Shape ComboBox
    shapeBox.addItemList (juce::StringArray { "Standard", "Tight", "Heavy", "Extreme", "Linear", "Classic SC", "Sine", "Triangle", "Soft Gate 25", "Soft Gate 50", "Hard Gate 25", "Hard Gate 50", "Reverse", "Staircase", "Double Pump", "Custom (Draw)" }, 1);
    addAndMakeVisible (shapeBox);
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SHAPE", shapeBox);

    shapeLabel.setText ("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    shapeLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (shapeLabel);

    // Curve Editor Component
    addAndMakeVisible (curveEditor);
    
    // Load existing custom points from DSP!
    {
        std::lock_guard<std::mutex> lock(audioProcessor.customCurveMutex);
        curveEditor.points = audioProcessor.customCurvePoints;
    }

    // Sync points when custom curve changes
    curveEditor.onCurveChanged = [this]() {
        std::lock_guard<std::mutex> lock(audioProcessor.customCurveMutex);
        audioProcessor.customCurvePoints = curveEditor.points;
        
        juce::String curveString;
        for (const auto& pt : curveEditor.points) {
            curveString << pt.x << "," << pt.y << "," << pt.tension << ";";
        }
        audioProcessor.apvts.state.setProperty("CUSTOM_CURVE_STRING", curveString, nullptr);
    };

    setSize (600, 440); // Increased height slightly for header/footer
    startTimerHz(30); 
}

PumpItAudioProcessorEditor::~PumpItAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    stopTimer();
}

void PumpItAudioProcessorEditor::timerCallback()
{
    int shapeIndex = audioProcessor.apvts.getRawParameterValue("SHAPE")->load();
    curveEditor.shapeIndex = shapeIndex;

    // Copy scope data for oscilloscope
    std::copy(std::begin(audioProcessor.inputScope), std::end(audioProcessor.inputScope), std::begin(curveEditor.inputScope));
    std::copy(std::begin(audioProcessor.outputScope), std::end(audioProcessor.outputScope), std::begin(curveEditor.outputScope));

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
    // Sleek solid dark background
    g.fillAll (juce::Colour(0xff121212)); 

    // Draw Modern Title (Top Left)
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(28.0f, juce::Font::bold));
    g.drawText ("PUMP", 25, 15, 100, 40, juce::Justification::centredLeft, true);
    
    g.setColour (juce::Colours::cyan);
    g.drawText ("IT", 108, 15, 100, 40, juce::Justification::centredLeft, true);

    // Subtle Brand Name (Far Down Right)
    g.setColour (juce::Colour(0xff555555));
    g.setFont (11.0f);
    g.drawText ("by Atharva Chauhan", getWidth() - 150, getHeight() - 25, 130, 20, juce::Justification::centredRight, true);
}

void PumpItAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Header space
    area.removeFromTop (60); 
    
    // Footer space
    area.removeFromBottom (30);

    // Position Visualizer in the top part of the remaining area
    curveEditor.setBounds (area.removeFromTop (220).reduced (20, 0));
    
    // Bottom area for controls
    auto controlsArea = area;
    int sectionWidth = controlsArea.getWidth() / 3;

    // Mix (Left)
    auto mixArea = controlsArea.removeFromLeft(sectionWidth);
    mixLabel.setBounds (mixArea.removeFromTop(40));
    mixSlider.setBounds (mixArea.withSizeKeepingCentre(100, 100));

    // Shift (Right)
    auto shiftArea = controlsArea.removeFromRight(sectionWidth);
    shiftLabel.setBounds (shiftArea.removeFromTop(40));
    shiftSlider.setBounds (shiftArea.withSizeKeepingCentre(100, 100));

    // Selectors (Middle)
    auto middleArea = controlsArea;
    middleArea.reduce (0, 15); // Use full width, just padding on top/bottom

    // Division Row
    auto divRow = middleArea.removeFromTop(middleArea.getHeight() / 2);
    auto divCenter = divRow.withSizeKeepingCentre(190, 30); // 60 (Label) + 10 (Space) + 120 (Box)
    
    divisionLabel.setBounds (divCenter.removeFromLeft(60));
    divisionLabel.setJustificationType(juce::Justification::centredRight);
    divCenter.removeFromLeft(10); // Spacing
    divisionBox.setBounds (divCenter.removeFromLeft(120));

    // Shape Row
    auto shapeRow = middleArea;
    auto shapeCenter = shapeRow.withSizeKeepingCentre(190, 30);
    
    shapeLabel.setBounds (shapeCenter.removeFromLeft(60));
    shapeLabel.setJustificationType(juce::Justification::centredRight);
    shapeCenter.removeFromLeft(10); // Spacing
    shapeBox.setBounds (shapeCenter.removeFromLeft(120));
}
