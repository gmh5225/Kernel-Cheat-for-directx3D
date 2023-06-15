#include "hook.h"

GdiSelectBrush_t GdiSelectBrush = NULL;
PatBlt_t NtGdiPatBlt = NULL;
NtGdiCreateSolidBrush_t NtGdiCreateSolidBrush = NULL;
NtUserGetDC_t NtUserGetDc = NULL;
RealeaseDC_t ReleaseDc = NULL;
DeleteObjectApp_t DeleteObjectApp = NULL;
NtGdiExtTextOutW_t NtGdiExtTextOutW = NULL;
int (*func)(int a) = NULL;
INT Dx[10] = { 10, -5, 10, 5, 10, -10, 10, 5, 10, 5 };

bool CallKernelFunction(void* kernelFunctionAddress)
{
	
	if (!kernelFunctionAddress)
		return false;
	PVOID* function = reinterpret_cast<PVOID*>(GetSystemModuleExport("\\SystemRoot\\System32\\drivers\\dxgkrnl.sys",
		"NtDxgkGetTrackedWorkloadStatistics"));
	
	if (!function)
		return false;
	BYTE original[] = { 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
	BYTE shellCodeStart[] = { 0x48,0xB8 };// MOV RAX , XXX
	BYTE shellCodeEnd[] = { 0xFF,0xE0 }; //jmp RAX

	RtlSecureZeroMemory(&original, sizeof(original));
	memcpy(original, shellCodeStart, sizeof(shellCodeStart));
	uintptr_t hookAddress = reinterpret_cast<uintptr_t>(kernelFunctionAddress);
	memcpy(original + sizeof(shellCodeStart), &hookAddress, sizeof(void*));
	memcpy(original + sizeof(shellCodeStart) + sizeof(void*), &shellCodeEnd, sizeof(shellCodeEnd));
	WriteToReadOnlyMemory(function, &original, sizeof(original));
	
	
	GdiSelectBrush = (GdiSelectBrush_t)GetSystemModuleExport(L"win32kfull.sys", "NtGdiSelectBrush");
	NtGdiCreateSolidBrush = (NtGdiCreateSolidBrush_t)GetSystemModuleExport(L"win32kfull.sys", "NtGdiCreateSolidBrush");
	NtGdiPatBlt = (PatBlt_t)GetSystemModuleExport(L"win32kfull.sys", "NtGdiPatBlt");
	NtUserGetDc = (NtUserGetDC_t)GetSystemModuleExport(L"win32kbase.sys", "NtUserGetDC");
	ReleaseDc = (RealeaseDC_t)GetSystemModuleExport(L"win32kbase.sys", "NtUserReleaseDC");
	DeleteObjectApp = (DeleteObjectApp_t)GetSystemModuleExport(L"win32kbase.sys", "NtGdiDeleteObjectApp");
	NtGdiExtTextOutW = (NtGdiExtTextOutW_t)GetSystemModuleExport(L"win32kfull.sys", "NtGdiExtTextOutW");
	return true;
}

NTSTATUS HookHandler(PVOID calledParam) //NtOpenSurface ~~ hooked work
{
	NULL_MEMORY* instructions = (NULL_MEMORY*)calledParam;
	if (instructions->draw_box == TRUE)
	{
		HDC hdc = NtUserGetDc(NULL);
		if (!hdc)
			return STATUS_UNSUCCESSFUL;

		HBRUSH brush = NtGdiCreateSolidBrush(RGB(instructions->r, instructions->g, instructions->b), NULL);
		if (!brush)
			return STATUS_UNSUCCESSFUL;

		RECT rect = { instructions->x, instructions->y, instructions->x + instructions->w, instructions->y + instructions->h };
		FrameRect(hdc, &rect, brush, instructions->t);
		
		
		
		
		ReleaseDc(hdc);
		DeleteObjectApp(brush);
	}

	return STATUS_SUCCESS;
}
INT FrameRect(HDC 	hDC,
	CONST RECT* lprc,
	HBRUSH 	hbr,
	int thickness
	)
{
	HBRUSH oldbrush;
	RECT r = *lprc;
	oldbrush = GdiSelectBrush(hDC, hbr);
	if (!oldbrush) return 0;

	NtGdiPatBlt(hDC, r.left, r.top, thickness, r.bottom - r.top, PATCOPY);
	NtGdiPatBlt(hDC, r.right - thickness, r.top, thickness, r.bottom - r.top, PATCOPY);
	NtGdiPatBlt(hDC, r.left, r.top, r.right - r.left, thickness, PATCOPY);
	NtGdiPatBlt(hDC, r.left, r.bottom - thickness, r.right - r.left, thickness, PATCOPY);
	
	GdiSelectBrush(hDC, oldbrush);
	return TRUE;
}

