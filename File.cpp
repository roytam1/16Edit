
#include <malloc.h>
#include <limits.h>
#include "File.h"
#include "Macros.h"

// Chunk size for ReadFile/WriteFile loops (must fit in DWORD).
// 1GB keeps each Win32 call well below the 4GB DWORD limit.
#define CFILE_IO_CHUNK (0x40000000UL)

CFile::CFile()
{
	// Initialize before Destroy() so Destroy() never reads
	// uninitialized stack garbage (which would crash on free/CloseHandle).
	hFile = INVALID_HANDLE_VALUE;
	pMap = NULL;
	qwMapSize = 0;
	bReadOnly = FALSE;
	cFilePath[0] = '\0';
}

CFile::~CFile()
{
	// Cleanup
	Destroy();
}

BOOL CFile::OpenFileForSave() {
	DWORD  dwFlags, dwAccess, dwShare;

	Destroy();

	bReadOnly = FALSE;
	dwFlags = OPEN_ALWAYS;
	dwAccess = GENERIC_READ | GENERIC_WRITE;
	dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;

	hFile = CreateFile(
		cFilePath,
		dwAccess,
		dwShare,
		NULL,
		dwFlags,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == (HANDLE)-1)
		return FALSE;

	return TRUE;
}

//
// Open a file in the specified mode
//
BOOL CFile::GetFileHandle(char *szFilePath, DWORD dwMode)
{
	DWORD  dwFlags, dwAccess, dwShare;

	// destroy if sth is already loaded
	Destroy();

	// save path
	lstrcpy(cFilePath, szFilePath);

	// handle flags
	switch(dwMode)
	{
	case F_OPENEXISTING_R:
		bReadOnly = TRUE;
		dwFlags = OPEN_EXISTING;
		dwAccess = GENERIC_READ;
		dwShare = FILE_SHARE_READ;
		break;

	case F_OPENEXISTING_RW:
		bReadOnly = FALSE;
		dwFlags = OPEN_EXISTING;
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;

	case F_CREATENEW:
		bReadOnly = FALSE;
		dwFlags = CREATE_ALWAYS;
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;

	case F_TRUNCATE: // RW
		bReadOnly = FALSE;
		dwFlags = TRUNCATE_EXISTING;
		dwAccess = GENERIC_READ | GENERIC_WRITE;
		dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	}

	hFile = CreateFile(
		szFilePath,
		dwAccess,
		dwShare,
		NULL,
		dwFlags,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == (HANDLE)-1)
		return FALSE;

	return TRUE;
}

//
// opens a file with RW access if possible else just R
BOOL CFile::GetFileHandleWithMaxAccess(char* szFilePath)
{
	BOOL bRet;

	bRet = GetFileHandle(szFilePath, F_OPENEXISTING_RW);
	if (!bRet)
		bRet = GetFileHandle(szFilePath, F_OPENEXISTING_R);

	return bRet;
}

//
// Cleanup routine
//
BOOL CFile::Destroy()
{
	BOOL bRet = FALSE;

	// cleanup
	if (hFile != INVALID_HANDLE_VALUE)
		if (CloseHandle(hFile))
			bRet = TRUE;
	if (pMap)
		free(pMap);

	// adjust variables
	hFile        = INVALID_HANDLE_VALUE;
	pMap         = NULL;
	qwMapSize    = 0;

	return bRet;
}

//
// returns:
// INVALID_HANDLE_VALUE - if no file is loaded
//
HANDLE CFile::GetHandle()
{
	return hFile;
}

BOOL CFile::IsFileReadOnly()
{
	return bReadOnly;
}

