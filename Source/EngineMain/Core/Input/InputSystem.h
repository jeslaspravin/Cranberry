#pragma once
#include "Keys.h"

class InputSystem
{
private:
    Keys keys;

public:
    const KeyState* keyState(const Key& key) const;
    bool isKeyPressed(const Key& key) const;
    /* Resets inputs as if it is first frame */
    void resetToStart();
    // Clears all input marking the once that were pressed as released this frame.
    void clearInputs();

    void updateInputStates();
};