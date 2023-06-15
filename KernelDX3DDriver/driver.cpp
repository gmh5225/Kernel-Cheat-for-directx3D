#include "hook.h"

extern "C" NTSTATUS EntryPoint(PDRIVER_OBJECT driverObj,PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(driverObj);
	UNREFERENCED_PARAMETER(registryPath);

	bool fatched =CallKernelFunction(&HookHandler);

	DbgPrint("Driver Loaded %d\n",fatched);
	return STATUS_SUCCESS;
}