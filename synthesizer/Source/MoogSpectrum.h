

#include <JuceHeader.h>

#pragma once
class MoogSpectrum  : public juce::Component, public juce::Timer {
    enum
    {
        fftOrder = 11,
        fftSize = 1 << fftOrder,
        scopeSize = 512,
    };

    public:
        MoogSpectrum() : forwardFFT(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann)
        {
            setOpaque(true);
            startTimerHz(20);
            setSize(400, 200);
        }

        ~MoogSpectrum() {}

        void timerCallback() override
        {
            if (nextFFTBlockReady)
            {
                drawNextFrameOfSpectrum();
                nextFFTBlockReady = false;
                repaint();
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId).darker(0.95F));

            g.setOpacity(0.9f);
            g.setColour(juce::Colours::forestgreen);
            drawFrame(g);
        }

        void drawNextFrameOfSpectrum()
        {
            // first apply a windowing function to our data
            window.multiplyWithWindowingTable(fftData, fftSize);

            // then render our FFT data..
            forwardFFT.performFrequencyOnlyForwardTransform(fftData);

            auto mindB = -100.0f;
            auto maxdB = 0.0f;

            for (int i = 0; i < scopeSize; ++i)
            {
                auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.2f);
                auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * (float)fftSize * 0.5));
                auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(fftData[fftDataIndex])
                    - juce::Decibels::gainToDecibels((float)fftSize)),
                    mindB, maxdB, 0.0f, 1.0f);
                scopeData[i] = level;
            }
        }
        void drawFrame(juce::Graphics& g)
        {
            for (int i = 1; i < scopeSize; ++i)
            {
                auto width = getLocalBounds().getWidth();
                auto height = getLocalBounds().getHeight();

                g.drawLine({ (float)juce::jmap(i - 1, 0, scopeSize - 1, 0, width),
                                      juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
                              (float)juce::jmap(i,     0, scopeSize - 1, 0, width),
                                      juce::jmap(scopeData[i],     0.0f, 1.0f, (float)height, 0.0f) });
            }
        }

        void pushNextSampleIntoFifo(float sample) noexcept
        {
            if (fifoIndex == fftSize)
            {
                if (!nextFFTBlockReady)
                {
                    juce::zeromem(fftData, sizeof(fftData));
                    memcpy(fftData, fifo, sizeof(fifo));
                    nextFFTBlockReady = true;
                }

                fifoIndex = 0;
            }

            fifo[fifoIndex++] = sample;
        }

        void nextBlock(dsp::AudioBlock<float> block)
        {
            for (int i = 0; i < block.getNumChannels(); i++) {
                for (int j = 0; j < block.getNumSamples(); j++) {
                    pushNextSampleIntoFifo(block.getSample(i, j));
                }
            }
        }

    private:
        juce::dsp::FFT forwardFFT;
        juce::dsp::WindowingFunction<float> window;

        float fifo[fftSize];
        float fftData[2 * fftSize];
        int fifoIndex = 0;
        bool nextFFTBlockReady = false;
        float scopeData[scopeSize];
        juce::Line<float> threshold;

        std::string title;
        bool showThreshold = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoogSpectrum)
};