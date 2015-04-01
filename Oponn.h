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

class Oponn : public IOponn {
public:
	//Searches for a window with the given title and attaches to it
	//Returns false on failure, true on success
	bool Attach(const char* windowTitle);

	//Attaches to the process of the given window 
	//Returns false on failure, true on success
	bool Attach(HWND hWnd);

	//Attaches to the given process
	//Returns false on failure, true on success
	bool Attach(DWORD processId);

	//Detaches from the current process
	void Detach();

	//Returns the handlet of the given module, if it has been loaded
	//Otherwise returns NULL
	HMODULE GetModuleHandle(const char* moduleName, bool useCache = true);

	//Returns the address of the given function/module. If the module name is iomitted,
	//the function is looked for in the process's main module
	FARPROC GetFunctionAddress(const char* funcName, const char* moduleName = 0);
	
	//Adds an intercept at the given function of the given module. If the module name
	//is omitted, the function is looked for in the process's main module
	void AddIntercept(OPONN_CALLBACK intercept, const char* funcName, const char* moduleName = NULL);

	//Adds an intercept at the given address
	void AddIntercept(OPONN_CALLBACK intercept, FARPROC address);

	//Removes the intercept at the given address
	void RemoveIntercept(FARPROC address);

	//Removes the intercept at the given function
	void RemoveIntercept(const char* funcName, const char* moduleName = 0) {
		RemoveIntercept(GetFunctionAddress(funcName, moduleName));
	}

	//Reads the process's memory into the given buffer. See ReadProcessMemory
	bool ReadMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesRead = NULL);

	//Writes the given buffer to the process's memory. See WriteProcessMemory
	bool WriteMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesWritten = NULL);

	//Returns whether or not intercepts are enabled
	bool IsInterceptsEnabled() {
		return isInterceptsEnabled;
	}

	//Enables or disables intercepts
	void SetInterceptsEnabled(bool enableIntercepts) {
		isInterceptsEnabled = enableIntercepts;
	}
	
	//Begins intercepting. This runs an indefinite loop so it should be called in a separate thread!
	void StartIntercepting();

	Oponn() : hasInstalled(false), isInterceptsEnabled(true), isRunning(false), processId(0), hProcess(0) {}
	~Oponn() { Detach(); }

private:
	//Internally used for stopping interceptions
	//BOOL stopSignal;
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
	//Intercept callbacks
	map<FARPROC, OPONN_CALLBACK> callbacks;
	//Original intercept instructions
	map<FARPROC, unsigned char> instructions;	
};


#endif