BOOL CFile::MapFile()
{
	ULONGLONG qwFSize;

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	qwFSize = GetFSize();
	if (qwFSize == (ULONGLONG)-1)
		return FALSE;

	// malloc takes SIZE_T: fail cleanly if file doesn't fit in address space
	// (e.g. >4GB file opened in a 32-bit build).
	if (qwFSize > (ULONGLONG)(SIZE_MAX))
		return FALSE;

	if (qwFSize == 0)
	{
		// malloc(0) is implementation defined; keep a valid non-NULL marker
		// so IsMapped()/GetMapSize() behave consistently for empty files.
		pMap = malloc(1);
		if (!pMap)
			return FALSE;
		qwMapSize = 0;
		return TRUE;
	}

	// map file
	pMap = malloc((SIZE_T)qwFSize);
	if (!pMap)
		return FALSE;

	// ReadFile takes a DWORD byte count, so loop for files >4GB.
	if (!SetFPointer(0))
	{
		free(pMap);
		pMap = NULL;
		qwMapSize = 0;
		return FALSE;
	}
	if (!Read(pMap, qwFSize))
	{
		free(pMap);
		pMap = NULL;
		qwMapSize = 0;
		return FALSE;
	}

	// set vars
	qwMapSize = qwFSize;

	return TRUE;
}

void* CFile::GetMapPtr()
{
	return pMap;
}

BOOL CFile::UnmapFile()
{
	if (!pMap)
		return FALSE;

	free(pMap);
	pMap       = NULL;
	qwMapSize  = 0;

	return TRUE;
}

//
// change size of file memory
//
BOOL CFile::ReMapFile(ULONGLONG qwNewSize)
{
	void *pNew;

	if (!pMap)
		return FALSE; // ERR

	if (qwNewSize > (ULONGLONG)(SIZE_MAX))
		return FALSE; // ERR - doesn't fit in address space

	pNew = realloc(pMap, (SIZE_T)qwNewSize);
	if (!pNew && qwNewSize != 0)
		return FALSE; // ERR - keep old pMap/qwMapSize intact

	pMap      = pNew;
	qwMapSize = qwNewSize;
	return TRUE; // OK
}

//
// returns:
// (ULONGLONG)-1 in the case of an error
//
ULONGLONG CFile::GetMapSize()
{
	if (!pMap)
		return (ULONGLONG)-1; // ERR

	return qwMapSize;
}

BOOL CFile::IsMapped()
{
	return pMap != NULL ? TRUE : FALSE;
}

//
// copy mapping memory to file
//
BOOL CFile::FlushFileMap()
{
	if ( !Truncate() )
		return FALSE; // ERR

	return Write(pMap, qwMapSize);
}

BOOL CFile::FileExits(char* szFilePath)
{
	CFile f;

	return f.GetFileHandle(szFilePath, F_OPENEXISTING_R);
}

ULONGLONG CFile::GetFSize()
{
	DWORD dwLow, dwHigh;
	DWORD dwErr;

	if (hFile == INVALID_HANDLE_VALUE)
		return (ULONGLONG)-1;

	// Use GetFileSize with high DWORD so this builds on old SDKs (VC6)
	// and works on old Windows, while still returning full 64-bit sizes >4GB.
	SetLastError(NO_ERROR);
	dwLow = GetFileSize(hFile, &dwHigh);
	dwErr = GetLastError();
	if (dwLow == (DWORD)-1 && dwErr != NO_ERROR)
		return (ULONGLONG)-1;

	return ((ULONGLONG)dwHigh << 32) | (ULONGLONG)dwLow;
}
	
//
// returns:
// NULL - if no file is loaded
//	
char* CFile::GetFilePath()
{
	return cFilePath;
}

//
// write to file (chunked so sizes >4GB work; WriteFile takes DWORD)
//
BOOL CFile::Write(void* pBuff, ULONGLONG qwCount)
{
	BYTE *p = (BYTE*)pBuff;
	ULONGLONG qwLeft = qwCount;

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	// Writing 0 bytes is a no-op success (needed for empty files).
	while (qwLeft > 0)
	{
		DWORD dwToWrite = (qwLeft > CFILE_IO_CHUNK) ? CFILE_IO_CHUNK : (DWORD)qwLeft;
		DWORD dwWritten = 0;

		if (!WriteFile(hFile, p, dwToWrite, &dwWritten, NULL))
			return FALSE;
		if (dwWritten != dwToWrite)
			return FALSE;

		p += dwWritten;
		qwLeft -= dwWritten;
	}

	return TRUE;
}

