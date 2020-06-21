#pragma once
#include "../Platform/PlatformTypes.h"
#include "../String/String.h"
#include "../Types/Time.h"

#include <map>

// TODO (Jeslas) : change this to proper input system later on
struct Key
{
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
    uint8 rawKeyStates[256];

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

    void pollInputs();
    // Resets and init with values so that input will be zero in any time relative values(eg, deltas will be zero,clicked and release this frame will be 0)
    void resetInit();
    // Clears all input marking the once that were pressed as released this frame.
    void clear();
    const KeyState* queryKeyState(const Key& key) const;
};