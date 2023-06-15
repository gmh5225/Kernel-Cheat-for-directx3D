#include "function.h"

PVOID GetSystemModuleBase(const char* moduleName)
{
	ULONG bytes = 0; 
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, bytes, &bytes); 

	if (!bytes) 
		return NULL;

	PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePool2(POOL_FLAG_NON_PAGED, bytes, 0x4e554c4c);

	status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes); 
	
	if (!NT_SUCCESS(status))
		return NULL;
	PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;
	PVOID moduleBase = 0, moduleSize = 0;

	for (ULONG i = 0; i < modules->NumberOfModules; i++)
	{
		if (strcmp((char*)module[i].FullPathName, moduleName)==0)
		{
			moduleBase = module[i].ImageBase;
			moduleSize =(PVOID) module[i].ImageSize;
			break;
		}
	}

	if (modules)
		ExFreePool(modules);
	if (moduleBase <= NULL)
		return NULL;

	return moduleBase;
}

PVOID GetSystemModuleExport(const char* moduleName, LPCSTR routineName)
{

	PVOID lpModule = GetSystemModuleBase(moduleName);
	if (!lpModule)
		return NULL;
	return RtlFindExportedRoutineByName(lpModule, routineName);
}


PVOID GetSystemModuleExport(LPCWSTR moduleName, LPCSTR routineName)
{
	PLIST_ENTRY moduleList = reinterpret_cast<PLIST_ENTRY>(GetSystemRoutineAddress(L"PsLoadedModuleList"));
	if (!moduleList)
		return NULL;

	for (PLIST_ENTRY link = moduleList->Flink; link != moduleList; link = link->Flink)
	{
		LDR_DATA_TABLE_ENTRY * entry = CONTAINING_RECORD(link, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
		UNICODE_STRING name;
		RtlInitUnicodeString(&name, moduleName);

		if (RtlEqualUnicodeString(&entry->BaseDllName, &name, TRUE))
		{
			// Check if the module base address is valid
			if (entry->DllBase == NULL || entry->SizeOfImage == 0)
				continue;

			// Check if the routine name is provided
			if (routineName != NULL)
			{
			
				PVOID routineAddress = RtlFindExportedRoutineByName(entry->DllBase, routineName);
				if (routineAddress != NULL)
					return routineAddress;
			}
			else
			{
				// No routine name provided, return the module base address
				return entry->DllBase;
			}
		}
	}

	return NULL;
}
bool WriteMemory(void* address, void* buffer, size_t size)
{
	if (!RtlCopyMemory(address, buffer, size))
	{
		return false;
	}
	else {
		return true;
	}
}

bool WriteToReadOnlyMemory(void* address, void* buffer, size_t size)
{
	PMDL mdl = IoAllocateMdl(address, (ULONG)size, false, false, NULL);//mdl이 가상주소 페이지의 설명 메모리 할당
	if (!mdl)
		return false;
	MmProbeAndLockPages(mdl, KernelMode, IoReadAccess); //할당된 메모리를 인자값에 해당하는 페이지 조사,mdl이 물리적 페이지를 descript하도록 변경 후lock
	PVOID Mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);//할당한 메모리를 매핑
	MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);//매핑된 메모리 권한 설정
	WriteMemory(Mapping, buffer, size);
	MmUnmapLockedPages(Mapping, mdl);// 언매핑
	IoFreeMdl(mdl);//mdl 할당 해지
	return true;
}

ULONG64 GetModuleBaseX64(PEPROCESS proc, UNICODE_STRING moduleName)
{
	PPEB pPeb = PsGetProcessPeb(proc);
	if (!pPeb)
		return NULL;
	KAPC_STATE state;
	KeStackAttachProcess(proc, &state);

	PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;
	if (!pLdr)
	{
		KeUnstackDetachProcess(&state);
		return NULL;
	}

	for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink; list != &pLdr->ModuleListLoadOrder; list = (PLIST_ENTRY)list->Flink)
	{
		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);

		if (RtlCompareUnicodeString(&pEntry->BaseDllName, &moduleName, TRUE) == NULL)
		{
			ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
			KeUnstackDetachProcess(&state);
			return baseAddr;
		}
	}

	KeUnstackDetachProcess(&state);
	return NULL;


}

bool ReadKernelMemory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size)
{
	if (!address || !buffer || !size) 
		return false;

	SIZE_T bytes = 0; 
	NTSTATUS status = STATUS_SUCCESS; 
	PEPROCESS process; 

	if ((HANDLE)pid == 0) return false;
	PsLookupProcessByProcessId((HANDLE)pid, &process); 
	status = MmCopyVirtualMemory(process, (void*)address, (PEPROCESS)PsGetCurrentProcess(), (void*)buffer, size, KernelMode, &bytes); 

	if (!NT_SUCCESS(status))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool WriteKernelMemory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size)
{
	KAPC_STATE state;
	NTSTATUS status = STATUS_SUCCESS; 
	PEPROCESS process;
	
	if(!address||!buffer||!size)
		return false;
	
	if ((HANDLE)pid == 0) 
		return false;

	PsLookupProcessByProcessId((HANDLE)pid, &process); 
	KeStackAttachProcess((PEPROCESS)process, &state);//주소에 타겟 프로세스의 루틴으로 현재 스레드를 attach
	
	MEMORY_BASIC_INFORMATION info;
	status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)address, MemoryBasicInformation, &info, sizeof(info), NULL);//메모리 주소에 대한 베이스 주소와 보호상태 확인을 위한 INFO 
	

	if (((uintptr_t)info.BaseAddress + info.RegionSize) < (address + size)) 
	{
		KeUnstackDetachProcess(&state);
		return false;
	}

	if (!(info.State & MEM_COMMIT) || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
	{
		KeUnstackDetachProcess(&state);
		return false;
	}

	if ((info.Protect & PAGE_EXECUTE_READWRITE) || (info.Protect & PAGE_EXECUTE_WRITECOPY)
		|| (info.Protect & PAGE_READWRITE) || (info.Protect & PAGE_WRITECOPY))
	{
		RtlCopyMemory((void*)address, buffer, size);
	}
	KeUnstackDetachProcess(&state);
	return true;

}

PVOID GetSystemRoutineAddress(PCWSTR routineName)
{
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, routineName);
	return MmGetSystemRoutineAddress(&name);
}


