#pragma once
#include "RawInputBuffer.h"

class WindowsRawInputBuffer final : public IRawInputBuffer
{
    uint8* rawBuffer = nullptr;
    uint32 currentBufferSize;
    int32 inputBlocksNum;
private:
    void clearBuffer();
    // Resizing raw buffer size if needed
    void resize(uint32 newSize);
public:
    ~WindowsRawInputBuffer();
    /* IRawInputBuffer overrides */
    void update() final;
    void processInputs(const ProcessInputsParam& params) const final;
    /* override ends */
};

namespace Input
{
    typedef WindowsRawInputBuffer RawInputBuffer;
}
