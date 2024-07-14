#include <windows.h>
#include <iostream>

extern "C" char const* __cdecl getTest() {
    return "test123";
}

extern "C" void __cdecl testMessage() {
    MessageBoxW(NULL, L"This is a test", L"main DLL", MB_OK);
    return;
}

extern "C" void __cdecl beep(const char* type) {
    std::string strType = std::string(type);
    if (strType == "normal") {
        MessageBeep(MB_ICONASTERISK);
    }
    else if (strType == "strong") {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    else if (strType == "crit") {
        MessageBeep(MB_ICONHAND);
    }
    else if (strType == "click") {
        Beep(1000, 10);
    }
}

void keyPress(BYTE b) {
    keybd_event(b, 0, 0, 0);
    keybd_event(b, 0, KEYEVENTF_KEYUP, 0);
}

BYTE intToByte(int in) { return (byte)in; }
BYTE charToVKCode(char in) {
    BYTE A = 0x41;
    if (in >= 'a' && in <= 'z') {
        return A + (in - 'a');
    }
    else if (in >= 'A' && in <= 'Z') {
        return A + (in - 'A');
    }
    return -1;
}

extern "C" void __cdecl key(const char* type_) {
    std::string type = std::string(type_);
    if (type.length() > 1) {//More than 1 char
        if (type[0] == 'F') {//F keys
            BYTE baseFnum = 0x70;
            char FIndexChar = type[0];
            int FIndex = 0;
            if (FIndexChar > '1' && FIndexChar < '9') {
                FIndex = FIndexChar - '0';
            }

            try {
                std::string str = std::to_string(type[1]);
                int FIndex = (std::stoi(str) - 1);
                BYTE num = baseFnum + FIndex;
                keyPress(num);
                return;
            }
            catch (std::exception) {}
        }
        return;
    }

    //Try to create VK code from alphaChar
    int alphaByte = charToVKCode(type[0]);
    if (alphaByte != -1 && alphaByte != 255) {
        keyPress(alphaByte);
        return;
    }
    //

    //Try to create binary VK code from integer 0-9
    try {
        int i = std::stoi(type);
        BYTE vk_code = VK_NUMPAD0 + i;
        keyPress(vk_code);
        return;
    }
    catch (std::exception) {}
    //
    beep("click");
}

extern "C" void __cdecl mouse(const char* btn_) {
    std::string btn = std::string(btn_);
    POINT pt;
    GetCursorPos(&pt);

    if (btn == "left_down") { mouse_event(MOUSEEVENTF_LEFTDOWN, pt.x, pt.y, 0, 0); return; }
    if (btn == "left_up") { mouse_event(MOUSEEVENTF_LEFTUP, pt.x, pt.y, 0, 0); return; }
    if (btn == "right_down") { mouse_event(MOUSEEVENTF_RIGHTDOWN, pt.x, pt.y, 0, 0); return; }
    if (btn == "right_up") { mouse_event(MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0); return; }
    if (btn == "middle_down") { mouse_event(MOUSEEVENTF_MIDDLEDOWN, pt.x, pt.y, 0, 0); return; }
    if (btn == "middle_up") { mouse_event(MOUSEEVENTF_MIDDLEUP, pt.x, pt.y, 0, 0); return; }
}


/*
WINBOOL beepNiv;
switch (type) {
    case BEEP_NORMAL:
        beepNiv = MB_ICONASTERISK;
    break;
    case BEEP_STRONG:
        beepNiv = MB_ICONEXCLAMATION;
    break;
    case BEEP_CRIT:
        beepNiv = MB_ICONHAND;
    break;
    case BEEP_QUESTION:
        beepNiv = MB_ICONQUESTION;
    break;
    default:
        beepNiv = MB_ICONASTERISK;
    break;
}
MessageBeep(beepNiv);


    if (type == "0") { keyPress(0x30); return; }
    if (type == "1") { keyPress(0x31); return; }
    if (type == "2") { keyPress(0x32); return; }
    if (type == "3") { keyPress(0x33); return; }
    if (type == "4") { keyPress(0x34); return; }
    if (type == "5") { keyPress(0x35); return; }
    if (type == "6") { keyPress(0x36); return; }
    if (type == "7") { keyPress(0x37); return; }
    if (type == "8") { keyPress(0x38); return; }
    if (type == "a") { keyPress(0x41); return; }
    if (type == "b") { keyPress(0x42); return; }

*/
