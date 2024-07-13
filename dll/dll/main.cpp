#include <windows.h>
#include <dbghelp.h>
#include <iostream>

#pragma comment(lib, "Dbghelp.lib")

// Error handling macro
#define CHECK_ERROR(cond, msg) if (!(cond)) { std::cerr << msg << std::endl; return FALSE; }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        // Initialization code for the DLL

        // Get the base address of the module
        PBYTE baseAddress = reinterpret_cast<PBYTE>(hModule);

        // Get the DOS header
        PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(baseAddress);
        CHECK_ERROR(dosHeader->e_magic == IMAGE_DOS_SIGNATURE, "Invalid DOS signature");

        // Get the NT headers
        PIMAGE_NT_HEADERS ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(baseAddress + dosHeader->e_lfanew);
        CHECK_ERROR(ntHeaders->Signature == IMAGE_NT_SIGNATURE, "Invalid NT signature");

        // Example RVA (replace with a valid RVA from your context)
        DWORD rva = 0x1000; // Example RVA, change to a valid one

        // Convert RVA to VA
        PVOID va = ImageRvaToVa(ntHeaders, baseAddress, rva, nullptr);
        CHECK_ERROR(va != nullptr, "ImageRvaToVa failed");

        // Read the value at the VA
        try {
            int value = *reinterpret_cast<int*>(va);
            std::cout << "Value: " << value << std::endl;
        }
        catch (...) {
            std::cerr << "Failed to read value" << std::endl;
            return FALSE;
        }

        MessageBoxA(NULL, "DLL Loaded: Process Attach", "DLL Message", MB_OK | MB_ICONINFORMATION);

        break;
    }
    case DLL_PROCESS_DETACH:
        // Cleanup code for the DLL
        break;
    }
    return TRUE;
}
