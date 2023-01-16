
#pragma once
#include "MoogFilter.h"
#include "MoogSpectrum.h"

//==============================================================================
/** A demo synth sound that's just a basic sine wave.. */
class SineWaveSound : public SynthesiserSound
{
public:
    SineWaveSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override    { return true; }
    bool appliesToChannel (int /*midiChannel*/) override    { return true; }
};

//==============================================================================
/** A simple demo synth voice that just plays a sine wave.. */
class SineWaveVoice   : public SynthesiserVoice
{
public:
    SineWaveVoice() {}

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * MathConstants<double>::twoPi;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0) 
                tailOff = 1.0;
        }
        else
        {
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int /*newValue*/) override {}

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override {}

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (sin (currentAngle) * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote();

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (sin (currentAngle) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

    using SynthesiserVoice::renderNextBlock;

private:
    double currentAngle = 0.0;
    double angleDelta   = 0.0;
    double level        = 0.0;
    double tailOff      = 0.0;
};

//==============================================================================
/** As the name suggest, this class does the actual audio processing. */
class MoogAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    MoogAudioProcessor()
        : AudioProcessor (getBusesProperties()),
          state (*this, nullptr, "state",{ 
                std::make_unique<AudioParameterFloat> (ParameterID { "gain",  1 }, "Gain",         NormalisableRange<float> (0.0f, 1.0f, 0.01), 0.9f),
                std::make_unique<AudioParameterFloat> (ParameterID { "Ia", 1 }, "Input amplitude", NormalisableRange<float> (0.0f, 1.0f, 0.01), 0.1f),
                std::make_unique<AudioParameterFloat>(ParameterID { "r", 1 }, "Resonance amount",  NormalisableRange<float>(0.0f, 4.0f, 0.01), 0.5f),
                std::make_unique<AudioParameterFloat>(ParameterID { "Fc", 1 }, "Cutoff frequency", NormalisableRange<float>(20.0f, 20000.0f, 0.01,0.3), 440.0f),
          })
    {
        // Add a sub-tree to store the state of our UI
        state.state.addChild ({ "uiState", { { "width",  500 }, { "height", 300 } }, {} }, -1, nullptr);

        initialiseSynth();
    }

    ~MoogAudioProcessor() override = default;

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        // Only mono/stereo and input/output must have same layout
        const auto& mainOutput = layouts.getMainOutputChannelSet();
        const auto& mainInput  = layouts.getMainInputChannelSet();

        // input and output layout must either be the same or the input must be disabled altogether
        if (! mainInput.isDisabled() && mainInput != mainOutput)
            return false;

        // only allow stereo and mono
        if (mainOutput.size() > 2)
            return false;

        return true;
    }

    void prepareToPlay (double newSampleRate, int samplesPerBlock) override
    {
        // Use this method as the place to do any pre-playback
        // initialisation that you need..
        synth.setCurrentPlaybackSampleRate (newSampleRate);
        keyboardState.reset();
        reset();

        filter.prepare(newSampleRate, samplesPerBlock, 2, &state);
    }

    void releaseResources() override
    {
        keyboardState.reset();
        filter.reset();
    }

    void reset() override
    {
        filter.reset();
    }

    //==============================================================================
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (! isUsingDoublePrecision());
        process (buffer, midiMessages);
    }

    void processBlock (AudioBuffer<double>& buffer, MidiBuffer& midiMessages) override
    {
        jassert (isUsingDoublePrecision());
        process ((AudioBuffer<float>&)buffer, midiMessages);
    }

    //==============================================================================
    bool hasEditor() const override                                   { return true; }

    AudioProcessorEditor* createEditor() override
    {
        return new MoogAudioProcessorEditor (*this);
    }

    //==============================================================================
    const String getName() const override                             { return "AudioPluginDemo"; }
    bool acceptsMidi() const override                                 { return true; }
    bool producesMidi() const override                                { return true; }
    double getTailLengthSeconds() const override                      { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                                     { return 0; }
    int getCurrentProgram() override                                  { return 0; }
    void setCurrentProgram (int) override                             {}
    const String getProgramName (int) override                        { return "None"; }
    void changeProgramName (int, const String&) override              {}

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override
    {
        if (auto xmlState = state.copyState().createXml())
            copyXmlToBinary (*xmlState, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
            state.replaceState (ValueTree::fromXml (*xmlState));
    }

    //==============================================================================
    void updateTrackProperties (const TrackProperties& properties) override
    {
        {
            const ScopedLock sl (trackPropertiesLock);
            trackProperties = properties;
        }

        MessageManager::callAsync ([this]
        {
            if (auto* editor = dynamic_cast<MoogAudioProcessorEditor*> (getActiveEditor()))
                 editor->updateTrackProperties();
        });
    }

    TrackProperties getTrackProperties() const
    {
        const ScopedLock sl (trackPropertiesLock);
        return trackProperties;
    }

    class SpinLockedPosInfo
    {
    public:
        void set (const AudioPlayHead::PositionInfo& newInfo)
        {
            const juce::SpinLock::ScopedTryLockType lock (mutex);

            if (lock.isLocked())
                info = newInfo;
        }

        AudioPlayHead::PositionInfo get() const noexcept
        {
            const juce::SpinLock::ScopedLockType lock (mutex);
            return info;
        }

    private:
        juce::SpinLock mutex;
        AudioPlayHead::PositionInfo info;
    };


    MidiKeyboardState keyboardState;
    SpinLockedPosInfo lastPosInfo;
    MoogSpectrum spectrum;

    AudioProcessorValueTreeState state;

private:
    //==============================================================================
    /** This is the editor component that our filter will display. */
    class MoogAudioProcessorEditor  : public AudioProcessorEditor,
                                      private Value::Listener
    {
    public:
        MoogAudioProcessorEditor (MoogAudioProcessor& owner)
            : AudioProcessorEditor (owner),
              midiKeyboard         (owner.keyboardState, MidiKeyboardComponent::horizontalKeyboard),
              gainAttachment       (owner.state, "gain",  gainSlider),
              rAttachment          (owner.state, "r",     rSlider),
              FcAttachment(owner.state, "Fc", FcSlider),
              IaAttachment(owner.state, "Ia", IaSlider)
        {
            addAndMakeVisible(owner.spectrum);

            //addSlider(&gainSlider,&gainLabel);
            addSlider(&IaSlider, &IaLabel);
            addSlider(&FcSlider, &FcLabel);
            FcSlider.setTextValueSuffix(" Hz");
            addSlider(&rSlider, &rLabel);


            // add the midi keyboard component..
            addAndMakeVisible (midiKeyboard);

            // add a label that will display the current timecode and status..

            // set resize limits for this plug-in
            setResizeLimits (400, 200, 1024, 700);
            setResizable (true, owner.wrapperType != wrapperType_AudioUnitv3);

            lastUIWidth .referTo (owner.state.state.getChildWithName ("uiState").getPropertyAsValue ("width",  nullptr));
            lastUIHeight.referTo (owner.state.state.getChildWithName ("uiState").getPropertyAsValue ("height", nullptr));

            // set our component's initial size to be the last one that was stored in the filter's settings
            setSize (lastUIWidth.getValue(), lastUIHeight.getValue());

            lastUIWidth. addListener (this);
            lastUIHeight.addListener (this);

            updateTrackProperties();
        }

        ~MoogAudioProcessorEditor() override {}

        //==============================================================================
        void paint (Graphics& g) override
        {
            g.setColour (backgroundColour);
            g.fillAll();
        }

        void resized() override
        {

            auto r = getLocalBounds().reduced (8);

            midiKeyboard.setBounds (r.removeFromBottom (70));

            auto area = r.removeFromTop(250);
            auto sliderArea = area.removeFromRight(200);

            getProcessor().spectrum.setBounds(area);

            sliderArea.removeFromTop (20);
            
            auto row1 = sliderArea.removeFromTop(100);
            //gainSlider.setBounds(row1.removeFromRight(90));
            FcSlider.setBounds(row1.removeFromRight(180));

            sliderArea.removeFromTop(35);

            auto row2 = sliderArea.removeFromTop(100);
            IaSlider.setBounds(row2.removeFromRight(90));
            rSlider.setBounds(row2.removeFromRight(90));

            lastUIWidth  = getWidth();
            lastUIHeight = getHeight();
        }

        void hostMIDIControllerIsAvailable (bool controllerIsAvailable) override
        {
            midiKeyboard.setVisible (! controllerIsAvailable);
        }

        int getControlParameterIndex (Component& control) override
        {
            if (&control == &gainSlider)
                return 0;
            if (&control == &rSlider)
                return 1;

            return -1;
        }

        void updateTrackProperties()
        {
            auto trackColour = getProcessor().getTrackProperties().colour;
            auto& lf = getLookAndFeel();

            backgroundColour = (trackColour == Colour() ? lf.findColour (ResizableWindow::backgroundColourId)
                                                        : trackColour.withAlpha (1.0f).withBrightness (0.266f));
            repaint();
        }

        void addSlider(Slider* slider, Label* label) {
            addAndMakeVisible(*slider);
            slider->setSliderStyle(Slider::Rotary);
            slider->setTextBoxStyle(Slider::TextBoxBelow, false, 70, 20);
            slider->setSize(50,50);
            label->attachToComponent(slider, false);
            label->setFont(Font(14.0f));
        }

    private:
        MidiKeyboardComponent midiKeyboard;

        Label gainLabel{ {}, "Volume" }, 
              rLabel{ {}, "Resonance" }, 
              FcLabel{ {}, "Cutoff frequency" }, 
              IaLabel{ {}, "Input amplitude" };

        Slider gainSlider,rSlider,FcSlider,IaSlider;
        AudioProcessorValueTreeState::SliderAttachment gainAttachment, rAttachment, FcAttachment, IaAttachment;
        Colour backgroundColour;
        Value lastUIWidth, lastUIHeight;

        //==============================================================================
        MoogAudioProcessor& getProcessor() const
        {
            return static_cast<MoogAudioProcessor&> (processor);
        }

        // called when the stored window size changes
        void valueChanged (Value&) override
        {
            setSize (lastUIWidth.getValue(), lastUIHeight.getValue());
        }
    };

    //==============================================================================
    void process (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        auto gainParamValue  = state.getParameter ("gain") ->getValue();

        auto numSamples = buffer.getNumSamples();

        for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
            buffer.clear (i, 0, numSamples);

        keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);
        synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

        dsp::AudioBlock<float> block(buffer);
        filter.process(block);

        spectrum.nextBlock(block);
    }

    MoogFilter filter;
    Synthesiser synth;

    CriticalSection trackPropertiesLock;
    TrackProperties trackProperties;

    void initialiseSynth()
    {
        auto numVoices = 8;

        for (auto i = 0; i < numVoices; ++i)
            synth.addVoice (new SineWaveVoice());

        synth.addSound (new SineWaveSound());
    }

    static BusesProperties getBusesProperties()
    {
        return BusesProperties().withInput  ("Input",  AudioChannelSet::stereo(), true)
                                .withOutput ("Output", AudioChannelSet::stereo(), true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoogAudioProcessor)
};
