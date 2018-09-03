#ifndef COMMON_P2PDBG_H_H_
#define COMMON_P2PDBG_H_H_
#include <Windows.h>

typedef BOOL (WINAPI* pfnGdFileHandlerW)(LPCWSTR, void*);

VOID CentreWindow(HWND hParent, HWND hChild);

BOOL ReleaseResource(LPCWSTR wszDstPath, DWORD id, LPCWSTR wszType);

HANDLE ProcExecProcessW(LPCWSTR cmdLine, DWORD* procId, BOOL bShowWindow);

BOOL FileEnumFileW(LPCWSTR wszDir, BOOL bRecursion, pfnGdFileHandlerW handler, void* lpParam);

BOOL GetPeVersionW(LPCWSTR lpszFileName, LPWSTR outBuf, UINT size);
#endif