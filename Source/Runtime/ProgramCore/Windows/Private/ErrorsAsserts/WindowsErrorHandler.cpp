/*!
 * \file WindowsErrorHandler.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WindowsErrorHandler.h"
#include "Logger/Logger.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "WindowsCommonHeaders.h"

#pragma comment(lib, "dbghelp.lib")

#include <DbgHelp.h>
#include <sstream>

class SymbolInfo
{
public:
    constexpr static const int32 MAX_BUFFER_LEN = 1024;
    constexpr static const uint32 INVALID_LINE_NUM = ~(0u);

private:
    union SymType
    {
        IMAGEHLP_SYMBOL64 symbol;
        AChar symBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_BUFFER_LEN];
    };

    SymType symBuff;
    IMAGEHLP_LINE64 line;

public:
    SymbolInfo(HANDLE process, uint64 address, dword offset)
        : symBuff()
        , line()
    {
        line.SizeOfStruct = sizeof(line);
        symBuff.symbol.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
        symBuff.symbol.MaxNameLength = MAX_BUFFER_LEN;
        uint64 displacement = offset;

        ::SymGetSymFromAddr64(process, address, &displacement, &symBuff.symbol);
        if (!SymGetLineFromAddr64(process, address, &offset, &line))
        {
            line.FileName = nullptr;
            line.LineNumber = INVALID_LINE_NUM;
        }
    }

    String name() { return UTF8_TO_TCHAR(symBuff.symbol.Name); }
    String undecoratedName()
    {
        String udName;
        if (*symBuff.symbol.Name == '\0')
        {
            udName = TCHAR("no mapping from PC to function name");
        }
        else
        {
            std::string undecName;
            undecName.resize(MAX_BUFFER_LEN, '\0');
            dword nameLen = ::UnDecorateSymbolName(symBuff.symbol.Name, undecName.data(), MAX_BUFFER_LEN, UNDNAME_COMPLETE);
            undecName.resize(nameLen);

            udName = UTF8_TO_TCHAR(undecName.c_str());
        }
        return udName;
    }

    String fileName() { return line.FileName ? UTF8_TO_TCHAR(line.FileName) : TCHAR(""); }

    dword lineNumber() { return line.LineNumber; }
};

void WindowsUnexpectedErrorHandler::registerPlatformFilters()
{
    prevExpFilter = ::SetUnhandledExceptionFilter(unhandledExceptFilter);
    vecExpHandlerHandle = ::AddVectoredExceptionHandler(0, vectoredExceptHandler);
}
void WindowsUnexpectedErrorHandler::unregisterPlatformFilters()
{
    ::SetUnhandledExceptionFilter(prevExpFilter);
    ::RemoveVectoredExceptionHandler(vecExpHandlerHandle);
    prevExpFilter = nullptr;
    vecExpHandlerHandle = nullptr;
}

void WindowsUnexpectedErrorHandler::debugBreak() const
{
    if (PlatformFunctions::hasAttachedDebugger())
    {
        ::DebugBreak();
    }
}

void WindowsUnexpectedErrorHandler::dumpCallStack(bool bShouldCrashApp) const
{
    if (_CONTEXT *exceptionCntxt = getCurrentExceptionCntxt())
    {
        LOG_ERROR("WindowsUnexpectedErrorHandler", "Exception call trace -->");
        dumpStack(exceptionCntxt, false);
    }

    LOG_ERROR("WindowsUnexpectedErrorHandler", "Current call trace -->");
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;
    ::RtlCaptureContext(&context);
    dumpStack(&context, bShouldCrashApp);
}

#if defined(_MSC_VER) && _MSC_VER >= 1900
extern "C" void **__cdecl __current_exception_context();
#endif
_CONTEXT *WindowsUnexpectedErrorHandler::getCurrentExceptionCntxt() const
{
    _CONTEXT **pctx = NULL;
#if defined(_MSC_VER) && _MSC_VER >= 1900
    pctx = (_CONTEXT **)__current_exception_context();
#endif
    return pctx ? *pctx : NULL;
}

void WindowsUnexpectedErrorHandler::dumpStack(_CONTEXT *context, bool bCloseApp) const
{
    HANDLE processHandle = ::GetCurrentProcess();
    HANDLE threadHandle = ::GetCurrentThread();
    dword symOffset = 0;

    dword symOptions = ::SymGetOptions();
    symOptions |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
    ::SymSetOptions(symOptions);

    if (!::SymInitialize(processHandle, NULL, TRUE))
    {
        // Try cleaning once
        ::SymCleanup(processHandle);
        if (!::SymInitialize(processHandle, NULL, TRUE))
        {
            LOG_ERROR("WindowsUnexpectedErrorHandler", "Failed loading symbols for initializing stack trace symbols");
            Logger::flushStream();
            return;
        }
    }

    // We do not want to write all debug logs when getting all modules
    Logger::pushMuteSeverities(Logger::Debug);
    std::vector<std::pair<LibHandle, LibraryData>> modulesDataPairs = ModuleManager::get()->getAllModuleData();
    Logger::popMuteSeverities();

    IMAGE_NT_HEADERS *imageHeader = ::ImageNtHeader(modulesDataPairs[0].second.basePtr);
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

    SizeT longestLine = 0;
    StringStream stackTrace;
    StringStream stackTraceForDebugger;
    do
    {
        if (frame.AddrPC.Offset != 0)
        {
            uint64 moduleBase = ::SymGetModuleBase64(processHandle, frame.AddrPC.Offset);
            SymbolInfo symInfo = SymbolInfo(processHandle, frame.AddrPC.Offset, symOffset);
            String moduleName;
            for (const std::pair<const LibHandle, LibraryData> &modulePair : modulesDataPairs)
            {
                if (moduleBase == (uint64)modulePair.second.basePtr)
                {
                    moduleName = PathFunctions::fileOrDirectoryName(modulePair.second.imgPath);
                    break;
                }
            }

            /**
             * {0} - Module name
             * {1} - Symbol name
             * {2} - File path
             * {3} - Program counter(Instruction address)
             * {4} - Line number
             */

            /* For logger */
            const TChar *fmtStr = symInfo.lineNumber() == SymbolInfo::INVALID_LINE_NUM ? TCHAR("  {0}!{1} ({2}) ({3:#018x})")
                                                                                       : TCHAR("  {0}!{1} ({2}, {4}) ({3:#018x})");
            String lineStr
                = StringFormat::vFormat(fmtStr, moduleName, symInfo.name(), symInfo.fileName(), frame.AddrPC.Offset, symInfo.lineNumber());
            stackTrace << lineStr;
            longestLine = longestLine < lineStr.length() ? lineStr.length() : longestLine;

            /* For debugger */
            fmtStr = symInfo.lineNumber() == SymbolInfo::INVALID_LINE_NUM ? TCHAR("{2}(0, 0): {1}") : TCHAR("{2}({4}, 0): {1}");
            stackTraceForDebugger << StringFormat::vFormat(
                fmtStr, moduleName, symInfo.name(), symInfo.fileName(), frame.AddrPC.Offset, symInfo.lineNumber()
            );
        }
        else
        {
            stackTrace << TCHAR("No symbols found");
        }

        bool bSuccess = ::StackWalk64(
            imageType, processHandle, threadHandle, &frame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr
        );
        if (!bSuccess || frame.AddrReturn.Offset == 0)
        {
            break;
        }
        stackTrace << TCHAR("\n");
        stackTraceForDebugger << TCHAR("\n");
    }
    while (true);
    ::SymCleanup(processHandle);

    const String lineSep{ longestLine, '=' };
    LOG_ERROR("WindowsUnexpectedErrorHandler", "\n{0}\nCall trace : \n{0}\n{1}\n{0}", lineSep, stackTrace.str().c_str());
    if (PlatformFunctions::hasAttachedDebugger())
    {
        PlatformFunctions::outputToDebugger(lineSep.getChar());
        PlatformFunctions::outputToDebugger(LINE_FEED_TCHAR);
        PlatformFunctions::outputToDebugger(stackTraceForDebugger.str().c_str());
        PlatformFunctions::outputToDebugger(LINE_FEED_TCHAR);
        PlatformFunctions::outputToDebugger(lineSep.getChar());
        PlatformFunctions::outputToDebugger(LINE_FEED_TCHAR);
    }
    else
    {
        LOG_ERROR("WindowsUnexpectedErrorHandler", "\n{0}\nFor debugger : \n{0}\n{1}\n{0}", lineSep, stackTraceForDebugger.str().c_str());
    }

    if (bCloseApp)
    {
        CALL_ONCE(crashApplication);
    }
    else
    {
        Logger::flushStream();
    }
}

