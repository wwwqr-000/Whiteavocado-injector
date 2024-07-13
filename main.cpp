#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <memory>//Unique ptr
#include <functional>//For function storage
#include <thread>
#include <fstream>
#include <tlhelp32.h>

//Enumerators
enum FlexDirection { COLUMN, ROW };
//

#include "classes/dimensions.hpp"
#include "classes/frameObj.hpp"
#include "classes/frames/box.hpp"
#include "classes/frames/menuItem.hpp"
#include "classes/frames/menu.hpp"
#include <libloaderapi.h>

using namespace std;

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("WhiteavocadoApp");

//Global variables
HWND hwnd;
std::vector<std::unique_ptr<frameObj>> frameObjects;
//

void updateScreen() {
    InvalidateRect(hwnd, NULL, TRUE);
}

void toggleMenu(std::string menuName) {
    for (auto& obj : frameObjects) {
        menu* menuObj = dynamic_cast<menu*>(obj.get());

        if (menuObj && menuObj->getName() == menuName) {
            menuObj->setVisable(!menuObj->isVisable());
            updateScreen();
            std::cout << "changed visability\n";
        }
    }
}

void getProcessID(std::string& window_name, DWORD &process_ID) {
    GetWindowThreadProcessId(FindWindow(NULL, window_name.c_str()), &process_ID);
}

bool fileExists(std::string& fName) {
    std::ifstream file(fName);
    if (!file.is_open()) {
        return false;
    }
    file.close();
    return true;
}

void nullClick() {}

DWORD WINAPI loadLibraryThread(LPVOID lpParameter) {
    LoadLibraryA((LPCSTR)lpParameter);
    return 0;
}

bool EnableDebugPrivilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        std::cerr << "Failed to open process token." << std::endl;
        return false;
    }

    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        std::cerr << "Failed to lookup privilege value." << std::endl;
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        std::cerr << "Failed to adjust token privileges." << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

