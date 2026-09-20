#pragma once

#include <cmath>
#include <algorithm>

namespace PumpItCurves
{
    // A ducking curve takes a phase (0.0 to 1.0) and returns a volume multiplier (0.0 to 1.0)
    // phase = 0.0 is the exact moment the beat hits (ducked volume).
    // phase = 1.0 is right before the next beat hits (fully recovered volume).

    inline float getCurveValue (int curveIndex, float phase)
    {
        // Clamp phase just in case
        phase = std::clamp(phase, 0.0f, 1.0f);
        float val = 0.0f;

        switch (curveIndex)
        {
            case 0:  // Standard Duck
                val = std::pow(phase, 0.5f); break;
            case 1:  // Tight Duck (Faster recovery)
                val = std::pow(phase, 0.2f); break;
            case 2:  // Heavy Duck (Slower recovery)
                val = std::pow(phase, 1.5f); break;
            case 3:  // Extreme Duck
                val = std::pow(phase, 3.0f); break;
            case 4:  // Linear
                val = phase; break;
            case 5:  // Classic Sidechain (S-curve)
                val = (phase < 0.5f) ? (2.0f * phase * phase) : (1.0f - std::pow(-2.0f * phase + 2.0f, 2.0f) / 2.0f); break;
            case 6:  // Sine Pump
                val = std::sin(phase * 1.570796f); break; 
            case 7:  // Triangle Swell
                val = (phase < 0.5f) ? (phase * 2.0f) : (1.0f - ((phase - 0.5f) * 2.0f)); break;
            case 8:  // Soft Gate 25%
                val = (phase < 0.25f) ? 0.0f : std::clamp((phase - 0.25f) * 2.0f, 0.0f, 1.0f); break;
            case 9:  // Soft Gate 50%
                val = (phase < 0.5f) ? 0.0f : std::clamp((phase - 0.5f) * 2.0f, 0.0f, 1.0f); break;
            case 10: // Hard Gate 25%
                val = (phase > 0.25f) ? 1.0f : 0.0f; break;
            case 11: // Hard Gate 50%
                val = (phase > 0.5f) ? 1.0f : 0.0f; break;
            case 12: // Reverse Pump
                val = std::pow(phase, 5.0f); break;
            case 13: // Staircase (3 steps)
                if (phase < 0.33f) val = 0.0f;
                else if (phase < 0.66f) val = 0.5f;
                else val = 1.0f;
                break;
            case 14: // Double Pump (2 pumps per beat)
                val = std::pow(std::fmod(phase * 2.0f, 1.0f), 0.5f); break;
            default:
                val = std::pow(phase, 0.5f); break;
        }

        // Anti-pop: Force all curves to ramp down to 0.0 at the very end (last 3% of the cycle).
        if (phase > 0.97f)
        {
            float fade = (1.0f - phase) / 0.03f;
            val *= fade;
        }

        return val;
    }
}
