#ifndef _IOPONN_H_
#define _IOPONN_H_

#include "windows.h"

typedef bool (*OPONN_CALLBACK) (LPCONTEXT pContext);

#define OPONNLIBRARY_EXPORT

#if defined(OPONNLIBRARY_EXPORT) // inside DLL
#   define OPONNAPI   __declspec(dllexport)
#else // outside DLL
#   define OPONNAPI   __declspec(dllimport)
#endif

class IOponn {
public:
	//Searches for a window with the given title and attaches to it
	//Returns false on failure, true on success
	virtual bool Attach(const char* windowTitle) = 0;

	//Attaches to the process of the given window 
	//Returns false on failure, true on success
	virtual bool Attach(HWND hWnd) = 0;

	//Attaches to the given process
	//Returns false on failure, true on success
	virtual bool Attach(DWORD processId) = 0;

	//Detaches from the current process
	virtual void Detach() = 0;

	//Returns the handlet of the given module, if it has been loaded
	//Otherwise returns NULL
	virtual HMODULE GetModuleHandle(const char* moduleName, bool useCache = true) = 0;

	//Returns the address of the given function/module. If the module name is iomitted,
	//the function is looked for in the process's main module
	virtual FARPROC GetFunctionAddress(const char* funcName, const char* moduleName = 0) = 0;
	
	//Adds an intercept at the given function of the given module. If the module name
	//is omitted, the function is looked for in the process's main module
	virtual void AddIntercept(OPONN_CALLBACK intercept, const char* funcName, const char* moduleName = NULL) = 0;

	//Adds an intercept at the given address
	virtual void AddIntercept(OPONN_CALLBACK intercept, FARPROC address) = 0;

	//Removes the intercept at the given address
	virtual void RemoveIntercept(FARPROC address) = 0;

	//Removes the intercept at the given function
	virtual void RemoveIntercept(const char* funcName, const char* moduleName = 0) = 0;

	//Reads the process's memory into the given buffer. See ReadProcessMemory
	virtual bool ReadMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesRead = NULL) = 0;

	//Writes the given buffer to the process's memory. See WriteProcessMemory
	virtual bool WriteMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesWritten = NULL) = 0;

	//Returns whether or not intercepts are enabled
	virtual bool IsInterceptsEnabled() = 0;

	//Enables or disables intercepts
	virtual void SetInterceptsEnabled(bool enableIntercepts) = 0;
	
	//Begins intercepting. This runs an indefinite loop so it should be called in a separate thread!
	virtual void StartIntercepting() = 0;

};

extern "C" OPONNAPI IOponn* CreateOponn();

#endif