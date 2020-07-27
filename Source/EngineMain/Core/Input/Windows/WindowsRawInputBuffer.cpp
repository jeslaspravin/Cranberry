#include "WindowsRawInputBuffer.h"
#include "../../Logger/Logger.h"
#include "../InputDevice.h"
#include "../../Platform/Windows/WindowsCommonHeaders.h"


WindowsRawInputBuffer::~WindowsRawInputBuffer()
{
    clearBuffer();
}

void WindowsRawInputBuffer::processInputs(const ProcessInputsParam& params) const
{
    RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(rawBuffer);

    for (int32 blockIdx = 0; blockIdx < inputBlocksNum; ++blockIdx)
    {
        for (int32 deviceIdx = 0; deviceIdx < params.devicesNum; ++deviceIdx)
        {
            if (params.inputDevices[deviceIdx]->sendInRaw(rawInput))
            {
                break;
            }
        }
        using QWORD = uint64;
        rawInput = NEXTRAWINPUTBLOCK(rawInput);
    }

    for (int32 deviceIdx = 0; deviceIdx < params.devicesNum; ++deviceIdx)
    {
        params.inputDevices[deviceIdx]->pullProcessedInputs(params.keyStates, params.analogStates);
    }
}

void WindowsRawInputBuffer::update()
{
    uint32 bufferSize;
    inputBlocksNum = GetRawInputBuffer(nullptr, &bufferSize, sizeof(RAWINPUTHEADER));
    if (inputBlocksNum == -1)
    {
        Logger::error("WindowsRawInputBuffer", "%s : Retrieving input buffer size failed", __func__);
        clearBuffer();
        return;
    }
    bufferSize *= 8;
    resize(bufferSize);
    RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(rawBuffer);
    inputBlocksNum = GetRawInputBuffer(rawInput, &bufferSize, sizeof(RAWINPUTHEADER));
    if (inputBlocksNum == -1)
    {
        Logger::error("WindowsRawInputBuffer", "%s : Reading buffered raw input failed", __func__);
        clearBuffer();
    }
}

void WindowsRawInputBuffer::clearBuffer()
{
    if (rawBuffer != nullptr)
    {
        delete[] rawBuffer;
        rawBuffer = nullptr;
    }
    currentBufferSize = 0;
    inputBlocksNum = 0;
}

void WindowsRawInputBuffer::resize(uint32 newSize)
{
    if (currentBufferSize < newSize)
    {
        clearBuffer();
        currentBufferSize = newSize;
        rawBuffer = new uint8[currentBufferSize];
    }
}

