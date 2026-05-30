#pragma once

#include <JuceHeader.h>

namespace elaw
{
    inline float onePoleAlpha (float cutoffHz, double sampleRate) noexcept
    {
        const auto safeCutoff = juce::jlimit (1.0f, static_cast<float> (sampleRate * 0.45), cutoffHz);
        return 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * safeCutoff / static_cast<float> (sampleRate));
    }

    inline void storeRmsLevels (std::atomic<float>& inputLevel,
                                std::atomic<float>& outputLevel,
                                double inputSquareSum,
                                double outputSquareSum,
                                int sampleCount) noexcept
    {
        if (sampleCount <= 0)
            return;

        inputLevel.store (static_cast<float> (std::sqrt (inputSquareSum / static_cast<double> (sampleCount))));
        outputLevel.store (static_cast<float> (std::sqrt (outputSquareSum / static_cast<double> (sampleCount))));
    }
}
