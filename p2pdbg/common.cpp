#include <Windows.h>
#include "common.h"

VOID WINAPI CentreWindow(HWND hParent, HWND hChild)
{
    if (!hParent)
    {
        hParent = GetDesktopWindow();
    }

    RECT rt = {0};
    GetWindowRect(hParent, &rt);
    RECT crt = {0};
    GetWindowRect(hChild, &crt);
    int iX = 0;
    int iY = 0;
    int icW = crt.right - crt.left;
    int iW = rt.right - rt.left;
    int icH = crt.bottom - crt.top;
    int iH = rt.bottom - rt.top;
    iX = rt.left + (iW - icW) / 2;
    iY = rt.top + (iH - icH) / 2;
    MoveWindow(hChild, iX, iY, icW, icH, TRUE);
}