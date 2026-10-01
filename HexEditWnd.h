#ifndef __HexEditWnd_h__
#define __HexEditWnd_h__

#include <windows.h>
#include <shellapi.h>
#include "Common.h"
#include "File.h"

#ifndef EXTERN_HE_STRUCTS
#define EXTERN_HE_STRUCTS

#define                 ID_TB               0x2000
#define                 ID_TRAYICON         0x2001

#define                 IDT_FIRST           0x1000
#define                 IDT_RESTORE         IDT_FIRST + 1
#define                 IDT_EXIT            IDT_FIRST + 2
#define                 WM_TRAYMENU         WM_USER   + IDT_FIRST

#define					HEDIT_WND_CLASS     "16Edit Main Window"
#define					HEDIT_CLASS     	"16Edit Client Window"
#define					HEDIT_WND_TITLE     "[ 16Edit ]"
#define                 HEDIT_TRAY_TIP      "[16Edit]"
#define                 szMemoryBuff        "memory buffer"
#define                 HEDIT_WND_WIDTH     715
#define					HEDIT_WND_HEIGHT    458
#define                 SB_HEIGHT           20
#define                 TB_HEIGHT           26

#define                 INI_NAME            "16Edit.ini"
#define                 INI_SECTION         "Options"
#define                 INI_WINPOSKEY       "WinPos"
#define                 INI_GO_OFFSET       "GO_OFFSET"
#define                 INI_SEARCH_STRING   "SS_STR"
#define                 INI_REPLACE_STRING  "RS_STR"
#define                 INI_SS_OFFSET       "SS_OFFSET"
#define                 INI_FILECD       	"FileCD"
#define                 INI_SHELL       	"ShellMenu"

#define                 CF_16Edit           "CF_16EDIT"

#define                 STR_NO_MEM          "Not enough memory available !"

#define                 DEF_FONT_HEIGHT     17
#define                 DEF_FONT_WIDTH       8

#define                 RGB_BLACK           RGB(0, 0, 0)
#define                 RGB_RED             RGB(0xFF, 0, 0)
#define                 RGB_BLUE            RGB(0, 0, 0xFF)
#define                 RGB_GREEN           RGB(0, 0xFf, 0)
#define                 RGB_SBGRAY          RGB(0x40, 0x40, 0x40)

#define                 H2                  "%02lX"
#define                 H8                  "%08lX"
#define                 _H8                 "0x%08lX"
#define                 H16                 "%08X%08X"

// Format a 64-bit offset as hex without relying on %I64 support in wsprintf:
// 8 digits when the high DWORD is zero (keeps the classic display),
// 16 digits otherwise (files >4GB).
inline void FormatOffset64(char *buf, ULONGLONG qwOff)
{
	if ((DWORD)(qwOff >> 32) == 0)
		wsprintf(buf, H8, (DWORD)qwOff);
	else
		wsprintf(buf, H16, (DWORD)(qwOff >> 32), (DWORD)qwOff);
}

#define                 DEF_MAX_LINES       22 // A LOOK UP VALUE
#define                 PAIRS_X             (10 * uFontWidth)
#define                 DIGIT_PAIR_WIDTH    (3  * uFontWidth)
#define                 CHARS_X             (PAIRS_X + 16 * DIGIT_PAIR_WIDTH + uFontWidth)

#define                 WM_MOUSEWHEEL       0x020A
#define                 WHEEL_MOVE_STEPS    4
#define                 SELECT_TIMER	    1
#define					TOP_OFFSET			4
#define					LEFT_OFFSET			8

#define					HEX_STRING		0
#define					ASC_STRING		1
#define					UNI_STRING		2

typedef struct _HE_WIN_POS {
	int                 ix, iy, icx, icy;
} HE_WIN_POS, *PHE_WIN_POS;

