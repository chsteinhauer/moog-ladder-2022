#include <JuceHeader.h>
#include "MoogWindow.h"

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoogAudioProcessor();
}
