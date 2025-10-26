#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <basetsd.h>
#include <WTypesbase.h>
#include <intrin.h>
#include <string>

inline uint64_t cr3;
inline uint64_t base_address;

namespace mem {
	inline HANDLE processHandle = nullptr;
	inline INT32 process_id;

	inline bool Connected_Driver() {

		return 0;
	}

	inline uintptr_t base_address() {

		return 0;
	}

	inline bool CR3() {

		return 0;
	}

	inline INT32 find_process(LPCTSTR process_name) {

		PROCESSENTRY32 pt;
		HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		pt.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hsnap, &pt)) {
			do {
				if (!lstrcmpi(pt.szExeFile, process_name)) {
					CloseHandle(hsnap);
					process_id = pt.th32ProcessID;
					return pt.th32ProcessID;
				}
			} while (Process32Next(hsnap, &pt));
		}
		CloseHandle(hsnap);

		return { NULL };
	}

	template <typename T>
	bool ReadMemory(uint64_t address, T* buffer, size_t size)
	{
		if (!processHandle || !address || !buffer)
			return false;

		SIZE_T bytesRead;
		return ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead)
			&& bytesRead == size;
	}

	// Shortcut overload
	template <typename T>
	T Read(uint64_t address)
	{
		T value{};
		ReadMemory(address, &value, sizeof(T));
		return value;
	}

}