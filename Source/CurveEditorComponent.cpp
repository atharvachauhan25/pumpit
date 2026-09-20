#include "CurveEditorComponent.h"
#include "Curves.h"
#include <algorithm>

CurveEditorComponent::CurveEditorComponent()
{
    // Initialize with a custom standard curve
    points.push_back ({ 0.0f, 0.0f, 0.0f });
    points.push_back ({ 0.2f, 0.5f, 0.0f });
    points.push_back ({ 1.0f, 1.0f, 0.0f });
}

CurveEditorComponent::~CurveEditorComponent()
{
}

juce::Rectangle<float> CurveEditorComponent::getDrawArea() const
{
    return getLocalBounds().toFloat().reduced(10.0f);
}

juce::Point<float> CurveEditorComponent::getPixelFromNorm (juce::Point<float> normPoint) const
{
    auto area = getDrawArea();
    return {
        area.getX() + normPoint.x * area.getWidth(),
        area.getBottom() - normPoint.y * area.getHeight()
    };
}

juce::Point<float> CurveEditorComponent::getNormFromPixel (juce::Point<float> pixelPoint) const
{
    auto area = getDrawArea();
    float nx = (pixelPoint.x - area.getX()) / area.getWidth();
    float ny = (area.getBottom() - pixelPoint.y) / area.getHeight();
    return { std::clamp(nx, 0.0f, 1.0f), std::clamp(ny, 0.0f, 1.0f) };
}

float CurveEditorComponent::getCurveValueAt (float phase) const
{
    if (shapeIndex != 15) return PumpItCurves::getCurveValue(shapeIndex, phase);

    if (points.empty()) return 1.0f;
    if (phase <= points.front().x) return points.front().y;
    if (phase >= points.back().x) return points.back().y;

    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        if (phase >= points[i].x && phase <= points[i+1].x)
        {
            float t = (phase - points[i].x) / (points[i+1].x - points[i].x);
            float tension = points[i].tension;
            if (std::abs(tension) > 0.01f)
            {
                t = (std::exp(t * tension) - 1.0f) / (std::exp(tension) - 1.0f);
            }
            return points[i].y + t * (points[i+1].y - points[i].y);
        }
    }
    return 1.0f;
}

void CurveEditorComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour (juce::Colour (0xff252525));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (juce::Colours::grey);
    g.drawRoundedRectangle (bounds, 10.0f, 2.0f);

    auto drawArea = getDrawArea();
    g.saveState();
    g.reduceClipRegion (drawArea.toNearestInt());

    // 1. Draw the curve path
    juce::Path curvePath;
    float startX = drawArea.getX();
    float width = drawArea.getWidth();
    float bottomY = drawArea.getBottom();
    float height = drawArea.getHeight();

    for (int i = 0; i <= width; ++i)
    {
        float phase = i / width;
        float curveVal = getCurveValueAt(phase);
        float y = bottomY - (curveVal * height);
        
        if (i == 0) curvePath.startNewSubPath(startX + i, y);
        else        curvePath.lineTo(startX + i, y);
    }
    
    g.setColour (juce::Colours::cyan.withAlpha(0.7f));
    g.strokePath (curvePath, juce::PathStrokeType(3.0f));

    // 2. Draw Playhead Trail and Dot
    if (currentPlayheadPhase >= 0.0f) // always draw if >= 0
    {
        float playheadX = drawArea.getX() + (currentPlayheadPhase * drawArea.getWidth());
        float currentY = drawArea.getBottom() - (getCurveValueAt(currentPlayheadPhase) * drawArea.getHeight());
        
        // Fading trail backwards
        for (int i = 0; i < 40; ++i) 
        {
            float p1 = currentPlayheadPhase - (i * 0.004f);
            float p2 = currentPlayheadPhase - ((i + 1) * 0.004f);
            
            if (p1 < 0.0f) p1 += 1.0f;
            if (p2 < 0.0f) p2 += 1.0f;
            
            if (p2 > p1) continue; 
            
            float x1 = drawArea.getX() + (p1 * drawArea.getWidth());
            float y1 = drawArea.getBottom() - (getCurveValueAt(p1) * drawArea.getHeight());
            float x2 = drawArea.getX() + (p2 * drawArea.getWidth());
            float y2 = drawArea.getBottom() - (getCurveValueAt(p2) * drawArea.getHeight());
            
            float alpha = 0.8f * (1.0f - (i / 40.0f));
            g.setColour (juce::Colours::cyan.withAlpha (alpha));
            g.drawLine (x1, y1, x2, y2, 6.0f);
        }

        // Faint vertical line
        g.setColour (juce::Colours::white.withAlpha(0.15f));
        g.drawLine (playheadX, drawArea.getY(), playheadX, drawArea.getBottom(), 1.0f);

        // Glowing Point
        g.setColour (juce::Colours::cyan.withAlpha(0.6f));
        g.fillEllipse (playheadX - 8.0f, currentY - 8.0f, 16.0f, 16.0f);
        g.setColour (juce::Colours::white);
        g.fillEllipse (playheadX - 3.0f, currentY - 3.0f, 6.0f, 6.0f);
    }

    // 3. Draw nodes (handles) ONLY in custom mode
    if (shapeIndex == 15)
    {
        // Draw Tension Handles (hollow circles in the middle of each segment)
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            float midX = (points[i].x + points[i+1].x) * 0.5f;
            float midY = getCurveValueAt(midX);
            auto midP = getPixelFromNorm({midX, midY});
            
            g.setColour (i == draggingSegmentIndex ? juce::Colours::white : juce::Colours::cyan.withAlpha(0.6f));
            g.drawEllipse (midP.x - 4.0f, midP.y - 4.0f, 8.0f, 8.0f, 2.0f);
        }

        // Draw Main Nodes (solid circles)
        for (size_t i = 0; i < points.size(); ++i)
        {
            auto p = getPixelFromNorm({points[i].x, points[i].y});
            g.setColour (i == draggingNodeIndex ? juce::Colours::white : juce::Colours::cyan);
            g.fillEllipse (p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
        }
    }

    g.restoreState();
}

