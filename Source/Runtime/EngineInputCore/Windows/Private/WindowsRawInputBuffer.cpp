#include "WindowsRawInputBuffer.h"
#include "Logger/Logger.h"
#include "InputDevice.h"
#include "WindowsCommonHeaders.h"

WindowsRawInputBuffer::~WindowsRawInputBuffer()
{
    clearBuffer();
}

void WindowsRawInputBuffer::processInputs(const ProcessInputsParam& params) const
{
    RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(rawBuffer);

    for (int32 blockIdx = 0; blockIdx < inputBlocksNum; ++blockIdx)
    {
        bool bProcessed = false;
        for (int32 deviceIdx = 0; deviceIdx < params.devicesNum; ++deviceIdx)
        {
            if (params.inputDevices[deviceIdx]->sendInRaw(rawInput))
            {
                bProcessed = true;
                break;
            }
        }

        if (!bProcessed)
        {
            Logger::warn("WindowsRawInputBuffer", "%s: No device found for processing raw input", __func__);
            DefRawInputProc(&rawInput, 1, sizeof(RAWINPUTHEADER));
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
    std::vector<uint8> bufferedRawInput;
    int32 currentBlocksNum = 0, totalBlocksNum = 0;

    while(true)
    {
        const uint32 currRawSize = uint32(bufferedRawInput.size());

        uint32 bufferSize;
        currentBlocksNum = GetRawInputBuffer(nullptr, &bufferSize, sizeof(RAWINPUTHEADER));
        if (currentBlocksNum == -1)
        {
            Logger::error("WindowsRawInputBuffer", "%s : Retrieving input buffer size failed", __func__);
            clearBuffer();
            return;
        }
        // To support proper alignment
        bufferSize *= 8;
        if (bufferSize == 0)
        {
            break;
        }

        bufferedRawInput.resize(currRawSize + bufferSize);
        RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(&bufferedRawInput[currRawSize]);
        currentBlocksNum = GetRawInputBuffer(rawInput, &bufferSize, sizeof(RAWINPUTHEADER));
        totalBlocksNum += currentBlocksNum;
        if (currentBlocksNum == -1)
        {
            Logger::error("WindowsRawInputBuffer", "%s : Reading buffered raw input failed", __func__);
            clearBuffer();
            return;
        }
    }

    resize(uint32(bufferedRawInput.size()));
    inputBlocksNum = totalBlocksNum;
    memcpy(rawBuffer, bufferedRawInput.data(), bufferedRawInput.size());
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

