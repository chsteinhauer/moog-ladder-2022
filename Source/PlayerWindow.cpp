

#include "PlayerWindow.h"


PlayerWindow::PlayerWindow() {
    addAndMakeVisible (fileReaderComponent);
    setSize (750, 525);
}

void PlayerWindow::resized() {
    fileReaderComponent.setBounds (getLocalBounds());
}