//box onclick calls
void inject() {
    EnableDebugPrivilege();
    // Check if DLL exists
    string dllPath = "test.dll";
    string processName = "Whiteavocado-injector.exe";
    if (!fileExists(dllPath)) {
        cout << "DLL could not be found.\n";
        return;
    }

    // Get the process from name
    HANDLE hProcess = NULL;
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        cout << "Failed to create snapshot." << endl;
        return;
    }

    if (!Process32First(hSnapshot, &pe32)) {
        cout << "Failed to get first process." << endl;
        CloseHandle(hSnapshot);
        return;
    }

    do {
        if (std::string(pe32.szExeFile, pe32.szExeFile + strlen(pe32.szExeFile)) == processName) {
            hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
            break;
        }
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    if (hProcess == NULL) {
        cout << "Failed to get process handle, are you sure " << processName << " is active?\n" << endl;
        return;
    }

    // Set debug privilege to enable injection into other processes
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    DWORD dwSize;
    ZeroMemory(&tp, sizeof(tp));
    tp.PrivilegeCount = 1;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
        CloseHandle(hProcess);
        cout << "Failed to open process token." << endl;
        return;
    }
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        CloseHandle(hProcess);
        cout << "Failed to lookup debug privilege." << endl;
        return;
    }
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, &dwSize)) {
        CloseHandle(hToken);
        CloseHandle(hProcess);
        cout << "Failed to adjust token privileges." << endl;
        return;
    }
    CloseHandle(hToken);

    FARPROC lpStartAddress = GetProcAddress(GetModuleHandle("kernel32"), "LoadLibraryA");
    if (lpStartAddress == NULL) {
        cout << "Failed to get address of LoadLibraryA." << endl;
        CloseHandle(hProcess);
        return;
    }

    // Allocate memory in the process
    LPVOID lpAddress = VirtualAllocEx(hProcess, NULL, dllPath.length() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (lpAddress == NULL) {
        cout << "Failed to allocate memory." << endl;
        CloseHandle(hProcess);
        return;
    }

    // Write the DLL path to the allocated memory
    SIZE_T bytesWritten;
    if (!WriteProcessMemory(hProcess, lpAddress, dllPath.c_str(), dllPath.length() + 1, &bytesWritten)) {
        cout << "Failed to write to process memory." << endl;
        VirtualFreeEx(hProcess, lpAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return;
    }

    if (bytesWritten!= dllPath.length() + 1) {
        cout << "Failed to write entire DLL path to process memory." << endl;
        VirtualFreeEx(hProcess, lpAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return;
    }

    // Create a remote thread to execute the LoadLibraryA function
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)lpStartAddress, lpAddress, 0, NULL);
    if (hThread == NULL) {
        cout << "Failed to create remote thread." << endl;
        VirtualFreeEx(hProcess, lpAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return;
    }

    // Wait for the thread to finish
    WaitForSingleObject(hThread, INFINITE);

    // Get the DLL's module handle
    HMODULE hModule;
    GetExitCodeThread(hThread, (LPDWORD)&hModule);
    CloseHandle(hThread);

    // Checkif the module handle is valid
    if (hModule == NULL) {
        DWORD dwError = GetLastError();
        cout << "Failed to get module handle. Error code: " << dwError << endl;
        VirtualFreeEx(hProcess, lpAddress, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return;
    }

    // Clean up
    FreeLibrary(hModule);
    VirtualFreeEx(hProcess, lpAddress, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    cout << "Injected successfully.\n";
    return;
}

void frameObjectSetup() {
    point3 color_green(0, 255, 72);
    std::vector<menuItem> titleItems;
    menuItem title(point2(0, 0), point2((50 * 16), 20), point3(0, 0, 0), 1, "Whiteavocado - Injector", nullClick, false);
    titleItems.emplace_back(title);
    menu titleHolder(point2(0, 0), point2((50 * 16), 20), color_green, titleItems, "title-holder", COLUMN, true);

    std::vector<menuItem> navbarItems;
    menuItem injectItem(point2(1, 23), point2(51, 41), color_green, 1, "inject", inject, false);
    navbarItems.emplace_back(injectItem);

    menu navbarMenu(point2(0, 22), point2((50 * 16), 42), color_green, navbarItems, "navbar", COLUMN, true);

    frameObjects.emplace_back(std::make_unique<menu>(titleHolder));
    frameObjects.emplace_back(std::make_unique<menu>(navbarMenu));
}

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow) {
    //HWND hwnd;             /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                               /* Extended possibilites for variation */
           szClassName,                     /* Classname */
           _T("Whiteavocado Injector"),       /* Title Text */
           WS_OVERLAPPEDWINDOW,             /* default window */
           CW_USEDEFAULT,                   /* Windows decides the position */
           CW_USEDEFAULT,                   /* where the window ends up on the screen */
           (50 * 16),                       /* The programs width */
           (50 * 9),                        /* and height in pixels */
           HWND_DESKTOP,                    /* The window is a child-window to desktop */
           NULL,                            /* No menu */
           hThisInstance,                   /* Program Instance handler */
           NULL                             /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nCmdShow);

    frameObjectSetup();

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}

void drawBox(HDC& hdc, point2 min_, point2 max_, point3 rgb, int width) {
    HPEN hPen;
    HPEN hOldPen;

    //Draw menuItem frame based on box
    hPen = CreatePen(PS_SOLID, width, RGB(rgb.x_i, rgb.y_i, rgb.z_i));
    hOldPen = (HPEN)SelectObject(hdc, hPen);

    MoveToEx(hdc, min_.x_i, min_.y_i, NULL);//Top left -> Top right
    LineTo(hdc, max_.x_i, min_.y_i);

    MoveToEx(hdc, max_.x_i, min_.y_i, NULL);//Top right -> Bottom right
    LineTo(hdc, max_.x_i, max_.y_i);

    MoveToEx(hdc, max_.x_i, max_.y_i, NULL);//Bottom right -> Bottom left
    LineTo(hdc, min_.x_i, max_.y_i);

    MoveToEx(hdc, min_.x_i, max_.y_i, NULL);//Bottom left -> Top left
    LineTo(hdc, min_.x_i, min_.y_i);
    //
}


/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_CREATE: {
            //HWND editCtrl = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 10, 10, 200, 20, hwnd, (HMENU)1, NULL, NULL);
        } break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HPEN hPen;
            HPEN hOldPen;

            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkMode(hdc, TRANSPARENT);

            for (const auto& obj : frameObjects) {
                const menu* menuObj = dynamic_cast<const menu*>(obj.get());

                if (menuObj && menuObj->isVisable()) {
                    drawBox(hdc, menuObj->getMin(), menuObj->getMax(), menuObj->getColor(), menuObj->getLineSize());
                    for (const auto& menItem : menuObj->getItems()) {
                        if (menItem.hasBorder()) {
                            drawBox(hdc, menItem.getMin(), menItem.getMax(), menItem.getColor(), menItem.getLineSize());
                        }
                        RECT textArea;
                        textArea.left = menItem.getMin().x_i;
                        textArea.top = menItem.getMin().y_i;
                        textArea.right = menItem.getMax().x_i;
                        textArea.bottom = menItem.getMax().y_i;
                        DrawText(hdc, menItem.getPlaceholder().c_str(), -1, &textArea, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    }
                }
            }

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            EndPaint(hwnd, &ps);
        } break;
        case WM_LBUTTONDOWN: {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);

            for (const auto& obj : frameObjects) {
                const menu* menuObj = dynamic_cast<const menu*>(obj.get());

                if (menuObj && menuObj->isVisable()) {
                    for (const auto& menItem : menuObj->getItems()) {
                        if (menItem.isClickable() && xPos >= menItem.getMin().x_i && xPos <= menItem.getMax().x_i && yPos >= menItem.getMin().y_i && yPos <= menItem.getMax().y_i) {
                            menItem.click();
                        }
                    }
                }
            }
        } break;
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
