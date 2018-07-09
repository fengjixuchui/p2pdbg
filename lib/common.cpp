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

static BOOL _WriteFileBuffer(HANDLE file, const char *buffer, size_t length)
{
    DWORD write = 0;
    DWORD ms = 0;
    while(write < length)
    {
        ms = 0;
        if (!WriteFile(file, buffer + write, length - write, &ms, NULL))
        {
            return FALSE;
        }
        write += ms;
    }
    return TRUE;
}

//从PE的资源中释放文件
BOOL ReleaseResource(LPCWSTR wszDstPath, DWORD id, LPCWSTR wszType)
{
    HRSRC src = NULL;
    HGLOBAL hg = NULL;
    HANDLE file = INVALID_HANDLE_VALUE;
    BOOL state = FALSE;
    do 
    {
        src = FindResourceW(NULL, MAKEINTRESOURCEW(id), wszType);
        if (!src)
        {
            break;
        }

        hg = LoadResource(NULL, src);
        if (!hg)
        {
            break;
        }

        DWORD size = SizeofResource(NULL, src);  
        HANDLE file = CreateFileW(
            wszDstPath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            0,
            NULL
            );
        if (!file || INVALID_HANDLE_VALUE == file)
        {
            break;
        }

        if (size <= 0)
        {
            break;
        }

        if(!_WriteFileBuffer(file, (const char *)hg, size))
        {
            break;
        }
        state = TRUE;
    } while (FALSE);

    if(hg)
    {
        FreeResource(hg);
    }

    if (file && INVALID_HANDLE_VALUE != file)
    {
        CloseHandle(file);
    }
    return TRUE;  
}