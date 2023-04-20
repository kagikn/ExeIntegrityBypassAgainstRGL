#include "pch.h"
#include "Logger.h"
#include <TlHelp32.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>

namespace {
    std::vector<std::string> split(std::string str, std::string separator) {
        if (separator == "") return { str };
        std::vector<std::string> result;
        std::string tstr = str + separator;
        size_t l = tstr.length(), sl = separator.length();
        std::string::size_type pos = 0, prev = 0;

        for (; pos < l && (pos = tstr.find(separator, pos)) != std::string::npos; prev = (pos += sl)) {
            result.emplace_back(tstr, prev, pos - prev);
        }
        return result;
    }
}

uintptr_t FindPattern(std::string patternStr) {
    auto space_separator = std::string(" ");
    std::vector<std::string> byte_str = split(patternStr, space_separator);

    MODULEINFO module_info{};
    const DWORD module_info_size = static_cast<DWORD>(sizeof(MODULEINFO));
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &module_info, module_info_size);

    auto* start_offset = static_cast<uint8_t*>(module_info.lpBaseOfDll);
    const auto size = static_cast<uintptr_t>(module_info.SizeOfImage);

    uintptr_t pos = 0;
    const uintptr_t search_len = byte_str.size();

    for (auto* ret_address = start_offset; ret_address < start_offset + size; ret_address++) {
        if (byte_str[pos] == "??" || byte_str[pos] == "?" ||
            *ret_address == static_cast<uint8_t>(std::strtoul(byte_str[pos].c_str(), nullptr, 16))) {
            if (pos + 1 == byte_str.size())
                return (reinterpret_cast<uintptr_t>(ret_address) - search_len + 1);
            pos++;
        }
        else {
            pos = 0;
        }
    }
    return 0;
}

BOOL SuspendSpecifiedThread(DWORD thread_id, bool suspend)
{
    HANDLE snHandle = nullptr;
    THREADENTRY32 te32 = { 0 };

    BOOL succeeded = false;

    DWORD current_process_id = GetCurrentProcessId();

    snHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snHandle == INVALID_HANDLE_VALUE) return (false);

    te32.dwSize = sizeof(THREADENTRY32);
    if (Thread32First(snHandle, &te32))
    {
        do
        {
            if (te32.th32OwnerProcessID == current_process_id && te32.th32ThreadID == thread_id)
            {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                if (hThread == NULL) {
                    continue;
                } else if (!suspend) {
                    succeeded = (bool)ResumeThread(hThread);
                } else {
                    succeeded = (bool)SuspendThread(hThread);
                }

                CloseHandle(hThread);
            }
        } while (Thread32Next(snHandle, &te32));
    }
    CloseHandle(snHandle);

    return succeeded;
}

Logger logger;
DWORD main_thread_id;
HMODULE this_module;

void PatchExeIntegrityCheck() {
    // Wait for a second, main thread
    SuspendSpecifiedThread(main_thread_id, true);

    // The function where this integrity check is done should be already decrypted when this asi is attached (but encrypted when the exe is not started at all)
    // Should patch before you can see the game window
    // This memory pattern is tested in b2060, b2372, b2699, and b2845
    const std::string pattern_for_exe_integrity_check = "84 C0 75 1B 39 ? 60 01 00 00 0F 85";
    const uintptr_t addr = FindPattern(pattern_for_exe_integrity_check);

    if (addr) {
        // Let's fuck the integrity check by making the exe not listen to the query result against the function that calls
        // CryptAcquireContextA, CryptMsgGetParam, and CryptQueryObject against socialclub.dll
        void* write_addr = reinterpret_cast<void*>(addr + 2);
        const uint8_t instruction_to_write[1] = { 0xEB }; // patch jnz with jmp instruction

        DWORD old_protect = 0;
        VirtualProtect(write_addr, 1u, PAGE_EXECUTE_READWRITE, &old_protect);
        memcpy(write_addr, instruction_to_write, 1);
        VirtualProtect(write_addr, 1u, old_protect, &old_protect);

        if (logger) {
            logger.AddLine(LogType::Info, "Done!");
        }
        SuspendSpecifiedThread(main_thread_id, false);
        FreeLibraryAndExitThread(this_module, 0);
        return;
    }

    if (logger) {
        logger.AddLine(LogType::Error, "Failed to patch the integrity check against exe.");
    }   
    SuspendSpecifiedThread(main_thread_id, false);
    FreeLibraryAndExitThread(this_module, 0);
}

void Main(HMODULE hModule) {
    logger = Logger("ExeIntegrityBypassAgainstRGL.log");

    if (logger) {
        logger.AddLine(LogType::Info, "Started to patch the integrity check against exe...");
    }
 
    this_module = hModule;
    main_thread_id = GetCurrentThreadId();
    auto thread_handle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)PatchExeIntegrityCheck, hModule, NULL, NULL);
    if (thread_handle == nullptr) {
        if (logger) {
            logger.AddLine(LogType::Error, "Thread creation failed, terminating...");
            FreeLibraryAndExitThread(hModule, 0);
        }
    } else {
        CloseHandle(thread_handle);
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Have to patch the assembly with another thread to avoid the "failed to load" error for taking too long time to pass through DllMain
        Main(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

