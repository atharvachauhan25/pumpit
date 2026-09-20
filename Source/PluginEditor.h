#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CurveEditorComponent.h"

class PumpItLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PumpItLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId, juce::Colours::cyan);
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff222222));
        
        setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xff202020));
        setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::arrowColourId, juce::Colours::cyan);
        
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour(0xff202020));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff333333));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::cyan);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 8.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Track
        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.drawEllipse (rx, ry, rw, rw, 8.0f);

        // Fill
        juce::Path p;
        p.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (p, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Text (in the center)
        g.setColour (juce::Colours::white);
        g.setFont (15.0f);
        juce::String text = juce::String (slider.getValue(), 0) + "%";
        g.drawText (text, x, y, width, height, juce::Justification::centred);
    }
};

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

    PumpItLookAndFeel customLookAndFeel;

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
