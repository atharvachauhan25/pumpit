#pragma once

#include <JuceHeader.h>
#include <vector>

#include "PluginProcessor.h"

class CurveEditorComponent : public juce::Component
{
public:
    CurveEditorComponent();
    ~CurveEditorComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    // The normalized points of the curve (x: 0.0 to 1.0, y: 0.0 to 1.0, tension)
    std::vector<PumpItAudioProcessor::CurveNode> points;

    float inputScope[1000] { 0.0f };
    float outputScope[1000] { 0.0f };

    int shapeIndex { 0 };

    // Evaluates the curve at a specific phase (0.0 to 1.0)
    float getCurveValueAt (float phase) const;

    // Let the editor know what the current playback phase is (for the playhead)
    void setPlayheadPhase (float phase) { currentPlayheadPhase = phase; }

    // Callback when the user changes the curve, so we can update the DSP
    std::function<void()> onCurveChanged;

private:
    float currentPlayheadPhase { 0.0f };
    int draggingNodeIndex { -1 };
    int draggingSegmentIndex { -1 };
    float dragStartTension { 0.0f };
    juce::Point<float> dragStartPixel;

    // Helper to convert normalized points to pixel coordinates
    juce::Point<float> getPixelFromNorm (juce::Point<float> normPoint) const;
    juce::Point<float> getNormFromPixel (juce::Point<float> pixelPoint) const;

    // Get the usable drawing area
    juce::Rectangle<float> getDrawArea() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEditorComponent)
};