typedef struct _HE_DATA_INFO {
	BYTE        *pDataBuff;
	ULONGLONG   qwSize;     // data indicator (64-bit: supports files >4GB)
	BOOL        bReadOnly;
} HE_DATA_INFO, *PHE_DATA_INFO;

typedef struct _HE_POS
{
	ULONGLONG   qwOffset;
	BOOL        bHiword;       // (opt.) first digit of the pair ? ...or the 2nd one ?
	BOOL        bTextSection;  // (opt.) Caret in the text part ?
} HE_POS, *PHE_POS;

#endif // EXTERN_HE_STRUCTS

typedef enum _EDIT_OPER {
	op_modify,
	op_delete,
	op_add,
	op_cut,
	op_paste
} EDIT_OPER;

class HE_OPER {

public:
	HE_OPER(EDIT_OPER, ULONGLONG, ULONGLONG, ULONGLONG);
	HE_OPER(EDIT_OPER, ULONGLONG, ULONGLONG, ULONGLONG, BOOL bNoAlloc);
	~HE_OPER();

public:
	EDIT_OPER	type;
	ULONGLONG	qwOffset;
	ULONGLONG	qwNewLen;
	ULONGLONG	qwOldLen;
	BYTE		*newData;   // NULL when staged in add store (see below)
	BYTE		*oldData;   // NULL when staged in add store
	ULONGLONG	qwNewAdd;   // add-store offset, or (ULONGLONG)-1
	ULONGLONG	qwOldAdd;   // add-store offset, or (ULONGLONG)-1
};

typedef struct HE_CLIPBOARD_DATA
{
	ULONGLONG           qwDataSize;
	BYTE                byDataStart;
} *PHE_CLIPBOARD_DATA;

// changes to this structure could affect "HexEditWnd::HexEditWnd()"
typedef struct HE_STATUS {
	HE_POS              posLastLButtonDown;
	BOOL                bLastLBDownPos;     // TRUE if posLastLButtonDown is valid
	BOOL                bMouseSelecting;

	HE_POS              posCaret;
	BOOL                bCaret;
	BOOL                bCaretVisible;
	BOOL                bCaretPosValid;     // TRUE if posCaret was at least set one time

	ULONGLONG           qwCurOffset;

	BOOL                bSel;
	ULONGLONG           qwOffSelStart;
	ULONGLONG           qwOffSelEnd;

	LONGLONG            llLastLine;
} *PHE_STATUS;

class HE_SEARCHOPTIONS {
public:
	BOOL                bInited;            // TRUE if the struct was set at least one time

	BYTE*               pData;              // buffer (malloced)
	ULONGLONG           qwBuff;
	ULONGLONG           qwStr;

	BYTE*				pReplaceData;
	ULONGLONG			qwReplaceStr;

	BOOL                bASCIIStr;
	BOOL                bWideCharStr;
	BOOL                bCaseSensitive;

	ULONGLONG           qwStartOff;
	BOOL                bDown;

	int                 iDlgSearchFrom;     // 0 - top, 1 - cur pos, 2 - off
	void*               pDlgStr;            // buffer (malloced)
	void*               pReplaceStr;            // buffer (malloced)
};

typedef HE_SEARCHOPTIONS *PHE_SEARCHOPTIONS;

class EditOperList {

public:
	EditOperList();
	~EditOperList();
	EditOperList* getFirst();
	EditOperList* getLast();
	EditOperList* getNext();
	EditOperList* getPrev();
	EditOperList* getCur();
	BOOL before(EditOperList *);
	HE_OPER* getOper();
	EditOperList* addOper(HE_OPER *);
	void releaseAfter();

private:
	EditOperList *prev;
	EditOperList *next;
	HE_OPER* oper;
};

extern UINT  cf16Edit;

//
// class HexEditWnd
//
class HexEditWnd
{

public:
	HFONT              hFont, hFontU;
	UINT               iyHETop;       // top of HexEdit area
	UINT               iyHEBottom;  
	BOOL               bHEOnTop;
	char               cIniPath[MAX_PATH];
	HWND               hMainWnd, hTB, hStatusBar, hClient ;
	
