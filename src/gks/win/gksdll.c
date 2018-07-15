#include <windows.h>

BOOL APIENTRY
DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    return TRUE;
}

BOOL APIENTRY
DllEntryPoint(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    return DllMain(hInst, reason, reserved);
}