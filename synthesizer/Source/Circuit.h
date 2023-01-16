
#pragma once
#include <cmath>
#include <JuceHeader.h>
#include "CircularCache.h"

struct Circuit {
    enum stage {
        A = 0,
        B = 1,
        C = 2,
        D = 3,
    };

    /*
     * Moog ladder stages
     */

     // stage a
    float a(int n) {
        if (cache.has(n, A)) return cache.get(n, A);

        float yd = d(n - 1);
        float ya = a(n - 1);
        float y = ya + G * (W((x - order * r * yd)) - W(ya));

        if (x > 0) {
            auto test = 0;
        }

        cache.set(n, A, y);

        return y;
    }

    // stage b
    float b(int n) {
        if (cache.has(n, B)) return cache.get(n, B);

        float ya = a(n);
        float yb = b(n - 1);
        float y = yb + G * (W(ya) - W(yb));

        cache.set(n, B, y);

        return y;
    }

    // stage c
    float c(int n) {
        if (cache.has(n, C)) return cache.get(n, C);

        float yb = b(n);
        float yc = c(n - 1);
        float y = yc + G * (W(yb) - W(yc));

        cache.set(n, C, y);

        return y;
    }

    // stage d
    float d(int n) {
        if (cache.has(n, D)) return cache.get(n, D);

        float yc = c(n);
        float yd = d(n - 1);
        float y = yd + G * (W(yc) - W(yd));

        cache.set(n, D, y);

        return y;
    }


    /*
     * Helper functions
     */

    float W(float y) {
        return tanh(y / (static_cast<float>(2) * Vt));
    }

    float calcG() {
        float g = static_cast<float>(1) - pow(juce::MathConstants<float>::euler, (-static_cast<float>(2) * juce::MathConstants<float>::pi * (Fc / Fs)));

        return static_cast<float>(2) * Vt * g;
    }

    void update() {
        float _Fc = get("Fc");
        float _r = get("r");

        if (Fc != _Fc) {
            Fc = _Fc;
            G = calcG();
        }

        if (r != _r) {
            r = _r;
        }
    }

    float get(std::string id) {
        auto par = state->getParameter(id);

        return par->convertFrom0to1(par->getValue());
    }

    /*
     * Lifecycle
     */

    void prepare(double sampleRate, int blockSize, AudioProcessorValueTreeState* _state) {
        Fs = sampleRate;
        size = blockSize;
        state = _state;

        Fc = get("Fc");
        r = get("r");
        G = calcG();
    }

    void run(dsp::AudioBlock<float> block) {
        update();

        // input amplitude
        float Ia = get("Ia");
        // make-up gain for input amplitude
        float gain = 1 + 1 - Ia;

        for (int i = 0; i < size; i++) {
            x = block.getSample(0, i) * Ia;

            float y = a(i) * gain;
            block.setSample(0, i, y);
        };
    }

    void reset() {
        cache.reset();
    }


    /*
     * Variables & constants
     */

     // Current value
    float x;
    // Factor based on frequency cut-off 
    float G;

    // Frequency cut-off
    float Fc;
    // Resonant factor
    float r;

    // Sample rate
    double Fs;
    // Audio block size
    float size;

    // Thermal voltage of a transistor
    const float Vt = 0.0026;
    // Number of stages
    const int order = 4;

    // Cache containing previously calculated values
    CircularCache cache;

    AudioProcessorValueTreeState* state;
};
