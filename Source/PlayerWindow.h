

#pragma once

#include <JuceHeader.h>
#include "DemoUtilities.h"
#include "DSPDemos_Common.h"
#include "PlayerController.h"

class PlayerWindow : public Component
{
public:
    PlayerWindow();
    void resized() override;
    
private:
    AudioFileReaderComponent<PlayerController> fileReaderComponent;
};
