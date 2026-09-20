#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Curves.h"

PumpItAudioProcessorEditor::PumpItAudioProcessorEditor (PumpItAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Configure Mix Slider
    mixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIX", mixSlider);

    mixLabel.setText ("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (mixLabel);

    // Configure Division ComboBox
    divisionBox.addItemList (juce::StringArray { "1/8", "1/4", "1/2", "1/1" }, 1);
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "DIVISION", divisionBox);

    divisionLabel.setText ("Division", juce::dontSendNotification);
    divisionLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (divisionLabel);

    // Configure Shape ComboBox
    shapeBox.addItemList (juce::StringArray { "Standard", "Tight", "Heavy", "Extreme", "Linear", "Classic SC", "Sine", "Triangle", "Soft Gate 25", "Soft Gate 50", "Hard Gate 25", "Hard Gate 50", "Reverse", "Staircase", "Double Pump" }, 1);
    addAndMakeVisible (shapeBox);
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "SHAPE", shapeBox);

    shapeLabel.setText ("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shapeLabel);

    setSize (600, 400);
    startTimerHz(30); // 30 FPS for visualizer
}

PumpItAudioProcessorEditor::~PumpItAudioProcessorEditor()
{
    stopTimer();
}

void PumpItAudioProcessorEditor::timerCallback()
{
    repaint();
}

void PumpItAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (0xff1a1a1a));

    // Draw the visualizer background
    auto bounds = getLocalBounds();
    auto visualizerBounds = bounds.reduced (20).withHeight(200);
    
    g.setColour (juce::Colour (0xff252525));
    g.fillRoundedRectangle (visualizerBounds.toFloat(), 10.0f);
    g.setColour (juce::Colours::grey);
    g.drawRoundedRectangle (visualizerBounds.toFloat(), 10.0f, 2.0f);

    // Use an inner bounds for the math drawing to strictly prevent any graphics bleeding over the edges
    auto drawBounds = visualizerBounds.reduced (8);

    // Save state so we can clip the drawing region 
    g.saveState();
    g.reduceClipRegion (drawBounds); 

    // Draw the selected Curve Shape
    int shapeIndex = audioProcessor.apvts.getRawParameterValue("SHAPE")->load();
    juce::Path curvePath;
    
    float startX = drawBounds.getX();
    float width = drawBounds.getWidth();
    float bottomY = drawBounds.getBottom();
    float height = drawBounds.getHeight();

    for (int i = 0; i <= width; ++i)
    {
        float phase = i / width;
        
        // getCurveValue returns 0.0 (silent/ducked) to 1.0 (full volume)
        float curveVal = PumpItCurves::getCurveValue(shapeIndex, phase);
        
        // Map 0.0 to bottom Y, and 1.0 to top Y
        float y = bottomY - (curveVal * height);
        
        if (i == 0) curvePath.startNewSubPath(startX + i, y);
        else        curvePath.lineTo(startX + i, y);
    }
    
    g.setColour (juce::Colours::cyan.withAlpha(0.2f));
    g.strokePath (curvePath, juce::PathStrokeType(3.0f));

    // Draw the Playhead and Trail ONLY if host is actively playing
    if (audioProcessor.getIsPlaying())
    {
        float currentPhase = audioProcessor.getCurrentPhase();
        float playheadX = startX + (currentPhase * width);
        float currentY = bottomY - (PumpItCurves::getCurveValue(shapeIndex, currentPhase) * height);
        
        // 1. Draw fading trail backwards
        for (int i = 0; i < 40; ++i) 
        {
            float p1 = currentPhase - (i * 0.004f);
            float p2 = currentPhase - ((i + 1) * 0.004f);
            
            // Wrap around handling
            if (p1 < 0.0f) p1 += 1.0f;
            if (p2 < 0.0f) p2 += 1.0f;
            
            // Don't draw across the wrap boundary to avoid horizontal glitch line
            if (p2 > p1) continue; 
            
            float x1 = startX + (p1 * width);
            float y1 = bottomY - (PumpItCurves::getCurveValue(shapeIndex, p1) * height);
            float x2 = startX + (p2 * width);
            float y2 = bottomY - (PumpItCurves::getCurveValue(shapeIndex, p2) * height);
            
            float alpha = 0.8f * (1.0f - (i / 40.0f));
            g.setColour (juce::Colours::cyan.withAlpha (alpha));
            g.drawLine (x1, y1, x2, y2, 6.0f);
        }

        // 2. Draw a faint vertical line just for alignment reference
        g.setColour (juce::Colours::white.withAlpha(0.15f));
        g.drawLine (playheadX, visualizerBounds.getY(), playheadX, visualizerBounds.getBottom(), 1.0f);

        // 3. Draw the Glowing Point on the curve
        // Glow (outer circle)
        g.setColour (juce::Colours::cyan.withAlpha(0.6f));
        g.fillEllipse (playheadX - 8.0f, currentY - 8.0f, 16.0f, 16.0f);
        
        // Core (inner circle)
        g.setColour (juce::Colours::white);
        g.fillEllipse (playheadX - 3.0f, currentY - 3.0f, 6.0f, 6.0f);
    }

    g.restoreState(); // Restore clip region
}

void PumpItAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Bottom section for controls
    auto controlsArea = bounds.removeFromBottom (150).reduced (20);
    
    int itemWidth = controlsArea.getWidth() / 3;

    // Mix Area
    auto mixArea = controlsArea.removeFromLeft(itemWidth);
    mixLabel.setBounds (mixArea.removeFromTop(20));
    mixSlider.setBounds (mixArea);

    // Division Area
    auto divArea = controlsArea.removeFromLeft(itemWidth);
    divisionLabel.setBounds (divArea.removeFromTop(20));
    divisionBox.setBounds (divArea.withSizeKeepingCentre(100, 30));

    // Shape Area
    auto shapeArea = controlsArea;
    shapeLabel.setBounds (shapeArea.removeFromTop(20));
    shapeBox.setBounds (shapeArea.withSizeKeepingCentre(120, 30));
}
