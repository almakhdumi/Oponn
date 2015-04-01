#pragma once
#include <windows.h>
#include <vcclr.h>
#include "..\Oponn\IOponn.h"

#using <System.dll>

using namespace System;
using namespace System::Runtime::InteropServices;

namespace AlMakhdumi {
	namespace Oponn {

		public ref class ManagedClass {
		public:
			ManagedClass() : pImpl(CreateOponn()) {}

			~ManagedClass() {
				delete pImpl;
			}

		protected:
			!ManagedClass() {
				delete pImpl;
			}

		public:

			//Searches for a window with the given title and attaches to it
			//Returns false on failure, true on success
			bool Attach(String^ windowTitle);

			//Attaches to the process of the given window 
			//Returns false on failure, true on success
			bool Attach(IntPtr hWnd);

			//Attaches to the given process
			//Returns false on failure, true on success
			bool Attach(DWORD processId);

			//Detaches from the current process
			void Detach();

			//Returns the handlet of the given module, if it has been loaded
			//Otherwise returns NULL
			IntPtr GetModule(String^ moduleName, bool useCache);

			//Returns the address of the given function/module. If the module name is iomitted,
			//the function is looked for in the process's main module
			IntPtr GetFunctionAddress(String^ funcName, String^ moduleName);

			//Adds an intercept at the given function of the given module. If the module name
			//is omitted, the function is looked for in the process's main module
			void AddIntercept(OPONN_CALLBACK intercept, String^ funcName, String^ moduleName);

			//Adds an intercept at the given address
			void AddIntercept(OPONN_CALLBACK intercept, FARPROC address);

			//Removes the intercept at the given address
			void RemoveIntercept(FARPROC address);

			//Removes the intercept at the given function
			void RemoveIntercept(String^ funcName, String^ moduleName);

			//Reads the process's memory into the given buffer. See ReadProcessMemory
			bool ReadMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesRead);

			//Writes the given buffer to the process's memory. See WriteProcessMemory
			bool WriteMemory(FARPROC address, LPVOID buffer, SIZE_T size, SIZE_T* pNumBytesWritten);

			//Returns whether or not intercepts are enabled
			bool IsInterceptsEnabled();

			//Enables or disables intercepts
			void SetInterceptsEnabled(bool enableIntercepts);

			//Begins intercepting. This runs an indefinite loop so it should be called in a separate thread!
			void StartIntercepting();

			property String ^  get_PropertyA {
				String ^ get() {
					return gcnew String(m_Impl->GetPropertyA());
				}
			}

			void MethodB(String ^ theString) {
				pin_ptr<const WCHAR> str = PtrToStringChars(theString);
				m_Impl->MethodB(str);
			}

		private:
			IOponn* pImpl;
		};
	}
}