String exceptionCodeMessage(dword ExpCode)
{
    String retVal;
    switch (ExpCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        retVal = TCHAR("Access violation");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        retVal = TCHAR("Misaligned data");
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        retVal = TCHAR("Array access out of bound");
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        retVal = TCHAR("Too small floating point value");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        retVal = TCHAR("Float divide by zero");
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        retVal = TCHAR("Decimal point representation not valid");
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        retVal = TCHAR("Invalid floating point operation");
        break;
    case EXCEPTION_FLT_OVERFLOW:
        retVal = TCHAR("Float overflow");
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        retVal = TCHAR("Floating point operation lead to stack overflow");
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        retVal = TCHAR("Exponent of float is less than minimum of this standard");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        retVal = TCHAR("Integer divide by zero");
        break;
    case EXCEPTION_INT_OVERFLOW:
        retVal = TCHAR("Integer overflow");
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        retVal = TCHAR("Invalid instruction for machine");
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        retVal = TCHAR("Page error");
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        retVal = TCHAR("Invalid instruction");
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        retVal = TCHAR("Non continuable exception");
        break;
    case EXCEPTION_STACK_OVERFLOW:
        retVal = TCHAR("Stack overflow");
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        retVal = TCHAR("Fatal exception occurred");
        break;
    case EXCEPTION_INVALID_HANDLE:
        retVal = TCHAR("Invalid handle");
        break;
    default:
        retVal = TCHAR("Generic exception has occurred");
        break;
    }
    return retVal;
}

