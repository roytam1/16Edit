
/*****************************************************************************

  CFile
  ----

  This is a class for handling files and memory maps of them.

  by yoda

  WWW:      y0da.cjb.net
  E-mail:   LordPE@gmx.net

  You are allowed to use this source code in your own projects if you mention
  my name.

*****************************************************************************/

#ifndef __File_h__
#define __File_h__

#include <windows.h>

//
// constants
//
// modes for GetFileHandle
#define F_OPENEXISTING_R     0
#define F_OPENEXISTING_RW    1
#define F_CREATENEW          2
#define F_TRUNCATE           3

//
// class CFile
//
class CFile
{
public:
	CFile();
	~CFile();
	BOOL		  OpenFileForSave();
	BOOL          GetFileHandle(char *szFilePath, DWORD dwMode);
	BOOL          GetFileHandleWithMaxAccess(char* szFilePath);
	BOOL          Destroy();
	HANDLE        GetHandle();
	BOOL          IsFileReadOnly();
	BOOL          MapFile();
	void*         GetMapPtr();
	BOOL          UnmapFile();
	BOOL          ReMapFile(ULONGLONG qwNewSize);
	ULONGLONG     GetMapSize();
	BOOL          IsMapped();
	ULONGLONG     GetFSize();
	BOOL          FlushFileMap();
	static BOOL   FileExits(char* szFilePath);
	char*         GetFilePath();
	BOOL          Write(void* pBuff, ULONGLONG qwCount);
	BOOL          Read(void* pBuff, ULONGLONG qwCount);
	BOOL          SetFPointer(ULONGLONG qwOff);
	BOOL          Truncate();
	void          SetMapPtrSize(void* ptr, ULONGLONG qwSize);

private:
	ULONGLONG     qwMapSize;
	void          *pMap;
	BOOL          bReadOnly;
	HANDLE        hFile;
	char          cFilePath[MAX_PATH];
};

//
// CPagedFile - file-backed sliding-window reader for files too large
// to fit in one contiguous malloc (32-bit builds editing >2GB files).
// Keeps a single mapped view (64MB) and remaps on demand; falls back
// to ReadFile when mapping is unavailable. Read-only; edits are kept
// in a separate overlay by the caller.
//
class CPagedFile
{
public:
	CPagedFile();
	~CPagedFile();
	BOOL          Open(const char *szFilePath, BOOL bReadOnly);
	void          Close();
	BOOL          IsOpen();
	ULONGLONG     GetSize();
	BOOL          ReadAt(ULONGLONG qwOff, void *pBuf, SIZE_T cb);
	BOOL          GetByteAt(ULONGLONG qwOff, BYTE *pby);

private:
	BOOL          EnsureView(ULONGLONG qwOff);

	HANDLE        hFile;
	HANDLE        hMap;
	ULONGLONG     qwSize;
	BYTE          *pView;
	ULONGLONG     qwViewOff;
	SIZE_T        cbView;
	DWORD         dwGran;
	BOOL          bReadOnly;
	char          cPath[MAX_PATH];
};

#endif
