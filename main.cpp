#include <windows.h>
#include <detours.h>
#include <iostream>
#include <string>

#pragma comment(lib, "detours.lib") 


void SetColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

bool SpawnWithDll(const std::wstring& exe)
{
    STARTUPINFOW        si{ sizeof(si) };
    PROCESS_INFORMATION pi{};

    // command line must be mutable buffer
    std::wstring cmd = L"\"" + exe + L"\"";

    BOOL ok = DetourCreateProcessWithDllExW(
        exe.c_str(),           // lpApplicationName 
        &cmd[0],               // lpCommandLine       
        nullptr, nullptr,
        FALSE,
        CREATE_DEFAULT_ERROR_MODE,
        nullptr,               // inherit environment
        nullptr,               // inherit CWD
        &si,
        &pi,
        "C:\\Users\\sample.dll",           // <‑‑ the injected DLL
        nullptr);              // use default CreateProcessW

    if (!ok) {
        SetColor(FOREGROUND_RED);
        std::wcerr << L"[!] DetourCreateProcessWithDllExW failed: "
            << GetLastError() << L"\n";
        return false;
    }

    SetColor(FOREGROUND_GREEN);
    std::wcout << L"[+] Process startet with injected DLL..." << L"\n";
    std::wcout << L"[+] PID:  " << pi.dwProcessId << L"\n";

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

int main()
{
    
    std::wstring exe = L"C:\\Windows\\System32\\dllhost.exe";

    if (!SpawnWithDll(exe))
        return 1;
    SetColor(FOREGROUND_INTENSITY);

    return 0;
}
