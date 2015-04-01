#include "stdafx.h"
#include <Psapi.h>
#include "RemoteOps.h"
#include "ioponn.h"
#include "oponn.h"

bool Oponn::Attach(const char* windowTitle)
{
	return Attach(FindWindow(NULL, windowTitle));
}

bool Oponn::Attach(HWND hWnd)
{
	if (hWnd == NULL) return false;
	DWORD processId = NULL;
	GetWindowThreadProcessId(hWnd, &processId);
	if (processId == NULL) return false;
	return Attach(processId);
}

bool Oponn::Attach(DWORD processId)
{
	if (hProcess) return false;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (!hProcess) return false;

	if (!DebugActiveProcess(processId)) return false;
	DebugSetProcessKillOnExit(FALSE); //Don't kill the process if we exit

	return true;
}


void Oponn::Detach()
{
	DebugActiveProcessStop(processId);
	isRunning = false;
	
	//Write back all the original instructions
	for(map<FARPROC, unsigned char>::iterator iter = instructions.begin(); iter != instructions.end(); ++iter) {
		WriteProcessMemory(hProcess, (void*) iter->first, &(iter->second), sizeof(iter->second), 0);
		FlushInstructionCache(hProcess, (void*) iter->first, 1); //Make sure the instruction cache is updated
	}
	hProcess = NULL;
}

HMODULE Oponn::GetModuleHandle(const char* nameOfModule, bool useCache)
{
	static char temp[MAX_PATH]; //for getting module names
	char* moduleName = (char*) nameOfModule;
	HMODULE hModule = NULL;

	//If no module was specified, get the name of the process itself
	if (!moduleName)
	{
		GetModuleBaseName(hProcess, hModule, temp, MAX_PATH);
		moduleName = new char[strlen(temp)+1];
		strcpy(moduleName, temp);
	}

	if (useCache && moduleCache.count(moduleName))
	{
		hModule = moduleCache[moduleName];
	}
	else
	{
		HMODULE hModule = GetRemoteModuleHandle(hProcess, moduleName);
		if (hModule)
			moduleCache[moduleName] = hModule;
		//If we aren't saving the module but allocated space, delete it
		else if (nameOfModule != moduleName)
			delete [] moduleName;
	}
	return hModule;
}

FARPROC Oponn::GetFunctionAddress(const char* funcName, const char* moduleName)
{
	HMODULE hModule = GetModuleHandle(moduleName);
	if (!hModule) return NULL;

	pair<HMODULE, const char*> func(hModule, funcName);
	if (functionCache.count(func))
		return functionCache[func];
	else {
		FARPROC addr = GetRemoteProcAddress(hProcess, hModule, funcName);
		if (addr) functionCache[func] = addr;
		return addr;
	}
}

void Oponn::RemoveIntercept(FARPROC address)
{
	if (!instructions.count(address)) return;
	bool tempInterceptsEnabled = IsInterceptsEnabled();
	SetInterceptsEnabled(false); //Temporarily disable intercepts

	//Overwrite the INT3 breakpoint with the old data
	char instr = instructions[address];
	WriteMemory(address, &instr, sizeof(instr));

	instructions.erase(address);
	callbacks.erase(address);

	SetInterceptsEnabled(tempInterceptsEnabled); //Restart intercepts, if they were enabled
}
	
void Oponn::AddIntercept(OPONN_CALLBACK callback, const char* funcName, const char* moduleName)
{
	FARPROC address = GetFunctionAddress(funcName, moduleName);
	AddIntercept(callback, address);
}

void Oponn::AddIntercept(OPONN_CALLBACK callback, FARPROC address)
{
	//Remove any existing intercept
	RemoveIntercept(address);

	//Store the instruction byte as it is now
	unsigned char instr;
	ReadMemory(address, &instr, sizeof(instr));
	instructions[address] = instr;
	
	//Write the breakpoint
	instr = INT3_BP;
	WriteMemory(address, &instr, sizeof(instr));
	
	//Add the callback
	callbacks[address] = callback;
}

bool Oponn::ReadMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesRead)
{
	return ReadProcessMemory(hProcess, address, buffer, size, pNumBytesRead) ? true : false;
}

bool Oponn::WriteMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesWritten)
{
	return WriteProcessMemory(hProcess, address, buffer, size, pNumBytesWritten) ? true : false;
}

