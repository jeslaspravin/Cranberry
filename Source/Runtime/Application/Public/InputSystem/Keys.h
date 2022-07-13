/*!
 * \file Keys.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "String/String.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"
#include "Types/Time.h"

#include <map>

template <typename KeyType, typename ValueType>
class InputStateIterator
{
private:
    using Iterator = typename std::initializer_list<std::pair<KeyType, ValueType>>::const_iterator;

    Iterator iterator;

public:
    /* Iterator traits skipped difference_type as it does not makes sense */
    using value_type = KeyType;
    using reference = const KeyType &;
    using pointer = const KeyType *;
    using iterator_category = std::forward_iterator_tag;

    InputStateIterator() = default;
    InputStateIterator(Iterator itr)
        : iterator(itr)
    {}

    pointer operator->() const { return &iterator->first; }

    reference operator*() const { return iterator->first; }

    bool operator!=(const InputStateIterator &other) const { return **this != *other; }

    InputStateIterator &operator++()
    {
        ++iterator;
        return *this;
    }

    InputStateIterator operator++(int)
    {
        InputStateIterator retVal(iterator);
        ++iterator;
        return retVal;
    }
};

template <typename InputType>
struct InputStateRange
{
    using IteratorType = InputStateIterator<typename InputType::StateKeyType, typename InputType::StateInfoType>;
    IteratorType begin() const { return IteratorType(InputType::STATES_INITIALIZER.begin()); }

    IteratorType end() const { return IteratorType(InputType::STATES_INITIALIZER.end()); }
};

struct APPLICATION_EXPORT Key
{
    // Make/Break code
    uint32 keyCode;
    String keyname;
};

struct APPLICATION_EXPORT KeyState
{
    TickRep pressedTick = -1;
    uint8 isPressed : 1 = 0;
    uint8 keyWentUp : 1 = 0;   // will be high the frame the key went up
    uint8 keyWentDown : 1 = 0; // will be high the frame the key went down

    KeyState()
        : pressedTick(-1)
        , isPressed(0)
        , keyWentUp(0)
        , keyWentDown(0)
    {}
};

class APPLICATION_EXPORT Keys
{
public:
    using StateKeyType = const Key *;
    using StateInfoType = KeyState;
    using Range = InputStateRange<Keys>;

private:
    friend Range;
    static std::initializer_list<std::pair<StateKeyType, StateInfoType>> STATES_INITIALIZER;

    std::map<StateKeyType, StateInfoType> keyStates;

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
    const static Key PAGEUP;
    const static Key PAGEDOWN;
    const static Key END;
    const static Key HOME;
    const static Key LEFT;
    const static Key UP;
    const static Key RIGHT;
    const static Key DOWN;
    const static Key INS;
    const static Key DEL; // Delete
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
    const static Key G;
    const static Key H;
    const static Key I;
    const static Key J;
    const static Key K;
    const static Key L;
    const static Key M;
    const static Key N;
    const static Key O;
    const static Key P;
    const static Key Q;
    const static Key R;
    const static Key S;
    const static Key T;
    const static Key U;
    const static Key V;
    const static Key W;
    const static Key X;
    const static Key Y;
    const static Key Z;
    const static Key NUM0;
    const static Key NUM1;
    const static Key NUM2;
    const static Key NUM3;
    const static Key NUM4;
    const static Key NUM5;
    const static Key NUM6;
    const static Key NUM7;
    const static Key NUM8;
    const static Key NUM9;
    const static Key ASTERICK;
    const static Key PLUS;
    const static Key NUMMINUS;
    const static Key NUMFULLSTOP;
    const static Key NUMFWDSLASH;
    const static Key F1;
    const static Key F2;
    const static Key F3;
    const static Key F4;
    const static Key F5;
    const static Key F6;
    const static Key F7;
    const static Key F8;
    const static Key F9;
    const static Key F10;
    const static Key F11;
    const static Key F12;
    const static Key LWIN;
    const static Key RWIN;
    const static Key MENU;
    const static Key F16;
    const static Key F17;
    const static Key F18;
    const static Key F19;
    const static Key F20;
    const static Key F21;
    const static Key F22;
    const static Key F23;
    const static Key F24;
    const static Key NUMLOCK;
    const static Key SCRLLOCK;
    const static Key PAUSE;
    const static Key LSHIFT;
    const static Key RSHIFT;
    const static Key LCTRL;
    const static Key RCTRL;
    const static Key LALT;
    const static Key RALT;
    const static Key SEMICOLON;
    const static Key COMMA;
    const static Key FULLSTOP;
    const static Key FWDSLASH;
    const static Key MINUS;
    const static Key BACKTICK;
    const static Key OPENSQR;
    const static Key CLOSESQR;
    const static Key BACKSLASH;
    const static Key APOSTROPHE;
    const static Key PA1;
    const static Key CLR;
    const static Key LEFTBACKSLASH;
    const static Key NUMENTER;
    const static Key EQUAL;
    const static Key FWDDEL;

    Keys();

    const StateInfoType *queryState(const Key &key) const;
    std::map<StateKeyType, StateInfoType> &getKeyStates();
    void resetStates();

    static bool isKeyboardKey(uint32 keyCode);
    static bool isMouseKey(uint32 keyCode);
};

// Analog states like scroll wheel or mouse movements
struct APPLICATION_EXPORT InputAnalogState
{
    float acceleration = 0;
    float currentValue = 0;
    uint8 startedThisFrame = 0;
    uint8 stoppedThisFrame = 0;
};

class APPLICATION_EXPORT AnalogStates
{
public:
    enum EStates
    {
        None,
        RelMouseX,
        RelMouseY,
        ScrollWheelX,
        ScrollWheelY,
        // Add absolute values below this
        AbsMouseX,
        AbsMouseY,
        CapsLock,
        NumLock,
        ScrollLock,
        AbsValsStart = AbsMouseX,
        AbsValsEnd = ScrollLock
    };

    using StateKeyType = EStates;
    using StateInfoType = InputAnalogState;
    using Range = InputStateRange<AnalogStates>;

private:
    friend Range;
    static std::initializer_list<std::pair<StateKeyType, StateInfoType>> STATES_INITIALIZER;

    std::map<StateKeyType, StateInfoType> analogStates;

public:
    AnalogStates();
    FORCE_INLINE static bool isAbsoluteValue(AnalogStates::EStates analogSate)
    {
        return analogSate >= EStates::AbsValsStart && analogSate <= EStates::AbsValsEnd;
    }
    const StateInfoType *queryState(AnalogStates::EStates analogState) const;
    std::map<StateKeyType, StateInfoType> &getAnalogStates();
    void resetStates();
};
