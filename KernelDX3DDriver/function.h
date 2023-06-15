#pragma once
#include "define.h"

PVOID GetSystemModuleBase(const char* moduleName);
PVOID GetSystemModuleExport(const char* moduleName, LPCSTR routineName);
bool WriteMemory(void* address, void* buffer, size_t size);
bool WriteToReadOnlyMemory(void* address, void* buffer, size_t size);
ULONG64 GetModuleBaseX64(PEPROCESS proc, UNICODE_STRING moduleName);
bool ReadKernelMemory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size);
bool WriteKernelMemory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size);
PVOID GetSystemRoutineAddress(PCWSTR routineName);
PVOID GetSystemModuleExport(LPCWSTR moduleName, LPCSTR routineName);
typedef struct _NULL_MEMORY
{
	void* buffer_address;
	UINT_PTR address;
	ULONGLONG size;
	ULONG pid;
	BOOLEAN write;
	BOOLEAN read;
	BOOLEAN req_base;
	BOOLEAN draw_box;
	int		r, g, b, x, y, w, h, t;
	void* output;
	const char* module_name;
	ULONG64 base_address;
}NULL_MEMORY;