#pragma once
#include "../Platform/PlatformTypes.h"
#include "../String/String.h"
#include "../Types/Time.h"

#include <map>

// TODO (Jeslas) : change this to proper input system later on
struct Key
{
    // Make/Break code
    uint32 keyCode;
    String keyname;
    uint32 character;
};

struct KeyState
{
    TickRep pressedTick;
    uint8 isPressed : 1;
    uint8 keyWentUp : 1;// will be high the frame the key went up
    uint8 keyWentDown : 1;// will be high the frame the key went down

    KeyState()
        : pressedTick(0)
        , isPressed(0)
        , keyWentUp(0)
        , keyWentDown(0)
    {}
};

class Keys
{
private:
    static std::initializer_list<std::pair<const Key*, KeyState>> KEYSTATES_INITIALIZER;

    std::map<const Key*, KeyState> keyStates;
public:
    const static Key LMB;
    const static Key RMB;
    const static Key MMB;
    const static Key X1MB;
    const static Key X2MB;
    const static Key BACKSPACE;
    const static Key TAB;
    const static Key CAPS;
    const static Key ESC;
    const static Key ENTER;
    const static Key SPACE;
    const static Key LEFT;
    const static Key UP;
    const static Key RIGHT;
    const static Key DOWN;
    const static Key DEL;// Delete
    const static Key ZERO;
    const static Key ONE;
    const static Key TWO;
    const static Key THREE;
    const static Key FOUR;
    const static Key FIVE;
    const static Key SIX;
    const static Key SEVEN;
    const static Key EIGHT;
    const static Key NINE;
    const static Key A;
    const static Key B;
    const static Key C;
    const static Key D;
    const static Key E;
    const static Key F;
    const static Key P;
    const static Key Q;
    const static Key R;
    const static Key S;
    const static Key W;
    const static Key X;
    const static Key Y;
    const static Key Z;
    const static Key LSHIFT;
    const static Key RSHIFT;
    const static Key LCTRL;
    const static Key RCTRL;
    const static Key LALT;
    const static Key RALT;

    Keys();

    const KeyState* queryState(const Key& key) const;
    std::map<const Key*, KeyState>& getKeyStates();
    void resetStates();

    static bool isKeyboardKey(uint32 keyCode);
    static bool isMouseKey(uint32 keyCode);
};

// Analog states like scroll wheel or mouse movements
struct InputAnalogState
{
    float acceleration = 0;
    float currentValue = 0;
    uint8 startedThisFrame = 0;
    uint8 stoppedThisFrame = 0;
};

class AnalogStates
{
public:
    enum EStates
    {
        RelMouseX,
        RelMouseY,
        AbsMouseX,
        AbsMouseY,
        ScrollWheelX,
        ScrollWheelY
    };

private:
    static std::initializer_list<std::pair<AnalogStates::EStates, InputAnalogState>> ANALOGSTATES_INITIALIZER;

    std::map<AnalogStates::EStates, InputAnalogState> analogStates;

public:
    AnalogStates();
    const InputAnalogState* queryState(AnalogStates::EStates analogState) const;
    std::map<AnalogStates::EStates, InputAnalogState>& getAnalogStates();
    void resetStates();
};