void Oponn::StartIntercepting()
{
	static const unsigned char bpInstr = INT3_BP;

	if (isRunning) return;

	DEBUG_EVENT DebugEv;

	isRunning = true;
	isInterceptsEnabled = true;
	hasInstalled = false;
	//stopSignal = false;

	while (true)
	{
		
		if (!WaitForDebugEvent(&DebugEv, DEBUG_EVENT_WAIT_MS))
			continue; //Continue loop on event wait timeout

		FARPROC addr = (FARPROC)DebugEv.u.Exception.ExceptionRecord.ExceptionAddress;

		switch (DebugEv.dwDebugEventCode) { 
		case EXCEPTION_DEBUG_EVENT: 

			switch(DebugEv.u.Exception.ExceptionRecord.ExceptionCode)
			{ 
			case EXCEPTION_ACCESS_VIOLATION: break;

			case EXCEPTION_BREAKPOINT: 
				//The very first breakpoint occurs when we attach, so this is 
				//when we replace the intercept instructions with INT3 breakpoints
				if (!hasInstalled)
				{
					hasInstalled = true;
					//The instructions are already backed up in the instructions map 
					//so we only need to write the breakpoints
					for(map<FARPROC, unsigned char>::iterator iter = instructions.begin(); iter != instructions.end(); ++iter)
					{
						WriteProcessMemory(hProcess, (void*) iter->first, &bpInstr, sizeof(bpInstr), 0);
						FlushInstructionCache(hProcess, (void*) iter->first, 1); //Make sure the instruction cache is updated
					}
				}
				//We've hit a real breakpoint here, so now it's just a matter of calling the right
				//callback function, executing the original instruction, and rewriting the breakpoint
				else
				{
					HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, DebugEv.dwThreadId);
					
					//Get the thread context
					CONTEXT context;
					context.ContextFlags = CONTEXT_ALL;
					GetThreadContext(hThread, &context);

					//Now get and call the callback function for this intercept
					if (isInterceptsEnabled)
					{
						OPONN_CALLBACK callback = callbacks[addr];
						callback(&context);
					}

					//Write the original instruction back and reduce EIP (the PC) by 1 byte,
					//which will cause the original instruction to execute
					unsigned char instr = instructions[addr];
					context.Eip--;
					WriteProcessMemory(hProcess, (void*) addr, &instr, sizeof(instr), 0);
					FlushInstructionCache(hProcess, (void*) addr, 1); //Make sure the instruction cache is updated

					//We also have to set the trap flag so that we can break again right
					//after the original instruction is executed and re-write the breakpoint
					context.EFlags |= CPU_TRAP_FLAG;

					//Finally write the thread context back
					SetThreadContext(hThread, &context);
				}
				break;


			case EXCEPTION_SINGLE_STEP: 
				//This exception occurs after we set the trap flag
				//so we write back the breakpoint if it still exists
				if (instructions.count(addr) > 0)
				{
					WriteProcessMemory(hProcess, (void*) addr, &bpInstr, sizeof(bpInstr), 0);
					FlushInstructionCache(hProcess, (void*) addr, 1); //Make sure the instruction cache is updated
				}
				break;

			case EXCEPTION_DATATYPE_MISALIGNMENT: break;
			case DBG_CONTROL_C: break;
			default: break;
			} 

			break;

		case CREATE_THREAD_DEBUG_EVENT: 
			//New thread created. Can examine and do stuff here.
			//TODO: add option for intercepting thread creation too
			break;

		case CREATE_PROCESS_DEBUG_EVENT: 
			break;

		case EXIT_THREAD_DEBUG_EVENT: 
			//TODO: add intercepts for thread exit
			break;

		case EXIT_PROCESS_DEBUG_EVENT: 
			//TODO: add intercepts for process exit
			break;

		case LOAD_DLL_DEBUG_EVENT: 
			//TODO: add intercepts for dll loading
			break;

		case UNLOAD_DLL_DEBUG_EVENT: 
			//TODO: add intercepts for dll unloading
			break;

		case OUTPUT_DEBUG_STRING_EVENT: 
			//add option to print out debugged strings?
			break;

		case RIP_EVENT:
			//no idea what this is. MSDNA says "system debugging error" - fatal debug error?
			break;
		} 

		//Resume the thread
		ContinueDebugEvent(DebugEv.dwProcessId, 
				DebugEv.dwThreadId, 
				DBG_CONTINUE);
	}

}

extern "C" OPONNAPI IOponn* CreateOponn()
{
	return new Oponn();
}