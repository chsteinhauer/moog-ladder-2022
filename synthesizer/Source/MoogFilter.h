

#pragma once
#include <cmath>
#include <JuceHeader.h>
#include "Circuit.h"

class MoogFilter {
public:
    MoogFilter() = default;
    ~MoogFilter() {};

    void prepare(double sampleRate, int blockSize, int numChannels, AudioProcessorValueTreeState* state) {
        oversampling.reset(new dsp::Oversampling<float>(numChannels, 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));

        oversampling->reset();
        oversampling->initProcessing(blockSize);

        limiter.setThreshold(-10);
        limiter.setRelease(100);
        limiter.prepare({ sampleRate, (juce::uint32)blockSize, (juce::uint32)numChannels });


        circuits.clear();

        for (int i = 0; i < numChannels; i++) {
            Circuit c;

            c.prepare(sampleRate*2, blockSize*2, state);
            circuits.push_back(c);
        }
    }

    void process(dsp::AudioBlock<float> block) {
        auto _block = oversampling->processSamplesUp(block);

        for (int i = 0; i < _block.getNumChannels(); i++) {
            circuits.at(i).run(_block.getSingleChannelBlock(i));
        }

        oversampling->processSamplesDown(block);
        limiter.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    void reset() {
        oversampling.release();
        limiter.reset();
        circuits.clear();
    }


private:

    std::unique_ptr<dsp::Oversampling<float>> oversampling;
    dsp::Limiter<float> limiter;
    std::vector<Circuit> circuits;

    juce::HeapBlock<char> heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;
};

