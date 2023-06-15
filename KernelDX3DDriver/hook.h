#pragma once
#include "function.h"

bool CallKernelFunction(void* kernelFunctionAddress);
NTSTATUS HookHandler(PVOID calledParam);
INT FrameRect(HDC 	hDC,
	CONST RECT* lprc,
	HBRUSH 	hbr,
	int thickness
);

typedef HBRUSH(*GdiSelectBrush_t)(_In_ HDC 	hdc,
	_In_ HBRUSH 	hbr
	);
typedef BOOL(*PatBlt_t)(_In_ 	HDC,
	_In_ int x,
	_In_ int y,
	_In_ int w,
	_In_ int h,
	_In_ 	DWORD
	);
typedef HDC(*NtUserGetDC_t)(HWND hWnd);


typedef HBRUSH (*NtGdiCreateSolidBrush_t)(_In_ COLORREF 	cr,
	_In_opt_ HBRUSH 	hbr
);


typedef int (*RealeaseDC_t)(HDC hdc);
typedef BOOL(*DeleteObjectApp_t)(HANDLE hobj);





typedef BOOL(*NtGdiExtTextOutW_t)(IN HDC 	hDC,
	IN INT 	XStart,
	IN INT 	YStart,
	IN UINT 	fuOptions,
	IN OPTIONAL LPRECT 	UnsafeRect,
	IN LPWSTR 	UnsafeString,
	IN INT 	Count,
	IN OPTIONAL LPINT 	UnsafeDx,
	IN DWORD 	dwCodePage
	);