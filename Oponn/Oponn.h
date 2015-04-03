#ifndef _OPONN_H_
#define _OPONN_H_

#include "stdafx.h"
#include "ioponn.h"
#include <map>

#define INT3_BP 0xCC
#define CPU_TRAP_FLAG 0x100
#define DEBUG_EVENT_WAIT_MS 100

typedef bool (*OPONN_CALLBACK) (LPCONTEXT pContext);

using namespace std;

struct OponnIntercept
{
	// First byte of instruction, kept as a backup
	BYTE instructionByte;
	// 
	OPONN_CALLBACK callback;
};

class Oponn : public IOponn
{
public:
	bool Attach(const char* windowTitle);

	bool Attach(HWND hWnd);

	bool Attach(DWORD processId);

	bool Detach();

	HMODULE GetModuleHandle(const char* moduleName, bool useCache = true);

	FARPROC GetFunctionAddress(const char* funcName, const char* moduleName = 0);
	
	void AddIntercept(OPONN_CALLBACK intercept, const char* funcName, const char* moduleName = NULL);

	void AddIntercept(OPONN_CALLBACK intercept, FARPROC address);

	void RemoveIntercept(FARPROC address);

	void RemoveIntercept(const char* funcName, const char* moduleName = 0)
	{
		RemoveIntercept(GetFunctionAddress(funcName, moduleName));
	}

	bool ReadMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesRead = NULL);

	bool WriteMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesWritten = NULL);

	bool IsInterceptsEnabled()
	{
		return isInterceptsEnabled;
	}

	void SetInterceptsEnabled(bool enableIntercepts)
	{
		isInterceptsEnabled = enableIntercepts;
	}
	
	void StartIntercepting();

	Oponn() : hasInstalled(false), isInterceptsEnabled(true), isRunning(false), processId(0), hProcess(0) {}
	~Oponn() { Detach(); }

private:
	//Internally used for initial 'install' of breakpoints 
	bool hasInstalled;
	//Whether or not intercept callbacks are enabled
	bool isInterceptsEnabled;
	//Whether or not currently intercepting
	bool isRunning;
	//Attached process's ID
	DWORD processId;
	//Handle to attached process
	HANDLE hProcess;
	//Cache of module handles (ie addresses)
	map<const char*, HMODULE> moduleCache;
	//Cache of function addresses
	map<pair<HMODULE,const char*>, FARPROC> functionCache;
	//Intercepts
	map<FARPROC, OponnIntercept> intercepts;
};


#endif