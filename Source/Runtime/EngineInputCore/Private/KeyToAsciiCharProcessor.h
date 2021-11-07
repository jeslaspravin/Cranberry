#pragma once

#include "IKeyToCharProcessor.h"


class KeyToAsciiCharProcessor : public IKeyToCharProcessor
{
	struct KeyCharInfo
	{
		uint8 baseChar;
		uint8 shiftedChar = 0;
		AnalogStates::StateKeyType lockStateKey = AnalogStates::None;

		uint8 currentChar = 0;
	};

	std::map<Keys::StateKeyType, KeyCharInfo> keyToCharMap;

public:
	KeyToAsciiCharProcessor();

	/* IKeyToCharProcessor implementations */
	void updateCharacters(class Keys* keyStates, class AnalogStates* analogStates) override;
	Utf32 keyChar(Keys::StateKeyType key) const override;
	/* Overrides ends */
};