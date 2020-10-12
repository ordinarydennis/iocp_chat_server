#include "CrushDump.h"
#include <Dbghelp.h>

void make_minidump(EXCEPTION_POINTERS* e)
{
    auto hDbgHelp = LoadLibraryA("dbghelp");
    if (hDbgHelp == nullptr)
        return;
    auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if (pMiniDumpWriteDump == nullptr)
        return;

    char name[MAX_PATH];
    {
        auto nameEnd = name + GetModuleFileNameA(GetModuleHandleA(0), name, MAX_PATH);
        SYSTEMTIME t;
        GetSystemTime(&t);
        wsprintfA(nameEnd - strlen(".exe"),
            "_%4d%02d%02d_%02d%02d%02d.dmp",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }

    auto hFile = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = e;
    exceptionInfo.ClientPointers = FALSE;

    auto dumped = pMiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
        e ? &exceptionInfo : nullptr,
        nullptr,
        nullptr);

    CloseHandle(hFile);

    return;
}

LONG CALLBACK unhandled_handler(EXCEPTION_POINTERS* e)
{
    make_minidump(e);
    return EXCEPTION_CONTINUE_SEARCH;
}

#pragma comment(lib, "exception_handler.lib")
#pragma comment(lib, "crash_report_sender.lib")
#pragma comment(lib, "common.lib")
#pragma comment(lib, "crash_generation_client.lib")

#include "client/windows/handler/exception_handler.h"

bool BreakPadCallBack(const wchar_t* dump_path, const wchar_t* minidump_id, void* context,
    EXCEPTION_POINTERS* exinfo, MDRawAssertionInfo* assertion, bool succeeded)
{
    return succeeded;
}

bool beginBreakPad()
{
    google_breakpad::ExceptionHandler* pHandler = new google_breakpad::ExceptionHandler(
        L".",
        0,
        BreakPadCallBack,
        0,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal,
        L"", 0
    );

    return true;
}

void InitCrashDump(UINT32 type = 0)
{
    if (0 == type)
    {
        SetUnhandledExceptionFilter(unhandled_handler);
    }
    else if(1 == type)
    {
        beginBreakPad();
    }
}