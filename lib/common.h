#ifndef COMMON_P2PDBG_H_H_
#define COMMON_P2PDBG_H_H_
#include <Windows.h>

VOID WINAPI CentreWindow(HWND hParent, HWND hChild);

BOOL ReleaseResource(LPCWSTR wszDstPath, DWORD id, LPCWSTR wszType);
#endif