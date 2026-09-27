#ifndef __Macros_h__
#define __Macros_h__

#include <windows.h>
#include <limits.h>

void mymemcpy(void *dest, void *src, SIZE_T count);
void DebugPrint(char *szFormat, ...);

#define ARRAY_ITEMS(name) sizeof(name) / sizeof(name[0])
#define TESTBIT(val, flag)  (val & flag)
#define ZERO(strct) memset(&strct, 0, sizeof(strct));
#define MEMCPY mymemcpy

// Compatibility for old SDKs (e.g. VC6) that lack 64-bit pointer APIs.
// On 32-bit these map to the 32-bit versions; on 64-bit SDKs the real
// 64-bit versions are used.
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif
#ifndef GCLP_HICON
#ifdef GCL_HICON
#define GCLP_HICON GCL_HICON
#else
#define GCLP_HICON (-14)
#endif
#endif
#ifndef LONG_PTR
#define LONG_PTR LONG
#endif
#ifndef UINT_PTR
#define UINT_PTR UINT
#endif
#ifndef DWORD_PTR
#define DWORD_PTR DWORD
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#ifndef GetClassLongPtr
#define GetClassLongPtr GetClassLong
#endif
#ifndef SIZE_MAX
#define SIZE_MAX ((SIZE_T)-1)
#endif

#if defined(_M_ALPHA) && defined(_WIN64)
#ifdef GWLP_WNDPROC
#define GWL_WNDPROC GWLP_WNDPROC
#endif
 #define FUNC_CALLBACK __cdecl
 #define FUNC_RET int
#elif defined(_WIN64)
#ifdef GWLP_WNDPROC
#define GWL_WNDPROC GWLP_WNDPROC
#endif
 #define FUNC_CALLBACK 
 #define FUNC_RET __int64
#elif defined(_M_ALPHA)
 #define FUNC_CALLBACK 
 #define FUNC_RET int
#else
 #define FUNC_CALLBACK __stdcall
 #define FUNC_RET int
#endif

#ifndef GCL_HICON
#define GCL_HICON (-14)
#endif

#endif
