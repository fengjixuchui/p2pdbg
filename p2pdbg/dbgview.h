#ifndef DBGVIEW_P2PDBG_H_H_
#define DBGVIEW_P2PDBG_H_H_

void NotifyConnectSucc();

void NotifyLogFile(LPCWSTR wszDesc, LPCWSTR wszLogFile, bool bCompress);

void NotifyMessage(LPCWSTR wszMsg);

void ShowDbgViw();
#endif