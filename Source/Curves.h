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

        switch (curveIndex)
        {
            case 0: // Standard Duck (Exponential)
                // Volume recovers exponentially. Very natural pumping.
                return std::pow(phase, 0.5f);

            case 1: // Short Duck (Tight)
                // Recovers very quickly. Good for tight, punchy kicks.
                return std::pow(phase, 0.2f);

            case 2: // Long Duck (Heavy)
                // Recovers slowly. Heavy pumping effect.
                return std::pow(phase, 2.0f);

            case 3: // Gate (Hard cut)
                // Silent for the first 30%, then instantly back to full volume.
                return (phase > 0.3f) ? 1.0f : 0.0f;

            case 4: // Soft Gate
                // Silent for the first 20%, then smooth ramp to full volume.
                if (phase < 0.2f) return 0.0f;
                return std::clamp((phase - 0.2f) * 1.5f, 0.0f, 1.0f);

            case 5: // Sine Pump
                // Smooth sine wave modulation (starts at 0, ramps up, and dips slightly at the end)
                return std::sin(phase * 1.570796f); // phase * PI/2

            case 6: // Triangle (Swell)
                // Ramps up linearly to 1.0 at 50%, then ramps down.
                if (phase < 0.5f) return phase * 2.0f;
                return 1.0f - ((phase - 0.5f) * 2.0f);

            case 7: // Reverse / Late Pump
                // Stays low, then rapidly rises at the very end.
                return std::pow(phase, 4.0f);

            default:
                // Fallback to linear
                return phase;
        }
    }
}