long WindowsUnexpectedErrorHandler::unhandledExceptFilter(struct _EXCEPTION_POINTERS *exp) noexcept
{
    AChar *errorMsg;
    DWORD dw = GetLastError();
    ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errorMsg, 0, NULL
    );

    PEXCEPTION_RECORD pExceptionRecord = exp->ExceptionRecord;
    StringStream errorStream;
    while (pExceptionRecord != NULL)
    {
        errorStream << exceptionCodeMessage(pExceptionRecord->ExceptionCode)
                    << TCHAR(" [0x") << std::hex << pExceptionRecord->ExceptionAddress << TCHAR("]");
        pExceptionRecord = pExceptionRecord->ExceptionRecord;
    }

    LOG_ERROR("WindowsUnexpectedErrorHandler", "Application encountered an error! Error : {}{}", errorMsg, errorStream.str().c_str());
    ::LocalFree(errorMsg);

    getHandler()->unregisterFilter();
    getHandler()->dumpStack(exp->ContextRecord, true);
    return EXCEPTION_CONTINUE_SEARCH;
}

long WindowsUnexpectedErrorHandler::vectoredExceptHandler(_EXCEPTION_POINTERS *exp) noexcept
{
    bool bHandleException = false;
    PEXCEPTION_RECORD pExceptionRecord = exp->ExceptionRecord;
    while (pExceptionRecord != NULL)
    {
        switch (exp->ExceptionRecord->ExceptionCode)
        {
        case DBG_PRINTEXCEPTION_WIDE_C:
        case DBG_PRINTEXCEPTION_C:
            break;
        default:
            bHandleException = true;
            break;
        }
        pExceptionRecord = pExceptionRecord->ExceptionRecord;
    }

    if (!bHandleException)
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return unhandledExceptFilter(exp);
}