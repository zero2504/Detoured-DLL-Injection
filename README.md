# Detoured-DLL-Injection with Microsoft Detours


# What is Detours


"Detours is a software package for re-routing Win32 APIs underneath applications. For almost twenty years, has been licensed by hundreds of ISVs and used by nearly every product team at Microsoft." [1]

Detours is a library for instrumenting arbitrary Win32 functions on Windows‑compatible processors. It intercepts Win32 calls by rewriting the target function’s machine code in memory. The Detours package also ships with utilities that let you attach any DLLs or data segments (so‑called _payloads_) to existing Win32 binaries. 


# Installation

1. Clone the vcpkg package manager:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
```

2. Build vcpkg itself:

```sh
./bootstrap-vcpkg.sh      # .\bootstrap‑vcpkg.bat on Windows
```

3. Let vcpkg integrate with your Visual Studio or CMake setup:

```sh
./vcpkg integrate install
```

4. Install Detours:

```sh
vcpkg install detours
```


## Prepare Visual Studio project

1. Project → Project Properties → C\C++ → Additional Include Directories


![c1b4e54aa98a04f23def7c45ea82891d_MD5](https://github.com/user-attachments/assets/c88b99f1-24e0-4360-b55e-eca14c51230a)



2.  Project → Project Properties → Linker → Additional Library Directories


![11bb98e068a7dc91d958816a9b364fc0_MD5](https://github.com/user-attachments/assets/445bab0d-977d-48ab-bbeb-80ff4a70c52c)


# DetourCreateProcessWithDllEx


"Create a new process and load a DLL into it. Chooses the appropriate 32-bit or 64-bit DLL based on the target process"[2]. 

`DetourCreateProcessWithDllEx` launches a new process in **suspended** mode (`CREATE_SUSPENDED`), rewrites the executable’s in‑memory import table so that your DLL is listed **first**, and then resumes execution. When the process wakes up, the Windows loader brings in that DLL before loading any other imports and finally jumps to the program’s entry point.

- The altered import table points at **export ordinal #1** of your DLL (if that export is missing, the process fails to start)
    
- If the bitness of parent and target processes differ (32‑bit ↔ 64‑bit), Detours briefly spins up a matching `rundll32.exe` helper so it can patch the target’s import table correctly.
    
- Once the DLL is loaded you can undo the patch by calling `DetourRestoreAfterWith`; the information needed for the rollback was copied in advance via `DetourCopyPayloadToProcess`.



## Definition

```c
BOOL DetourCreateProcessWithDllEx(
    _In_opt_    LPCTSTR lpApplicationName,
    _Inout_opt_ LPTSTR lpCommandLine,
    _In_opt_    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_        BOOL bInheritHandles,
    _In_        DWORD dwCreationFlags,
    _In_opt_    LPVOID lpEnvironment,
    _In_opt_    LPCTSTR lpCurrentDirectory,
    _In_        LPSTARTUPINFOW lpStartupInfo,
    _Out_       LPPROCESS_INFORMATION lpProcessInformation,
    _In_        LPCSTR lpDllName,
    _In_opt_    PDETOUR_CREATE_PROCESS_ROUTINEW pfCreateProcessW
);
```

## Parameters

_lpApplicationName_ : Application name as defined for CreateProcess API.

_lpCommandLine_ : Command line as defined for CreateProcess API.

_lpProcessAttributes_ : Process attributes as defined for CreateProcess API.

_lpThreadAttributes_ : Thread attributes as defined for CreateProcess API.

_bInheritHandles_ : Inherit handle flags as defined for CreateProcess API.

_dwCreationFlags_ : Creation flags as defined for CreateProcess API.

_lpEnvironment_ : Process environment variables as defined for CreateProcess API.

_lpCurrentDirectory_ : Process current directory as defined for CreateProcess API.

_lpStartupInfo_ : Process startup information as defined for CreateProcess API.

_lpProcessInformation_ : Process handle information as defined for CreateProcess API.

_lpDllName_ : Pathname of the DLL to be insert into the new process. To support both 32-bit and 64-bit applications, The DLL name should end with "32" if the DLL contains 32-bit code and should end with "64" if the DLL contains 64-bit code. If the target process differs in size from the parent process, Detours will automatically replace "32" with "64" or "64" with "32" in the path name.

_pfCreateProcessW_ : Pointer to program specific replacement for the CreateProcess API, or `NULL` if the standard CreateProcess API should be used to create the new process.



## Return value

Returns `TRUE` if the new process was created; otherwise returns `FALSE`.

## Error codes

See error code returned from CreateProcess.


# Detoured DLL Injection

```c
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
```

# Detoured DLL Injection vs EDRs

 Now We Reach the Most Exciting Part: EDR Showdown
>
> I tested the approach against:
>
> - **Microsoft Defender ATP** (OS: Windows 11 – Bypassed)  
> - **Palo Alto Cortex XDR** (OS: Windows 11 – Bypassed)  
> - **SentinelOne** (OS: Windows 11 – Bypassed)  
> - **Trend Micro Vision One** (OS: Windows 10 – Bypassed)  
>
> Overall, none of the solutions raised an alert or blocked the activity.  
> Interestingly, Defender occasionally misbehaved when `dllhost.exe` spawned `cmd.exe` running `whoami`, but even then it didn’t detect the DLL injection itself.  
> The Defender event timeline showed no further anomalies, and the other EDRs behaved identically.  
> There’s clearly more potential here for further experimentation.  


## Defender ATP:



https://github.com/user-attachments/assets/c1b552ab-52d6-4a90-af94-b5ca9ecbe105

Screenshot:

![Detours_Defender_DLL](https://github.com/user-attachments/assets/444cb30f-beeb-4e01-8642-41e870bc5795)



## Cortex:



https://github.com/user-attachments/assets/860ca270-86e5-411b-9cf0-9d3608077877

Screenshot:


![Cortex_Screen](https://github.com/user-attachments/assets/3fa627ef-c693-45b0-808a-a53dfdfadd77)


## SentinelOne:


https://github.com/user-attachments/assets/62ec5a57-c866-4323-b961-0f3e550e5067

Screenshot:


![SentinelOne_Screen](https://github.com/user-attachments/assets/0834f5b3-ecae-413c-9aee-3007874d12ea)


## Trend Vision One:




https://github.com/user-attachments/assets/823266ea-e560-48c3-b52c-6951cdea733c

Screenshot:


![Trend_Screen](https://github.com/user-attachments/assets/9ed608f2-25b6-46e5-8be2-980c575a9f05)



# References

[1] https://www.microsoft.com/en-us/research/project/detours/


[2] https://github.com/microsoft/detours/wiki/DetourCreateProcessWithDllEx


[3] https://maldevacademy.com/
