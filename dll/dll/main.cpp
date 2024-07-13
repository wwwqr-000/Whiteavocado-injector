#include <Windows.h>

BOOL APIENTRY DllMain(HANDLE hModule, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ShellExecute(NULL, "open", "https://www.google.com", NULL, NULL, SW_SHOW);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
