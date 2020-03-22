#pragma once
#include "../Core/String/String.h"

namespace CoreGraphicsTypes
{
    struct EnumTypeInfo
    {
        uint32 value;
        String name;
    };

    namespace ECompareOp
    {
        enum Type
        {
            Never = 0,
            Less = 1,
            Equal = 2,
            EqualOrLess = 3,
            Greater = 4,
            NotEqual = 5,
            EqualOrGreater = 6,
            Always = 7,
        };
    }


    const EnumTypeInfo* getEnumTypeInfo(ECompareOp::Type compareOp);
}