void CurveEditorComponent::resized()
{
}

void CurveEditorComponent::mouseDown (const juce::MouseEvent& e)
{
    if (shapeIndex != 15) return;
    auto clickNorm = getNormFromPixel(e.position);
    
    draggingNodeIndex = -1;
    draggingSegmentIndex = -1;
    for (size_t i = 0; i < points.size(); ++i)
    {
        auto nodePixel = getPixelFromNorm({points[i].x, points[i].y});
        if (nodePixel.getDistanceFrom(e.position) < 10.0f)
        {
            draggingNodeIndex = (int)i;
            break;
        }
    }

    if (draggingNodeIndex == -1)
    {
        // 1. Check if we clicked exactly on a tension handle
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            float midX = (points[i].x + points[i+1].x) * 0.5f;
            float midY = getCurveValueAt(midX);
            auto midPixel = getPixelFromNorm({midX, midY});
            
            if (midPixel.getDistanceFrom(e.position) < 15.0f)
            {
                draggingSegmentIndex = (int)i;
                dragStartTension = points[i].tension;
                dragStartPixel = e.position;
                break;
            }
        }
        
        // 2. Fallback: clicked the segment area roughly
        if (draggingSegmentIndex == -1)
        {
            for (size_t i = 0; i < points.size() - 1; ++i)
            {
                if (clickNorm.x >= points[i].x && clickNorm.x <= points[i+1].x)
                {
                    draggingSegmentIndex = (int)i;
                    dragStartTension = points[i].tension;
                    dragStartPixel = e.position;
                    break;
                }
            }
        }
    }

    repaint();
}

void CurveEditorComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (shapeIndex != 15) return;
    
    if (draggingNodeIndex >= 0 && draggingNodeIndex < points.size())
    {
        auto norm = getNormFromPixel(e.position);
        
        if (draggingNodeIndex == 0) norm.x = 0.0f;
        if (draggingNodeIndex == points.size() - 1) norm.x = 1.0f;

        if (draggingNodeIndex > 0)
            norm.x = std::max(norm.x, points[draggingNodeIndex - 1].x + 0.01f);
        
        if (draggingNodeIndex < points.size() - 1)
            norm.x = std::min(norm.x, points[draggingNodeIndex + 1].x - 0.01f);

        points[draggingNodeIndex].x = norm.x;
        points[draggingNodeIndex].y = norm.y;
        
        if (onCurveChanged) onCurveChanged();
        repaint();
    }
    else if (draggingSegmentIndex >= 0 && draggingSegmentIndex < points.size() - 1)
    {
        float dy = e.position.y - dragStartPixel.y;
        float tensionDelta = dy * 0.05f; 
        points[draggingSegmentIndex].tension = std::clamp(dragStartTension + tensionDelta, -10.0f, 10.0f);
        
        if (onCurveChanged) onCurveChanged();
        repaint();
    }
}

void CurveEditorComponent::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);
    draggingNodeIndex = -1;
    draggingSegmentIndex = -1;
    repaint();
}

void CurveEditorComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (shapeIndex != 15) return;

    auto clickNorm = getNormFromPixel(e.position);
    for (size_t i = 1; i < points.size() - 1; ++i) 
    {
        auto nodePixel = getPixelFromNorm({points[i].x, points[i].y});
        if (nodePixel.getDistanceFrom(e.position) < 12.0f)
        {
            points.erase(points.begin() + i);
            if (onCurveChanged) onCurveChanged();
            repaint();
            return;
        }
    }

    for (auto it = points.begin(); it != points.end(); ++it)
    {
        if (it->x > clickNorm.x)
        {
            points.insert(it, {clickNorm.x, clickNorm.y, 0.0f});
            if (onCurveChanged) onCurveChanged();
            repaint();
            return;
        }
    }
}