	HexEditWnd();
	~HexEditWnd();
	void          SetHEWndCaption();
	BOOL          CreateMainWndThread();
	void          SetTBHandle(HWND hTB);
	HINSTANCE     GetInstance();
	BOOL          DoEditFile(char* szFilePath, BOOL bForceReadOnly);
	BOOL          PaintText(HWND hWnd);
	void          HEHandleWM_SETFOCUS(HWND hWnd);
	void          HEHandleWM_KILLFOCUS(HWND hWnd);
	BOOL          HESetFont(HFONT hf);
	void          HEHandleWM_SIZE(HWND hWnd, WPARAM wParam, LPARAM lParam);
	BOOL          HEHandleLButton(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          HEHandleWM_KEYDOWN(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          MakeCaretVisible();
	BOOL          ValidatePos(PHE_POS ppos);
	BOOL          SetCaretSet(BOOL bSet);
	BOOL		  HEHandleWM_TIMER(HWND hWnd, WPARAM wParam);
	BOOL          HEHandleWM_VSCROLL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          HEHandleWM_MOUSEWHEEL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          HEHandleWM_SHOWWINDOW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          HEHandleWM_COMMAND(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          HEHandleWM_NOTIFY(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          InitGotoDlg(HWND hDlg);
	BOOL          GDHandleWM_COMMAND(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT       HEHandleWM_CHAR(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          HEHandleWM_MOUSEMOVE(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          InitSelBlockDlg(HWND hDlg);
	BOOL          SBHandleWM_COMMAND(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          SSInitDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          ReplaceInitDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          SSHandleWM_COMMAND(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL          ReplaceHandleWM_COMMAND(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          SSEnableItems(HWND hDlg);
	BOOL          SSHandleSS_OK(HWND hDlg);
	BOOL          ReplaceHandleSS_OK(HWND hDlg);
	BOOL          DoSpecifySettings(char *, ULONGLONG start = 0, ULONGLONG len = 0);
	BOOL          IsResizingAllowed();
	BOOL          SetCaret(PHE_POS pos);
	BOOL          SetCaret(ULONGLONG qwOffset);
	BOOL          SetCaret();
	BOOL          SetTopLine(int iNewLine);
	BOOL          SetTopLine(ULONGLONG qwOffset);
	BOOL          SetTopLine();
	BOOL          SetSelection(ULONGLONG qwOffStart, ULONGLONG qwOffEnd);
	void          HEHandleWM_CLOSE(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          HEHandleWM_DESTROY(HWND hWnd);
	BOOL          SetHEWnd2Top(BOOL bTop);
	BOOL          HandleStartCaretPosSel(HWND hWnd);
	BOOL          HEditToTray();
	BOOL          HEditKillTrayIcon();
	BOOL          HEditReturnFromTray();
	void          HEHandleWM_TRAYMENU(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void          HEHandleWM_MOVE(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void          HEHandleWM_DROPFILES(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void		  OptionDlgInit(HWND);
	BOOL		  OptionDlgCommand(HWND, DWORD);
	void          ConfigureTBCCP();
	void          ConfigureTB();

typedef struct _HE_EDIT_CELL {
	ULONGLONG   qwOff;
	BYTE        byVal;
} HE_EDIT_CELL;

typedef struct _HE_PIECE {
	BYTE        bySrc;      // 0 = original file, 1 = add store
	ULONGLONG   qwSrcOff;   // offset within source
	ULONGLONG   qwLen;
} HE_PIECE;

private:
	UINT			   timerId;
	EditOperList	   *operList;
	EditOperList	   *current;
	EditOperList	   *savepoint;
	BOOL			   bSavePointValid;
	ULONGLONG		   qwOldSize;
	HINSTANCE          hInst;
	HE_DATA_INFO       diData;        // working buffer (malloc mode)
	HE_DATA_INFO       diOrgData;     // buffer with current file content
	CFile              fInput;
	CPagedFile         pagedFile;     // file-backed reader (large-file mode)
	BOOL               bPagedMode;    // TRUE when diData.pDataBuff is NULL
	HE_EDIT_CELL       *pEditCells;   // sorted overlay of modified bytes (paged mode)
	SIZE_T             nEditCells;
	SIZE_T             capEditCells;
	BYTE               *pHeadCache;   // first 64KB for PE/VA translation (paged mode)
	SIZE_T             cbHeadCache;
	HE_PIECE           *pPieces;      // piece table for size changes (paged mode)
	SIZE_T             nPieces;
	SIZE_T             capPieces;
	SIZE_T             idxPieceHint;  // scan hint for sequential reads
	HANDLE             hAddFile;      // add store for inserted bytes (paged mode)
	char               szAddPath[MAX_PATH];
	ULONGLONG          qwAddSize;
	UINT               uFontHeight, uFontWidth, uMaxLines;
	HE_STATUS          stat;
	HE_SEARCHOPTIONS   search;
	RECT               rctHE;         // rect of HexEdit area relative to HE client area
	BOOL               bResizingAllowed, bMinToTray, bSaveWinPos, bDispMultiByte, bReadOnly;
	BOOL			   bInsert, bFileOffset;
	char               cSBText[256];
	char               cSBInfo[200];
	NOTIFYICONDATA     nidTray;
	HMENU              hmTray;
	char               cInitialDir[MAX_PATH];
	RECT               rctLastPos;

	int			  IsDBCSFirstByte(ULONGLONG qwOffset);
	int			  GetDataStatus(ULONGLONG qwOffset);
	void		  AddOper(HE_OPER*);
	BOOL		  CanCut();
	BOOL		  CanCopy();
	BOOL		  CanPaste();
	BOOL		  CanUndo();
	BOOL		  CanRedo();
	BOOL		  CanSave();
	ULONGLONG	  GetOffset(ULONGLONG qwFileOffset);
	ULONGLONG	  GetFileOffset(ULONGLONG qwVirtualAddress);
	ULONGLONG	  GetVirtualAddress(ULONGLONG qwFileOffset);
	UINT		  GetOffsetDigits();
	UINT		  GetPairsX();
	UINT		  GetCharsX();
	void		  ApplyOper(HE_OPER *);
	void		  UndoOper(HE_OPER *);
	BOOL          IsOffsetVisible(ULONGLONG qwOffset);
	BOOL          PointToPos(IN POINT *pp, OUT PHE_POS ppos);
	void          SetupVScrollbar();
	ULONGLONG     GetTotalLineNum();
	void          RepaintClientArea();
	void          RepaintClientAreaNow();
	void          Beep();
	void          ErrMsg(HWND hWnd, char* szText);
	void          ErrMsg(char* szText);
	void          ErrMsg(HWND hWnd, char* szText, char* szCaption);
	BOOL          IsOutOfRange(ULONGLONG qwOffset);
	BOOL          IsOutOfRange(PHE_POS ppos);
    void          SetCaretPosData(PHE_POS ppos);
    void          SetCaretPosData(ULONGLONG qwOffset);
	BOOL          SaveChanges();
	void          SetStatusInfo(char *szFormat, ...);
	void          SetStatusText(char *szFormat, ...);
	void          SetStatusText();
	BOOL          IsReadOnly();
	BOOL          KillSelection();
	BOOL          MouseMoveSelect(LPPOINT ppos);
	BOOL          Point2Selection(LPPOINT ppClient);
	ULONGLONG     GetCurrentLine();
	BOOL          SetCurrentLine(ULONGLONG qwLine);
	BOOL          ValidateLine(LONGLONG *pllLine);
	BOOL          Search(PHE_SEARCHOPTIONS pso, ULONGLONG *pOffset);
	BOOL          SearchPaged(PHE_SEARCHOPTIONS pso, ULONGLONG *pOffset);
	BOOL          IsPEFile();
	BOOL          IsPagedMode();
	BOOL          PerformStrReplace(PHE_SEARCHOPTIONS pso);
	BOOL          PerformStrReplaceAll(PHE_SEARCHOPTIONS pso);
	BOOL          PerformStrSearch(PHE_SEARCHOPTIONS pso);
	BOOL          PerformSearchAgain(PHE_SEARCHOPTIONS pso, BOOL bDown);
	BOOL          CopySelectedBlock();
	BOOL          CopySelectedBlockAsText();
	BOOL          CutSelectedBlock();
	BOOL          DeleteSelectedBlock();
	BOOL          PasteBlockFromCB();
	BOOL          UndoChanges();
	BOOL          RedoChanges();
	void          SelectAll();
	BOOL          IsAllSelected();
	void          ShowAbout();
	void          InitEdition();
	void          QuitEdition();
	void          HEditQuit();
	BOOL          IsKeyDown(int iVKey);
	void          SetCaretSelInfoToStatus();
	BOOL					IsClipboardFormatOK();
	PHE_CLIPBOARD_DATA   	GetClipboardData();
	// Paged large-file mode (32-bit >2GB): unified read path with overlay.
	BYTE            GetByteAt(ULONGLONG qwOff);
	BOOL            ReadBytesAt(ULONGLONG qwOff, BYTE *pBuf, SIZE_T cb);
	BOOL            PagedOverlayGet(ULONGLONG qwOff, BYTE *pby);
	void            PagedOverlaySet(ULONGLONG qwOff, BYTE byVal);
	void            PagedOverlayRemove(ULONGLONG qwOff);
	void            PagedOverlayClear();
	void            OverlayShiftFrom(ULONGLONG qwPos, LONGLONG lDelta);
	void            OverlayRemoveRange(ULONGLONG qwPos, ULONGLONG qwLen);
	BYTE            PagedRawByte(ULONGLONG qwOff);
	void            ClosePaged();
	BOOL            OpenPaged(const char *szPath, BOOL bRO);
	BOOL            SavePaged();
	BOOL            PagedApplyModify(HE_OPER *op);
	void            PagedUndoModify(HE_OPER *op);
	BOOL            EnsurePieceCap(SIZE_T nNeed);
	BOOL            PieceIndexAt(ULONGLONG qwPos, SIZE_T *pIdx, ULONGLONG *pOffIn);
	BOOL            PagedResolve(ULONGLONG qwPos, BYTE *pSrc, ULONGLONG *pSrcOff, ULONGLONG *pAvail);
	SIZE_T          SplitPieceAt(ULONGLONG qwPos);
	void            MergeAround(SIZE_T idx);
	void            CompactPieces();
	void            RefreshHeadCache();
	ULONGLONG       PagedAddAppend(const BYTE *pData, ULONGLONG qwLen);
	BOOL            ReadAddAt(ULONGLONG qwOff, BYTE *pBuf, SIZE_T cb);
	BOOL            EnsureAddFile();
	void            TruncateAddTo(ULONGLONG qwSize);
	ULONGLONG       StageLogicalToAdd(ULONGLONG qwPos, ULONGLONG qwLen);
	BOOL            PagedInsert(ULONGLONG qwPos, const BYTE *pData, ULONGLONG qwLen);
	BOOL            PagedInsertFromAdd(ULONGLONG qwPos, ULONGLONG qwAddOff, ULONGLONG qwLen);
	BOOL            PagedDelete(ULONGLONG qwPos, ULONGLONG qwLen);

};

DWORD FUNC_CALLBACK HEditWindowThread();
DWORD file_type(char *base);
ULONGLONG get_va(char *base, ULONGLONG file_offset);
ULONGLONG get_fo(char *base, ULONGLONG va_offset);
ULONGLONG pe_max_va(char *base);

#endif
