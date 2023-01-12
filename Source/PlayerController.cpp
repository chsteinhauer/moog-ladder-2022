
#include "PlayerController.h"



void PlayerController::prepare (const ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    tempBlock = juce::dsp::AudioBlock<float>(heapBlock, spec.numChannels, spec.maximumBlockSize);

    os.prepare(spec);

    limiter.setThreshold(-10);
    limiter.setRelease(100);
    limiter.prepare(spec);

    filter.setParameters(&Fc, &r, &Ia);
    filter.prepare(sampleRate*2, spec.maximumBlockSize*2,spec.numChannels);
}

void PlayerController::process (const ProcessContextReplacing<float>& context)
{
    if (!ladderEnabled) return;

    tempBlock.copyFrom(context.getOutputBlock());

    auto block = os.processUp(tempBlock);
    filter.process(block);
    os.processDown(tempBlock);
    

    context.getOutputBlock().copyFrom(tempBlock);
    limiter.process(context);
}

void PlayerController::reset()
{
    os.reset();
    filter.reset();
    limiter.reset();
}

void PlayerController::updateParameters()
{
    if (sampleRate != 0.0)
    {
        auto cutoff = static_cast<float> (Fc.getCurrentValue());
        auto qVal   = static_cast<float> (r.getCurrentValue());

        switch (type.getCurrentSelectedID())
        {
            case 1:     ladderEnabled = false; break;
            case 2:     ladderEnabled = true; break;
            default:    break;
        }
    }
}


void PlayerController::LadderFilterOversampling::prepare(const ProcessSpec& spec) {
    oversampling.reset(new dsp::Oversampling<float>(spec.numChannels, 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false));

    oversampling -> reset();
    oversampling -> initProcessing(spec.maximumBlockSize);
}

dsp::AudioBlock<float> PlayerController::LadderFilterOversampling::processUp(dsp::AudioBlock<float> block) {
    return oversampling->processSamplesUp(block);
}

void PlayerController::LadderFilterOversampling::processDown(dsp::AudioBlock<float> block) {
    oversampling->processSamplesDown(block);
}
void PlayerController::LadderFilterOversampling::reset() {
    oversampling->reset();
}
