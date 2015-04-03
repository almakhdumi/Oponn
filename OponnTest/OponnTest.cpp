#include "stdafx.h"
#include <Windows.h>
#include "CppUnitTest.h"
#include "..\Oponn\IOponn.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#include <windows.h>
#include <strsafe.h>

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}


namespace OponnTest
{
	PROCESS_INFORMATION targetInfo;
	char targetWindowTitle[128];
	IOponn* oponn;

	TEST_CLASS(OponnTest)
	{
	public:

		TEST_CLASS_INITIALIZE(TestClassInit)
		{
			STARTUPINFO startupInfo;
			ZeroMemory(&startupInfo, sizeof(startupInfo));
			startupInfo.cb = sizeof(startupInfo);
			
			sprintf_s(targetWindowTitle, "%X", GetTickCount());
			startupInfo.lpTitle = targetWindowTitle;

			ZeroMemory(&targetInfo, sizeof(targetInfo));

			// Start the child process. 
			BOOL success = CreateProcess(NULL, "OponnTestTarget.exe",
				NULL,           // Process handle not inheritable
				NULL,           // Thread handle not inheritable
				FALSE,          // Set handle inheritance to FALSE
				CREATE_NEW_CONSOLE,  
				NULL,           // Use parent's environment block
				NULL,           // Use parent's starting directory 
				&startupInfo,   // Pointer to STARTUPINFO structure
				&targetInfo);   // Pointer to PROCESS_INFORMATION structure

			Assert::IsTrue(success);

			// Wait for the window to exist

			const int maxWaitTime = 1000, pollTime = 100;
			DWORD start = GetTickCount();
			HWND targetHwnd = NULL;

			DWORD now = GetTickCount();
			while ((targetHwnd = FindWindow(NULL, targetWindowTitle)) == NULL &&
				(GetTickCount() - start) < maxWaitTime)
			{
				Sleep(pollTime);
			}
		}

		TEST_METHOD_INITIALIZE(TestMethodInit)
		{
			oponn = CreateOponn();
		}

		TEST_METHOD_CLEANUP(TestMethodCleanup)
		{
			if (oponn) delete oponn;
			oponn = NULL;
		}

		TEST_METHOD(TestAttachDetach)
		{
			bool attached = oponn->Attach(targetInfo.dwProcessId);
			Assert::IsTrue(attached);

			bool attachedAgain = oponn->Attach(targetInfo.dwProcessId);
			Assert::IsFalse(attachedAgain);

			bool detached = oponn->Detach();
			Assert::IsTrue(detached);

			attached = oponn->Attach(targetInfo.dwProcessId);
			Assert::IsTrue(attached);

			IOponn* oponn2 = CreateOponn();
			bool secondAttached = oponn2->Attach(targetInfo.dwProcessId);
			Assert::IsFalse(secondAttached);
		}

		TEST_METHOD(TestAttachPid)
		{
			bool attachedFakePid = oponn->Attach((DWORD)0);
			Assert::IsFalse(attachedFakePid);

			bool attached = oponn->Attach(targetInfo.dwProcessId);
			//if (!attached) ErrorExit("TestAttachHwnd");
			Assert::IsTrue(attached);
		}
		
		TEST_METHOD(TestAttachWindowTitle)
		{
			bool attached = oponn->Attach(targetWindowTitle);
			Assert::IsTrue(attached);
		}

		TEST_METHOD(TestAttachHwnd)
		{
			HWND targetHwnd = FindWindow(NULL, targetWindowTitle);
			Assert::AreNotEqual(NULL, (INT_PTR)targetHwnd);

			bool attached = oponn->Attach(targetHwnd);
			//if (!attached) ErrorExit("TestAttachHwnd");
			Assert::IsTrue(attached);
		}

		TEST_METHOD(TestGetFunctionAddress)
		{
			oponn->Attach(targetInfo.dwProcessId);
			HMODULE localModule = GetModuleHandle("user32.dll");
			FARPROC localAddress = GetProcAddress(localModule, "MessageBoxW");

			INT_PTR localOffset = (INT_PTR)localAddress - (INT_PTR)localModule;
						

			CloseHandle(localModule);

		}



	};

}