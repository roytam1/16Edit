#ifndef __Macros_h__
#define __Macros_h__

#include <windows.h>
#include <limits.h>

#define ARRAY_ITEMS(name) sizeof(name) / sizeof(name[0])
#define TESTBIT(val, flag)  (val & flag)
#define ZERO(strct) memset(&strct, 0, sizeof(strct));
#define MEMCPY mymemcpy

// Compatibility for old SDKs (e.g. VC6) that lack 64-bit pointer APIs.
// NOTE: LONG_PTR/UINT_PTR/DWORD_PTR are typedefs (not macros) and
// SetWindowLongPtr/GetClassLongPtr are functions (not macros), so plain
// #ifndef would ALWAYS trigger — even on 64-bit SDKs — and silently
// truncate 64-bit pointers/handles to 32-bit (crash: jump to 0x40001FEC
// instead of 0x140001FEC). Therefore these fallbacks must NEVER apply
// on _WIN64; they are only for 32-bit builds with old SDKs.
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
#if !defined(_WIN64)
#if _MSC_VER < 1300
#ifndef LONG_PTR
#define LONG_PTR LONG
#endif
#ifndef ULONG_PTR
#define ULONG_PTR ULONG
#endif
#ifndef UINT_PTR
#define UINT_PTR UINT
#endif
#ifndef INT_PTR
#define INT_PTR INT
#endif
#ifndef DWORD_PTR
#define DWORD_PTR DWORD
#endif
#if _MSC_VER < 1200
typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;
typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;

/* _atoi64 is missing in VC4 */
LONGLONG _atoi64 (const char * nptr);
#endif
#endif
#if _MSC_VER < 1300
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#ifndef GetClassLongPtr
#define GetClassLongPtr GetClassLong
#endif
#endif
#endif
#ifndef SIZE_MAX
#define SIZE_MAX ((SIZE_T)-1)
#endif
#ifndef TBSTYLE_FLAT
#define TBSTYLE_FLAT 0x800
#endif
#ifndef MOVEFILE_WRITE_THROUGH
#define MOVEFILE_WRITE_THROUGH 0x8
#endif

#if _MSC_VER < 1200
#define TO_WNDPROC(x) ((int (__stdcall *)(void))x)
#else
#define TO_WNDPROC(x) (x)
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

void mymemcpy(void *dest, void *src, SIZE_T count);
void DebugPrint(char *szFormat, ...);

#endif
