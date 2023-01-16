

#pragma once

#include <JuceHeader.h>
#include "DemoUtilities.h"
#include "LadderFilter.h"
#include "SpectrumVisualizer.h"
#include "DSPDemos_Common.h"

class PlayerController
{
public:
    void prepare (const ProcessSpec& spec);
    void process (const ProcessContextReplacing<float>& context);
    void reset();
    void updateParameters();

    //==============================================================================

    ChoiceParameter type { { "Normal", "Ladder-Filter"}, 1, "Type"};
    SliderParameter Fc { { 20.0, 20000.0 }, 0.5, 440.0f, "Frequency cut-off", "Hz" };
    SliderParameter r { { 0.0, 4.0 }, 1.0, 0.2, "Resonance" };
    SliderParameter Ia { { 0.0, 2.0 }, 0.5, 0.2, "Input amplitude" };

    std::vector<DSPDemoParameterBase*> parameters { &type, &Ia, &Fc, &r };
    double sampleRate = 0.0;

    

    struct LadderFilterOversampling
    {
        void prepare(const ProcessSpec& spec);
        dsp::AudioBlock<float> processUp(dsp::AudioBlock<float> block);
        void processDown(dsp::AudioBlock<float> block);
        void reset();

        std::unique_ptr<dsp::Oversampling<float>> oversampling;
    };

    LadderFilterOversampling os;
    Limiter<float> limiter;
    MoogLadderFilter filter;

    bool ladderEnabled = false;

    juce::HeapBlock<char> heapBlock;
    juce::dsp::AudioBlock<float> tempBlock;
};