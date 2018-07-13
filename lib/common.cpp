#include <Windows.h>
#include <Shlwapi.h>
#include "common.h"

VOID CentreWindow(HWND hParent, HWND hChild)
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
        file = CreateFileW(
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

HANDLE ProcExecProcessW(LPCWSTR cmdLine, DWORD* procId, BOOL bShowWindow)
{
    if (!cmdLine)
    {
        return NULL;
    }

    STARTUPINFOW si = {sizeof(si)};
    if (!bShowWindow)
    {
        si.wShowWindow = SW_HIDE;
        si.dwFlags = STARTF_USESHOWWINDOW;
    }

    PROCESS_INFORMATION pi;

    LPWSTR wszCmdLine = (LPWSTR)malloc((MAX_PATH + lstrlenW(cmdLine)) * sizeof(WCHAR));
    lstrcpyW(wszCmdLine, cmdLine);
    if (CreateProcessW(NULL, wszCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);

        if (procId)
        {
            *procId = pi.dwProcessId;
        }

        free((void*)wszCmdLine);
        return pi.hProcess;
    }

    free((void*)wszCmdLine);
    return NULL;
}

BOOL FileEnumFileW(LPCWSTR wszDir, BOOL bRecursion, pfnGdFileHandlerW handler, void* lpParam)
{
    if (!wszDir || !handler)
    {
        return FALSE;
    }

    DWORD attrib = GetFileAttributesW(wszDir);
    if (!(attrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        return FALSE;
    }

    WCHAR findStr[MAX_PATH];
    lstrcpyW(findStr, wszDir);
    PathAppendW(findStr, L"*");

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(findStr, &findData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        return FALSE;
    }

    BOOL bRet = TRUE;

    do
    {
        if (0 == lstrcmpiW(findData.cFileName, L".") || 0 == lstrcmpiW(findData.cFileName, L".."))
        {
            continue;
        }
        WCHAR fileName[MAX_PATH];
        lstrcpyW(fileName, wszDir);
        PathAppendW(fileName, findData.cFileName);

        if (INVALID_FILE_ATTRIBUTES == findData.dwFileAttributes)
        {
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (bRecursion)
            {
                // 2 标志着用户主动退出，则无论递归了多少层，都应该退出
                if (2 == FileEnumFileW(fileName, bRecursion, handler, lpParam))
                {
                    bRet = 2;
                    break;
                }
            }
        }
        else
        {
            if (!handler(fileName, lpParam))
            {
                bRet = 2;
                break;
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);

    return bRet;
}