//
// read from file (chunked so sizes >4GB work; ReadFile takes DWORD)
//
BOOL CFile::Read(void* pBuff, ULONGLONG qwCount)
{
	BYTE *p = (BYTE*)pBuff;
	ULONGLONG qwLeft = qwCount;

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	while (qwLeft > 0)
	{
		DWORD dwToRead = (qwLeft > CFILE_IO_CHUNK) ? CFILE_IO_CHUNK : (DWORD)qwLeft;
		DWORD dwRead = 0;

		if (!ReadFile(hFile, p, dwToRead, &dwRead, NULL))
			return FALSE;
		if (dwRead != dwToRead)
			return FALSE;

		p += dwRead;
		qwLeft -= dwRead;
	}

	return TRUE;
}

//
// set file pointer (64-bit via SetFilePointer with high DWORD;
// works on old SDKs/Windows and supports offsets >4GB)
//
BOOL CFile::SetFPointer(ULONGLONG qwOff)
{
	LONG lLow, lHigh;
	DWORD dwRet;

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	lLow = (LONG)(qwOff & 0xFFFFFFFFUL);
	lHigh = (LONG)(qwOff >> 32);

	SetLastError(NO_ERROR);
	dwRet = SetFilePointer(hFile, lLow, &lHigh, FILE_BEGIN);
	if (dwRet == (DWORD)-1 && GetLastError() != NO_ERROR)
		return FALSE;

	return TRUE;
}

BOOL CFile::Truncate()
{
	if (!SetFPointer(0))
		return FALSE; // ERR
	if (!SetEndOfFile(hFile))
		return FALSE; // ERR

	return TRUE; // OK
}

void CFile::SetMapPtrSize(void* ptr, ULONGLONG qwSize)
{
	pMap      = ptr;
	qwMapSize = qwSize;

	return;
}

// 64MB sliding view: fits 32-bit address space easily, few remaps.
#define PAGED_VIEW_SIZE (0x4000000UL)

CPagedFile::CPagedFile()
{
	hFile = INVALID_HANDLE_VALUE;
	hMap = NULL;
	qwSize = 0;
	pView = NULL;
	qwViewOff = 0;
	cbView = 0;
	dwGran = 0;
	bReadOnly = TRUE;
	cPath[0] = '\0';
}

CPagedFile::~CPagedFile()
{
	Close();
}

void CPagedFile::Close()
{
	if (pView)
	{
		UnmapViewOfFile(pView);
		pView = NULL;
	}
	if (hMap)
	{
		CloseHandle(hMap);
		hMap = NULL;
	}
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	qwSize = 0;
	qwViewOff = 0;
	cbView = 0;
	cPath[0] = '\0';
}

BOOL CPagedFile::IsOpen()
{
	return (hFile != INVALID_HANDLE_VALUE) ? TRUE : FALSE;
}

ULONGLONG CPagedFile::GetSize()
{
	return qwSize;
}

