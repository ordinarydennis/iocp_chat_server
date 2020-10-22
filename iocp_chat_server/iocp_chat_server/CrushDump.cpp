#include "CrushDump.h"

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

void InitCrashDump()
{
    beginBreakPad();
}