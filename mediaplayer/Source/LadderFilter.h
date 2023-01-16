

#pragma once
#include <cmath>
#include <JuceHeader.h>
#include "Circuit.h"
#include "DSPDemos_Common.h"

class MoogLadderFilter {
public:
    MoogLadderFilter() = default;
    ~MoogLadderFilter() {};

    void prepare(double sampleRate, int blockSize, int numChannels) {
        circuits.clear();

        for (int i = 0; i < numChannels; i++) {
            Circuit c;

            c.prepare(sampleRate,blockSize);
            circuits.push_back(c);
        }
    }

    void process(dsp::AudioBlock<float> block) {

        for (int i = 0; i < block.getNumChannels(); i++) {
            circuits.at(i).run(
                block.getSingleChannelBlock(i), 
                Fc->getCurrentValue(), 
                r->getCurrentValue(),
                Ia->getCurrentValue()
            );
        }
    }

    void reset() {
        circuits.clear();
    }

    void setParameters(SliderParameter* _Fc, SliderParameter* _r, SliderParameter* _Ia) {
        Fc = _Fc;
        r = _r;
        Ia = _Ia;
    }

    SliderParameter* Fc;
    SliderParameter* r;
    SliderParameter* Ia;

private:

    std::vector<Circuit> circuits;
};