BOOL CPagedFile::Open(const char *szFilePath, BOOL bRO)
{
	DWORD dwLow, dwHigh, dwErr;
	DWORD dwAccess, dwShare, dwProtect;
	SYSTEM_INFO si;

	Close();

	lstrcpy(cPath, szFilePath);
	bReadOnly = bRO;

	dwAccess = GENERIC_READ;
	dwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
	hFile = CreateFile(szFilePath, dwAccess, dwShare, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	SetLastError(NO_ERROR);
	dwLow = GetFileSize(hFile, &dwHigh);
	dwErr = GetLastError();
	if (dwLow == (DWORD)-1 && dwErr != NO_ERROR)
	{
		Close();
		return FALSE;
	}
	qwSize = ((ULONGLONG)dwHigh << 32) | (ULONGLONG)dwLow;

	GetSystemInfo(&si);
	dwGran = si.dwAllocationGranularity;
	if (dwGran == 0)
		dwGran = 65536;

	if (qwSize > 0)
	{
		dwProtect = PAGE_READONLY;
		hMap = CreateFileMapping(hFile, NULL, dwProtect, 0, 0, NULL);
		// hMap may stay NULL (e.g. empty file); ReadAt falls back to ReadFile.
	}

	return TRUE;
}

BOOL CPagedFile::EnsureView(ULONGLONG qwOff)
{
	ULONGLONG qwAligned, qwViewSize;
	DWORD dwLow, dwHigh;

	if (hFile == INVALID_HANDLE_VALUE || qwOff >= qwSize)
		return FALSE;
	if (hMap == NULL)
		return FALSE; // caller falls back to ReadFile
	if (pView && qwOff >= qwViewOff && qwOff < qwViewOff + (ULONGLONG)cbView)
		return TRUE;

	qwAligned = (qwOff / (ULONGLONG)dwGran) * (ULONGLONG)dwGran;
	qwViewSize = (ULONGLONG)PAGED_VIEW_SIZE;
	if (qwAligned + qwViewSize > qwSize)
		qwViewSize = qwSize - qwAligned;

	if (pView)
	{
		UnmapViewOfFile(pView);
		pView = NULL;
		cbView = 0;
	}

	dwLow = (DWORD)(qwAligned & 0xFFFFFFFFUL);
	dwHigh = (DWORD)(qwAligned >> 32);
	pView = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, dwHigh, dwLow, (SIZE_T)qwViewSize);
	if (!pView)
		return FALSE;

	qwViewOff = qwAligned;
	cbView = (SIZE_T)qwViewSize;
	return TRUE;
}

BOOL CPagedFile::ReadAt(ULONGLONG qwOff, void *pBuf, SIZE_T cb)
{
	BYTE *pDst = (BYTE*)pBuf;

	if (!IsOpen() || pBuf == NULL)
		return FALSE;
	if (cb == 0)
		return TRUE;
	if (qwOff >= qwSize || qwOff + (ULONGLONG)cb > qwSize)
		return FALSE;

	while (cb > 0)
	{
		if (hMap != NULL && EnsureView(qwOff))
		{
			SIZE_T cbAvail = (SIZE_T)((qwViewOff + (ULONGLONG)cbView) - qwOff);
			SIZE_T cbCopy = (cb < cbAvail) ? cb : cbAvail;
			memcpy(pDst, pView + (SIZE_T)(qwOff - qwViewOff), cbCopy);
			pDst += cbCopy;
			qwOff += (ULONGLONG)cbCopy;
			cb -= cbCopy;
		}
		else
		{
			// Fallback: positional ReadFile for the remainder of this chunk.
			LONG lLow = (LONG)(qwOff & 0xFFFFFFFFUL);
			LONG lHigh = (LONG)(qwOff >> 32);
			DWORD dwRet;
			DWORD dwWant = (cb > CFILE_IO_CHUNK) ? CFILE_IO_CHUNK : (DWORD)cb;
			DWORD dwRead = 0;

			SetLastError(NO_ERROR);
			dwRet = SetFilePointer(hFile, lLow, &lHigh, FILE_BEGIN);
			if (dwRet == (DWORD)-1 && GetLastError() != NO_ERROR)
				return FALSE;
			if (!ReadFile(hFile, pDst, dwWant, &dwRead, NULL))
				return FALSE;
			if (dwRead != dwWant)
				return FALSE;
			pDst += dwRead;
			qwOff += (ULONGLONG)dwRead;
			cb -= (SIZE_T)dwRead;
		}
	}

	return TRUE;
}

BOOL CPagedFile::GetByteAt(ULONGLONG qwOff, BYTE *pby)
{
	return ReadAt(qwOff, pby, 1);
}
