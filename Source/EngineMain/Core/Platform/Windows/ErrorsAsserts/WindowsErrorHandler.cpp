#include "WindowsErrorHandler.h"
#include "../../../Logger/Logger.h"
#include "../../ModuleManager.h"
#include "../../GenericPlatformFunctions.h"

#pragma comment(lib, "dbghelp.lib")

#include <Windows.h>
#include <DbgHelp.h>
#include <sstream>
#include "../../../Engine/GameEngine.h"
#include "../../LFS/PlatformLFS.h"

class SymbolInfo {

    static const int MAX_BUFFER_LEN = 1024;
    union SymType
    {
        IMAGEHLP_SYMBOL64 symbol;
        AChar symBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_BUFFER_LEN];
    };

    SymType symBuff;
    IMAGEHLP_LINE64 line;

public:
	SymbolInfo(HANDLE process, uint64 address, dword offset) : symBuff(), line() {
		line.SizeOfStruct = sizeof(line);
        symBuff.symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symBuff.symbol.MaxNameLength = MAX_BUFFER_LEN;
        uint64 displacement = offset;

        SymGetSymFromAddr64(process, address, &displacement, &symBuff.symbol);
        if (!SymGetLineFromAddr64(process, address, &offset, &line))
        {
            line.FileName = nullptr;
            line.LineNumber = -1;
        }
    }

    String name() { return symBuff.symbol.Name; }
    String undecoratedName() {
        String udName;
        udName.resize(MAX_BUFFER_LEN);
        if (*symBuff.symbol.Name == '\0')
        {
            udName = "no mapping from PC to function name";
        }
        else
        {
            dword nameLen = UnDecorateSymbolName(symBuff.symbol.Name, udName.data(), MAX_BUFFER_LEN, UNDNAME_COMPLETE);
            udName.resize(nameLen);
        }        
        return udName;
    }

    String fileName()
    {
        return line.FileName?line.FileName:"";
    }

    dword lineNumber()
    {
        return line.LineNumber;
    }
};

void WindowsUnexpectedErrorHandler::registerFilter()
{

    previousFilter = SetUnhandledExceptionFilter(handlerFilter);
}

void WindowsUnexpectedErrorHandler::dumpCallStack()
{
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);
    dumpStack(&context);
}

void WindowsUnexpectedErrorHandler::unregisterFilter()
{
    SetUnhandledExceptionFilter(previousFilter);
}

void WindowsUnexpectedErrorHandler::dumpStack(struct _CONTEXT* context)
{
    HANDLE processHandle = GetCurrentProcess();
    HANDLE threadHandle = GetCurrentThread();
    dword symOffset = 0;

    if (!SymInitialize(processHandle, NULL, TRUE))
    {
        Logger::error("WindowsUnexpectedErrorHandler", "%s() : Failed loading symbols for initializing stack trace symbols", __func__);
        return;
    }
    dword symOptions = SymGetOptions();
    symOptions |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
    SymSetOptions(symOptions);

    auto modulesDataPairs = ModuleManager::get()->getAllModuleData();

    IMAGE_NT_HEADERS* imageHeader = ImageNtHeader(modulesDataPairs[0].second.basePtr);
    dword imageType = imageHeader->FileHeader.Machine;

#ifdef _M_X64
    STACKFRAME64 frame;
    frame.AddrPC.Offset = context->Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
#else
    STACKFRAME64 frame;
    frame.AddrPC.Offset = context->Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context->Esp;
    frame.AddrStack.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context->Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
#endif

    std::stringstream stackTrace;
    do 
    {
        if (frame.AddrPC.Offset != 0)
        {
            uint64 moduleBase = SymGetModuleBase64(processHandle, frame.AddrPC.Offset);
            SymbolInfo symInfo = SymbolInfo(processHandle, frame.AddrPC.Offset, symOffset);
            String moduleName;
            for (auto modulePair : modulesDataPairs)
            {
                if (moduleBase == (uint64)modulePair.second.basePtr)
                {
                    moduleName = modulePair.second.name;
                    break;
                }
            }
            String fileName = PlatformFile(symInfo.fileName()).getFileName();
            stackTrace << moduleName.getChar() << " [0x" << std::hex << frame.AddrPC.Offset << std::dec <<"] : " 
                << symInfo.undecoratedName() << "(" << symInfo.name() << ") : ("
                << fileName.getChar() << "):" << symInfo.lineNumber();
        }
        else
        {
            stackTrace << "No symbols found";
        }

        if (!StackWalk64(imageType, processHandle, threadHandle, &frame, context, NULL, SymFunctionTableAccess64,
            SymGetModuleBase64, NULL) || frame.AddrReturn.Offset == 0)
        {
            break;
        }
        stackTrace << "\r\n";
    } while (true);
    SymCleanup(processHandle);
    
    Logger::error("WindowsUnexpectedErrorHandler", "Error call trace : \r\n%s", stackTrace.str().c_str());

    if (gEngine)
    {
        // TODO(JESLAS): Check if it is waiting for exiting? and change if required 
        gEngine->requestExit();
    }
}

String exceptionCodeMessage(dword ExpCode)
{
    String retVal;
    switch (ExpCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        retVal = "Access violation";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        retVal = "Misaligned data";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        retVal = "Array access out of bound";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        retVal = "Too small floating point value";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        retVal = "Float divide by zero";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        retVal = "Decimal point representation not valid";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        retVal = "Invalid floating point operation";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        retVal = "Float overflow";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        retVal = "Floating point operation lead to stack overflow";
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        retVal = "Exponent of float is less than minimum of this standard";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        retVal = "Integer divide by zero";
        break;
    case EXCEPTION_INT_OVERFLOW:
        retVal = "Integer overflow";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        retVal = "Invalid instruction for machine";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        retVal = "Page error";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        retVal = "Invalid instruction";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        retVal = "Non continuable exception";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        retVal = "Stack overflow";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        retVal = "Fatal exception occurred";
        break;
    case EXCEPTION_INVALID_HANDLE:
        retVal = "Invalid handle";
        break;
    default:
        retVal = "Generic exception has occurred";
        break;
    }
    return retVal;
}

long WindowsUnexpectedErrorHandler::handlerFilter(struct _EXCEPTION_POINTERS* exp)
{
    PEXCEPTION_RECORD pExceptionRecord = exp->ExceptionRecord;

    std::stringstream errorStream;
    while (pExceptionRecord != NULL)
    {
        errorStream << exceptionCodeMessage(pExceptionRecord->ExceptionCode) << " [" << std::hex <<
            pExceptionRecord->ExceptionAddress << "]";
        pExceptionRecord = pExceptionRecord->ExceptionRecord;
    }
    AChar* errorMsg;
    DWORD dw = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPSTR)&errorMsg,0, NULL);
    
    Logger::error("WindowsUnexpectedErrorHandler", "Application encountered an error! Error : %s%s",errorMsg,errorStream.str().c_str());
    LocalFree(errorMsg);

    getHandler()->dumpStack(exp->ContextRecord);
    return EXCEPTION_CONTINUE_SEARCH;
}
