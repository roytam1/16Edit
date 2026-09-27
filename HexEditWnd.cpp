#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include "HexEditWnd.h"
#include "File.h"
#include "CPathString.h"
#include "OFN.h"
#include "macros.H"

extern HexEditWnd HEdit;

LRESULT                HEditMainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT  FUNC_CALLBACK HEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT  FUNC_CALLBACK TBHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
FUNC_RET FUNC_CALLBACK GotoDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
FUNC_RET FUNC_CALLBACK OptionDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
FUNC_RET FUNC_CALLBACK ReplaceDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
FUNC_RET FUNC_CALLBACK SelBlockDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
FUNC_RET FUNC_CALLBACK SearchDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD    FUNC_CALLBACK HEditWindowThread();

#define SB_STATUS		0
#define SB_ORIGIN_SIZE	1
#define SB_NEW_SIZE		2
#define SB_POSITION		3

const int SBbs[] = {
	150,
	80,
	80,
	80
};

const TBBUTTON   TBbs[]      = {
	{  6, TB_WIN2TOP,        TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  20,TB_OPEN,           TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  2, TB_SAVE,           TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  1, TB_GOTO,           TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  19,TB_READONLY,       TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  16,TB_MULTI,          TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  29,TB_OFFSET,         TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  24,TB_SIZE,           TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  0, NULL,              TBSTATE_ENABLED, (BYTE)TBSTYLE_SEP,    0, 0},
	{  8, TB_SELBLOCK,       TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 15, TB_SELALL,         TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 27, TB_REPLACE,        TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  7, TB_SEARCH,         TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 12, TB_SEARCHUP,       TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 13, TB_SEARCHDOWN,     TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  0, NULL,              TBSTATE_ENABLED, (BYTE)TBSTYLE_SEP,    0, 0},
	{ 23, TB_INSERT,         TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 14, TB_UNDO,           TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 21, TB_REDO,           TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 30, TB_DELETE,         TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  9, TB_CUT,            TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 10, TB_COPY,      	 TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
//	{ 31, TB_COPY_TEXT,      TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{ 11, TB_PASTE,          TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  0, NULL,              TBSTATE_ENABLED, (BYTE)TBSTYLE_SEP,    0, 0},
	{  26, TB_OPTION,        TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  3, TB_ABOUT,          TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0},
	{  0, TB_CLOSE,          TBSTATE_ENABLED, (BYTE)TBSTYLE_BUTTON, 0, 0}
};

WNDPROC            pOrgTBWndProc;
BOOL			   bRightClickMenu;
int				   iFileCDMode;
UINT			   cf16Edit;

void DebugPrint(char *szFormat, ...) {
	char msg[1024];
	va_list args;

	va_start(args, szFormat);
	wvsprintf(msg, szFormat, args);
	va_end(args);
	OutputDebugString(msg);
}

void mymemcpy(void *dest, void *src, SIZE_T count) {
	SIZE_T i;
	if (dest < src) {
		for (i = 0; i < count; i++) {
			((BYTE*)dest)[i] = ((BYTE*)src)[i]; 
		}
	} else {
		for (i = 0; i < count; i++) {
			((BYTE*)dest)[count - 1 - i] = ((BYTE*)src)[count - 1 - i];
		}
	}
}

HexEditWnd::HexEditWnd() {
	LOGFONT lf;
	DWORD	fontHeight;
	DWORD	fontQuality;
	char	fontName[256];

	hInst = GetModuleHandle(NULL);

	iyHETop           = 0;
	iyHEBottom        = 0;
	bDispMultiByte    = FALSE;

	timerId			  = 0;
	hMainWnd          = 0;
	bHEOnTop          = 0;
	bFileOffset		  = TRUE;
	bResizingAllowed  = FALSE;
	bMinToTray        = FALSE;
	bSaveWinPos       = FALSE;
	bInsert			  = FALSE;
	uMaxLines         = DEF_MAX_LINES;
	operList		  = NULL;
	current			  = NULL;

	InitEdition();
	ZERO(search);

	GetModuleFileName( hInst, cInitialDir, sizeof(cInitialDir) );
	CPathString::PathToDir( cInitialDir );
	CPathString::ForceEndBackslash( cInitialDir);
	wsprintf(cIniPath, "%s"INI_NAME, cInitialDir);

	fontHeight = GetPrivateProfileInt(INI_SECTION, "fh", 0, cIniPath);
	fontQuality = GetPrivateProfileInt(INI_SECTION, "fq", 0, cIniPath);
	GetPrivateProfileString(INI_SECTION, "fn", "", fontName, sizeof(fontName), cIniPath);

	ZERO(lf);
	lf.lfHeight     = fontHeight ? -fontHeight : DEF_FONT_HEIGHT;
	//lf.lfWidth      = DEF_FONT_WIDTH;
	lf.lfWeight     = FW_LIGHT;
	if(fontName[0])
		lstrcpy(lf.lfFaceName, fontName);
	else
		lstrcpy(lf.lfFaceName, "Courier New");

	if(fontQuality)
		lf.lfQuality     = fontQuality;

	hFont   = CreateFontIndirect(&lf);

	lf.lfUnderline  = TRUE;
	hFontU  = CreateFontIndirect(&lf);

	cf16Edit = RegisterClipboardFormat(CF_16Edit);

	hmTray = CreatePopupMenu();
	AppendMenu(hmTray, MF_STRING, IDT_RESTORE, "&Restore");
	AppendMenu(hmTray, MF_STRING, IDT_EXIT, "E&xit");
}

HexEditWnd::~HexEditWnd() {
	DeleteObject(hFont);
	DeleteObject(hFontU);

	DestroyMenu(hmTray);
}

void HexEditWnd::InitEdition() {
	ZERO(stat);
	ZERO(search);

	stat.bCaretPosValid = TRUE;
	stat.posCaret.bHiword = TRUE;
	stat.llLastLine = -1;

	delete operList;
	operList		  = new EditOperList();
	current			  = operList;
	savepoint		  = operList;
	bSavePointValid	  = TRUE;
	diData.qwSize     = 0;

	return;
}

void HexEditWnd::QuitEdition() {
	free(diData.pDataBuff);
	diData.pDataBuff = NULL;
	diData.qwSize = 0;
	if (search.bInited) {
		if (search.pData)
			free(search.pData);
		if (search.pDlgStr)
			free(search.pDlgStr);
		if (search.pReplaceData)
			free(search.pReplaceData);
		if (search.pReplaceStr)
			free(search.pReplaceStr);
		search.pData = NULL;
		search.pDlgStr = NULL;
		search.pReplaceData = NULL;
		search.pReplaceStr = NULL;
	}
	return;
}

DWORD FUNC_CALLBACK HEditWindowThread() {
	WNDCLASS               wc;
	MSG                    msg;
	UINT                   icx, icy, ix, iy;
	HWND                   hWnd, hTB;
	RECT                   rct;
	HACCEL                 hAccel;
	HE_WIN_POS             wp;

	InitCommonControls();

	ZERO(wc);
	wc.style          = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance      = HEdit.GetInstance();
	wc.hIcon          = LoadIcon(HEdit.GetInstance(), (PSTR)IDI_16Edit);
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszClassName  = HEDIT_WND_CLASS;
	wc.lpfnWndProc    = HEditWndProc;
	RegisterClass(&wc);

	wc.lpszClassName  = HEDIT_CLASS;
	wc.lpfnWndProc    = HEditWndProc;
	RegisterClass(&wc);

	wp.icx = icx = HEDIT_WND_WIDTH;
	wp.icy = icy = HEDIT_WND_HEIGHT;
	wp.ix = ix = (GetSystemMetrics(SM_CXFULLSCREEN) - icx) / 2;
	wp.iy = iy = (GetSystemMetrics(SM_CYFULLSCREEN) - icy) / 2;

	hWnd = CreateWindow(
		   HEDIT_WND_CLASS,
		   HEDIT_WND_TITLE,
		   WS_OVERLAPPEDWINDOW,
		   ix,
		   iy,
		   icx,
		   icy,
		   0,
		   NULL,
		   HEdit.GetInstance(),
		   NULL);

	HEdit.hMainWnd = hWnd;

	hTB = CreateToolbarEx(
		 hWnd,
		 WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_FLAT,
		 ID_TB,
		 50, // number of buttons in the bitmap
		 NULL,
		 (UINT_PTR)LoadBitmap(HEdit.GetInstance(), (PSTR)IDB_TOOLBAR),
		 (LPTBBUTTON)&TBbs,
		 ARRAY_ITEMS(TBbs),
		 16,
		 16,
		 16,
		 16,
		 sizeof(TBBUTTON));
	HEdit.SetTBHandle(hTB);
	pOrgTBWndProc = (WNDPROC)SetWindowLongPtr(hTB, GWLP_WNDPROC, (LONG_PTR)TBHookProc);

	GetClientRect(hTB, &rct);
	HEdit.iyHETop = rct.bottom - rct.top + 2;

	hAccel = LoadAccelerators(HEdit.GetInstance(), (PSTR)IDR_ACCEL);

	bRightClickMenu = FALSE;
	iFileCDMode = 0;
	GetPrivateProfileStruct(INI_SECTION, INI_SHELL, &bRightClickMenu, sizeof(BOOL), HEdit.cIniPath);
	GetPrivateProfileStruct(INI_SECTION, INI_FILECD, &iFileCDMode, sizeof(int), HEdit.cIniPath);

	GetPrivateProfileStruct(INI_SECTION, INI_WINPOSKEY, &wp, sizeof(wp), HEdit.cIniPath);
	MoveWindow( hWnd, wp.ix, wp.iy, wp.icx, wp.icy, TRUE);
	ShowWindow( hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWnd, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}

BOOL HexEditWnd::DoEditFile(char* szFilePath, BOOL bForceReadOnly) {
	BOOL          bRet;

	fInput.Destroy();

	if (bForceReadOnly)
		bRet = fInput.GetFileHandle(szFilePath, F_OPENEXISTING_R);
	else
		bRet = fInput.GetFileHandleWithMaxAccess(szFilePath);
	if (!bRet)
		return FALSE;

	if (!fInput.MapFile())
		return FALSE;

	diOrgData.bReadOnly  = fInput.IsFileReadOnly();
	diOrgData.qwSize     = fInput.GetFSize();
	diOrgData.pDataBuff  = (BYTE*)fInput.GetMapPtr();
	qwOldSize = diOrgData.qwSize;

	diData = diOrgData;
	fInput.SetMapPtrSize(NULL, 0);
	fInput.Destroy();

	if (diData.pDataBuff && diData.qwSize >= sizeof(IMAGE_DOS_HEADER)) {
		if (file_type((char *)diData.pDataBuff)) {
			bFileOffset = FALSE;
		}
	}

	SetHEWndCaption();
	return TRUE;
}

ULONGLONG HexEditWnd::GetFileOffset(ULONGLONG qwVirtualAddress) {
	ULONGLONG	qwOffset;

	if (diData.pDataBuff && diData.qwSize >= sizeof(IMAGE_DOS_HEADER) &&
		file_type((char *)diData.pDataBuff)) {
		qwOffset = get_fo((char *)diData.pDataBuff, qwVirtualAddress);
	} else {
		qwOffset = qwVirtualAddress;
	}
	return qwOffset;
}

ULONGLONG HexEditWnd::GetVirtualAddress(ULONGLONG qwFileOffset) {
	ULONGLONG	qwOffset;

	if (diData.pDataBuff && diData.qwSize >= sizeof(IMAGE_DOS_HEADER) &&
		file_type((char *)diData.pDataBuff)) {
		qwOffset = get_va((char *)diData.pDataBuff, qwFileOffset);
	} else {
		qwOffset = qwFileOffset;
	}
	return qwOffset;
}

ULONGLONG HexEditWnd::GetOffset(ULONGLONG qwFileOffset) {
	ULONGLONG	qwOffset;

	if (bFileOffset) {
		qwOffset = qwFileOffset;
	} else {
		qwOffset = GetVirtualAddress(qwFileOffset);
	}
	return qwOffset;
}

BOOL HexEditWnd::PaintText(HWND hWnd) {
	HDC           hDC, hdcBuff;
	ULONGLONG     qwOffset;
	UINT          u, u2, icySel;
	char          cBuff[40];
	char          cOffBuff[24];
	BYTE          byCur, byNext;
	RECT          rct;
	HBITMAP       hBmp;
	HBRUSH        hbrColor;
	HGDIOBJ       hobjOld;
	PAINTSTRUCT   ps;
	BOOL          bSkipOneByte = FALSE;

	hDC      = BeginPaint(hWnd, &ps);

	// create virtual bmp
	GetClientRect(hWnd, &rct);
	hdcBuff  = CreateCompatibleDC(hDC);
	hBmp     = CreateCompatibleBitmap(hDC,
			rct.right - rct.left,
			rct.bottom - rct.top - iyHETop);
	hobjOld = SelectObject(hdcBuff, hBmp);

	// fill bmp
	hbrColor = CreateSolidBrush( (COLORREF)0x00FFFFFF );
	FillRect(hdcBuff, &rct, hbrColor);
	DeleteObject(hbrColor);

	if (!diData.qwSize)
		goto Exit; // ERR

	// paint to bmp
	qwOffset = stat.qwCurOffset;

	for (u = 0; u < uMaxLines; u++) {
		// end of buffer ?
		if (qwOffset >= diData.qwSize)
			break;

		// paint offset (8 digits below 4GB, 16 digits above)
		SetTextColor(hdcBuff, RGB_BLACK);
		FormatOffset64(cOffBuff, GetOffset(qwOffset));
		wsprintf(cBuff, "%s:", cOffBuff);
		SelectObject(hdcBuff, hFont);
		TextOut(hdcBuff, LEFT_OFFSET, u * uFontHeight + TOP_OFFSET, cBuff, lstrlen(cBuff));

		// paint digit pairs
		for (u2 = 0; u2 < 16; u2++) {
			// end of buffer ?
			if (qwOffset >= diData.qwSize)
				break; // upper, same structured decision handles painting end

			//
			// change at this position?
			//
			byCur = *(diData.pDataBuff + qwOffset);

			// next byte may be multibyte
			if (qwOffset + 1 < diData.qwSize) {
				byNext = *(diData.pDataBuff + qwOffset + 1);
			} else {
				byNext = 0;
			}

			switch (GetDataStatus(qwOffset)) {
				case 0:
					SetTextColor(hdcBuff, RGB_BLACK);
					break;
				case 1:
					SetTextColor(hdcBuff, RGB_RED);
					break;
				case 2:
					SetTextColor(hdcBuff, RGB_BLUE);
					break;
				case 3:
					SetTextColor(hdcBuff, RGB_GREEN);
					break;
			}

			//
			// paint digit pair
			//
			if (stat.bCaretVisible &&
				stat.posCaret.qwOffset == qwOffset &&
				stat.posCaret.bTextSection)
				SelectObject(hdcBuff, hFontU);
			else
				SelectObject(hdcBuff, hFont);

			wsprintf(cBuff, H2, byCur);
			TextOut(hdcBuff,
					PAIRS_X + DIGIT_PAIR_WIDTH * u2, u * uFontHeight + TOP_OFFSET,
					cBuff, 2);

			//
			// paint character
			//
			if (stat.bCaretVisible &&
				stat.posCaret.qwOffset == qwOffset &&
				!stat.posCaret.bTextSection)
				SelectObject(hdcBuff, hFontU);
			else
				SelectObject(hdcBuff, hFont);

			if (!bSkipOneByte) {
				if (IsDBCSLeadByte(byCur) && bDispMultiByte) {
					if (qwOffset == stat.qwCurOffset && !IsDBCSFirstByte(qwOffset)) {
						lstrcpy(cBuff, " ");
					} else {
						if (byNext != 0) {
							bSkipOneByte = TRUE;
						}
						cBuff[0] = byCur;
						cBuff[1] = byNext;
						cBuff[2] = 0;
					}
				} else if (byCur < 0x20 || (byCur & 0x80)) {
					lstrcpy(cBuff, ".");
				} else {
					wsprintf(cBuff, "%c", byCur);
				}

				TextOut(hdcBuff,
						CHARS_X + u2 * uFontWidth, u * uFontHeight + TOP_OFFSET,
						cBuff, strlen(cBuff));
			} else {
				bSkipOneByte = FALSE;
			}

			//
			// draw sel
			//
			if (stat.bSel &&
				qwOffset >= stat.qwOffSelStart &&
				qwOffset <= stat.qwOffSelEnd) {
				if (qwOffset == stat.qwOffSelEnd ||
					qwOffset % 16 == 15)
					icySel = uFontWidth * 2 + 2;
				else
					icySel = DIGIT_PAIR_WIDTH;

				// sel pair
				BitBlt(hdcBuff,
					   PAIRS_X + u2 * DIGIT_PAIR_WIDTH - 2,
					   u * uFontHeight + TOP_OFFSET,
					   icySel,
					   uFontHeight,
					   hdcBuff,
					   PAIRS_X + u2 * DIGIT_PAIR_WIDTH - 2,
					   u * uFontHeight + TOP_OFFSET,
					   NOTSRCCOPY);

				// sel char
				BitBlt(hdcBuff,
					   CHARS_X + u2 * uFontWidth - 2,
					   u * uFontHeight + TOP_OFFSET,
					   uFontWidth,
					   uFontHeight,
					   hdcBuff,
					   CHARS_X + u2 * uFontWidth - 2,
					   u * uFontHeight + TOP_OFFSET,
					   NOTSRCCOPY);
			}

			// adjust vars
			++qwOffset;
		}
	}

	//
	// SB stuff
	//

Exit:
	// paint SB text
	SetTextColor(hdcBuff, RGB_BLACK);
	SelectObject(hdcBuff, hFont);
	SetStatusText();
	MoveToEx(hdcBuff, rct.left + 4, rct.bottom - SB_HEIGHT - 30, NULL);
	LineTo(hdcBuff, CHARS_X + 16 * uFontWidth + 8, rct.bottom - SB_HEIGHT - 30);
	LineTo(hdcBuff, CHARS_X + 16 * uFontWidth + 8, rct.top);
	LineTo(hdcBuff, rct.left + 4, rct.top);
	LineTo(hdcBuff, rct.left + 4, rct.bottom - SB_HEIGHT - 30);
	TextOut(hdcBuff, rct.left + 4, rct.bottom - SB_HEIGHT - 28, cSBText, lstrlen(cSBText) );

	// copy buffer content to client area
	BitBlt(hDC,
		   rct.left,
		   rct.top + iyHETop,
		   rct.right - rct.left,
		   rct.bottom - rct.top - iyHETop,
		   hdcBuff,
		   0, 0, SRCCOPY);

	// cleanup
	SelectObject(hdcBuff, hobjOld);
	DeleteDC(hdcBuff);
	DeleteObject(hBmp);

	EndPaint(hWnd, &ps);

	return TRUE; // OK
}

void HexEditWnd::HEHandleWM_SETFOCUS(HWND hWnd) {
	CreateCaret(hWnd, NULL, uFontWidth, uFontHeight);
	SetCaretSet(TRUE);
	if (stat.bCaretPosValid)
		SetCaret(&stat.posCaret);

	return;
}

void HexEditWnd::HEHandleWM_KILLFOCUS(HWND hWnd) {
	if (!stat.bCaretVisible)
		HideCaret(hWnd);
	DestroyCaret();
	SetCaretSet(FALSE);
	stat.bCaretVisible = FALSE;

	return;
}

//
// save new caret pos and repaints
//
// returns:
// FALSE - mainly if the caret was hidden because it's not in the current visible range
//
BOOL HexEditWnd::SetCaret(PHE_POS ppos)
{
	BOOL     bRet = FALSE;
	ULONGLONG qwOffDelta;
	UINT     uxPair, uyLine, ux;

	if (IsOutOfRange(ppos))
		return FALSE; // ERR

	SetCaretPosData(ppos);

	if (stat.bSel)
		return FALSE; // ERR

	// new pos in current range?
	if (!IsOffsetVisible(ppos->qwOffset)) {
		if (stat.bCaretVisible)
			HideCaret(hMainWnd);
		stat.bCaretVisible = FALSE;
		return FALSE; // ERR
	}

	qwOffDelta = ppos->qwOffset - stat.qwCurOffset;
	uyLine = (UINT)(qwOffDelta / 16);
	uxPair = (UINT)(qwOffDelta % 16);

	// caret in the text section ?
	if (ppos->bTextSection) {
		bRet = SetCaretPos(
						  CHARS_X + uFontWidth * uxPair,
						  uyLine * uFontHeight + iyHETop + TOP_OFFSET);
	} else {
		ux = PAIRS_X + uxPair * DIGIT_PAIR_WIDTH;
		if (!ppos->bHiword)
			ux += uFontWidth;
		bRet = SetCaretPos(
						  ux,
						  uyLine * uFontHeight + iyHETop + TOP_OFFSET);
	}
	if (bRet) {
		if (!stat.bCaretVisible)
			ShowCaret(hMainWnd);
		stat.bCaretVisible   = TRUE;
		stat.bCaretPosValid  = TRUE;

		RepaintClientArea();

		return TRUE; // OK
	} else
		return FALSE; // ERR
}

//
// overloaded
//
BOOL HexEditWnd::SetCaret(ULONGLONG qwOffset)
{
	HE_POS  posNew;

	if (IsOutOfRange(qwOffset))
		return FALSE; // ERR

	posNew.bHiword      = TRUE;
	posNew.bTextSection = FALSE;
	posNew.qwOffset     = qwOffset;

	return SetCaret(&posNew);
}

BOOL HexEditWnd::SetCaret() {
	return SetCaret(&stat.posCaret);
}

//
// checks whether an Offset is currently visible in the GUI
//
BOOL HexEditWnd::IsOffsetVisible(ULONGLONG qwOffset)
{
	ULONGLONG qwBytes2C;

	// out of mem range ?
	if (qwOffset >= diData.qwSize)
		return FALSE; // ERR

	// out of visible range ?
	qwBytes2C = (ULONGLONG)16 * uMaxLines;
	if (qwBytes2C + stat.qwCurOffset > diData.qwSize)
		qwBytes2C = diData.qwSize - stat.qwCurOffset;

	if (qwOffset >= stat.qwCurOffset &&
		qwOffset <  stat.qwCurOffset + qwBytes2C)
		return TRUE; // OK
	else
		return FALSE; // ERR
}

BOOL HexEditWnd::HESetFont(HFONT hf) {
	TEXTMETRIC  tm;
	HDC         hDC;
	BOOL        bRet = FALSE;

	hDC = GetDC(hMainWnd);
	if (!hDC)
		return FALSE; // FAILURE
	SelectObject(hDC, hf);
	if (!GetTextMetrics(hDC, &tm))
		goto Exit; // FAILURE

	uFontHeight = tm.tmHeight;
	uFontWidth  = tm.tmAveCharWidth;

	SendMessage(hMainWnd, WM_SETFONT, (WPARAM)HEdit.hFont, 0);

	Exit:
	ReleaseDC(hMainWnd, hDC);

	return bRet; // EXIT
}

void HexEditWnd::HEHandleWM_SIZE(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	RECT              rct;
	UINT              uWidth, uHeight;

	if ( wParam != SIZE_MINIMIZED &&
		 wParam != SIZE_MAXIMIZED)
		GetWindowRect( hWnd, &rctLastPos );

	if (bMinToTray && wParam == SIZE_MINIMIZED) {
		HEditToTray();
		return;
	}

	// wnd size info -> vars
	uWidth = LOWORD(lParam);
	uHeight = HIWORD(lParam); 

	if (uFontHeight) { // avoid division through 0
		// calc max lines
		uMaxLines = (uHeight - iyHETop - SB_HEIGHT) / uFontHeight;

		// bottom of HE
		GetClientRect(hWnd, &rct);

		HEdit.iyHEBottom = rct.bottom - SB_HEIGHT;

		// get HE rect
		rctHE.top     = iyHETop;
		rctHE.bottom  = iyHEBottom;
		rctHE.left    = 0;
		rctHE.right   = CHARS_X + 16 * uFontWidth;
	}

	// resize TB
	SendMessage(hTB, TB_AUTOSIZE, 0, 0);
	return;	// RET
}

BOOL HexEditWnd::HEHandleWM_TIMER(HWND hWnd, WPARAM wTimerId) {
	POINT pos;
	if (wTimerId == SELECT_TIMER) {
		if (IsKeyDown(VK_LBUTTON)) {
			GetCursorPos(&pos);
			ScreenToClient(hWnd, &pos);
			MouseMoveSelect(&pos);
		} else {
			KillTimer(hWnd, SELECT_TIMER);
			timerId = 0;
		}
		return 0;
	}
	return TRUE;
}

BOOL HexEditWnd::HEHandleLButton(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT   pClick;
	HE_POS  pos;
	static  ULONGLONG qwOldOff;
	static  BOOL	lastvalid = FALSE;

	pClick.x = LOWORD(lParam);
	pClick.y = HIWORD(lParam);

	switch (uMsg) {
	case WM_LBUTTONDOWN:
		if (!PointToPos(&pClick, &pos)) {
			stat.bLastLBDownPos = FALSE;
			return FALSE; // ERR
		}

		stat.bMouseSelecting = TRUE;
		if (!stat.bSel || lastvalid == FALSE) {
			lastvalid = TRUE;
			qwOldOff = stat.posCaret.qwOffset;
		}
		stat.posLastLButtonDown = pos;
		stat.bLastLBDownPos = TRUE;

		// handle shift key actions ?
		if ( TESTBIT(wParam, MK_SHIFT) && stat.bCaretPosValid )
			SetSelection( qwOldOff, pos.qwOffset );
		else {
			lastvalid = FALSE;
			if (stat.bSel) {
				KillSelection();
			}
		}
		SetCaret(&pos);
		break;

	case WM_LBUTTONUP:
		KillTimer(hMainWnd, SELECT_TIMER);
		timerId = 0;
		stat.bMouseSelecting = FALSE;
		break;
	}

	return TRUE;
}

BOOL HexEditWnd::PointToPos(IN POINT *pp, OUT PHE_POS ppos) {
	UINT  uLine, uPair, uxCurPair;

	memset(ppos, 0, sizeof(HE_POS));

	// in digit pair field ?
	if ((DWORD)pp->x >= PAIRS_X &&
		(DWORD)pp->x <  PAIRS_X + 16 * DIGIT_PAIR_WIDTH &&
		(DWORD)pp->y >= iyHETop &&
		(DWORD)pp->y <  iyHETop + uMaxLines * uFontHeight) {
		uLine =  ((DWORD)pp->y - iyHETop) / uFontHeight;
		uPair =  pp->x - PAIRS_X;
		uPair /= DIGIT_PAIR_WIDTH;

		// x in space between digit pairs ?
		uxCurPair = PAIRS_X + uPair * DIGIT_PAIR_WIDTH;	// -> x pos of cur pair
		if ((UINT)pp->x > uxCurPair + 2*uFontWidth) {
			// last pair of the line ?
			if (uPair == 0xF)
				return FALSE; // ERR

			// autosel next pair
			if (pp->x - uxCurPair - 2*uFontWidth > uFontWidth / 2) {
				++uPair;
				uxCurPair += DIGIT_PAIR_WIDTH;
			}
		}

		// out of range?
		if (IsOutOfRange(stat.qwCurOffset + uLine * 16 + uPair))
			return FALSE; // ERR		

		// x -> LOWORD ?
		ppos->bHiword = ((UINT)pp->x > uxCurPair + uFontWidth) ? FALSE: TRUE;

		// save offset
		ppos->qwOffset = stat.qwCurOffset + uLine * 16 + uPair;

		return TRUE; // OK
	}
	// in text field ?
	else if ((DWORD)pp->x >= CHARS_X &&
			 (DWORD)pp->x <  CHARS_X + uFontWidth * 16 &&
			 (DWORD)pp->y >= iyHETop &&
			 (DWORD)pp->y <  iyHETop + uMaxLines * uFontHeight) {
		uLine =  ((DWORD)pp->y - iyHETop) / uFontHeight;
		uPair =  (UINT)pp->x - CHARS_X;
		uPair /= uFontWidth;

		// out of range?
		if (IsOutOfRange(stat.qwCurOffset + uLine * 16 + uPair))
			return FALSE; // ERR

		// build output
		ppos->bTextSection = TRUE;
		ppos->bHiword      = TRUE;
		ppos->qwOffset     = stat.qwCurOffset + uLine * 16 + uPair;

		return TRUE; // OK
	} else
		return FALSE; // ERR
}

BOOL HexEditWnd::HEHandleWM_KEYDOWN(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HE_POS  posNew;
	LONGLONG llNewLine;
	ULONGLONG qwOff;

	if (wParam == VK_ESCAPE) {
		if (stat.bSel) {
			KillSelection();
			RepaintClientAreaNow();
		} else {
			SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}
		return TRUE;
	}

	// caret already set
	if (!stat.bCaretPosValid)
		return FALSE; // ERR

	// handle special keys

	// tab key
	switch (wParam) {
	case VK_TAB:
		// switch between text/data fields
		if (stat.bCaret) {
			// kill sel
			if (stat.bSel)
				KillSelection();

			// reset caret
			stat.posCaret.bTextSection ^= TRUE;
			stat.posCaret.bHiword       = TRUE;
			SetCaret();

			// make visible if is not
			if (!IsOffsetVisible(stat.posCaret.qwOffset))
				SetTopLine();

			return TRUE; // OK
		} else
			return FALSE; // ERR
	}

	// shift key
	if ( IsKeyDown(VK_SHIFT) ) {
		posNew = stat.posCaret;
		switch (wParam) {
		case VK_LEFT:	 // left
			posNew.qwOffset--;
			break;

		case VK_RIGHT:	 // right
			posNew.qwOffset++;
			break;

		case VK_NEXT:	 // page down
			posNew.qwOffset += uMaxLines * 16;
			break;

		case VK_PRIOR:	 // page up
			posNew.qwOffset -= uMaxLines * 16;
			break;

		case VK_DOWN:	 // down
			posNew.qwOffset += 16;
			break;

		case VK_UP:		 // up
			posNew.qwOffset -= 16;
			break;

		default:
			return FALSE; // ERR
		}

		// validate
		if (ValidatePos(&posNew))
			Beep();

		if (stat.bSel) {
			if (stat.qwOffSelStart == stat.posCaret.qwOffset)
				qwOff = stat.qwOffSelEnd;
			else
				qwOff = stat.qwOffSelStart;

			SetSelection( qwOff, posNew.qwOffset);
		} else
			SetSelection( stat.posCaret.qwOffset, posNew.qwOffset );

		SetCaret(&posNew);
		MakeCaretVisible();

		return TRUE; // OK
	}

	//
	// move caret in HE area / move current line
	//
	posNew    = stat.posCaret;
	llNewLine  = (LONGLONG)GetCurrentLine();
	switch (wParam) {
	case VK_NEXT:	 // page down
		posNew.qwOffset += uMaxLines * 16;
		llNewLine += uMaxLines;
		break;

	case VK_PRIOR:	 // page up
		posNew.qwOffset -= uMaxLines * 16;
		llNewLine -= uMaxLines;
		break;

	case VK_DOWN:	 // down
		posNew.qwOffset += 16;
		++llNewLine;
		break;

	case VK_UP:		 // up
		posNew.qwOffset -= 16;
		--llNewLine;
		break;

	case VK_RIGHT:	 // rigth
		if (stat.posCaret.bTextSection) {
			++posNew.qwOffset;
			posNew.bHiword = TRUE;
		} else {
			posNew.bHiword ^= 1;    
			if (!stat.posCaret.bHiword)
				++posNew.qwOffset;
		}
		break;

	case VK_LEFT:	 // left
		if (stat.posCaret.bTextSection) {
			--posNew.qwOffset;
			posNew.bHiword = TRUE;
		} else {
			posNew.bHiword ^= 1;
			if (stat.posCaret.bHiword)
				--posNew.qwOffset;
		}
		break;

	case VK_BACK:
		if (stat.posCaret.bTextSection)
			--posNew.qwOffset;
		break;

	default:
		return FALSE; // ERR
	}

	// changes -> GUI
	if (stat.bSel &&
		wParam != VK_RIGHT && 
		wParam != VK_LEFT) {
		// validate
		if (ValidateLine(&llNewLine))
			Beep();

		// set
		SetCurrentLine((ULONGLONG)llNewLine);
	} else {
		// validate
		if (ValidatePos(&posNew))
			Beep();

		// set
		KillSelection();
		SetCaret(&posNew);
		MakeCaretVisible();
	}

	return TRUE; // OK
}

//
// if caret isn't in the visible area, the top line is reset
//
// returns:
// whether it was needed to reset the top line
//
BOOL HexEditWnd::MakeCaretVisible() {
	ULONGLONG qwLastVisibleOff;
	ULONGLONG qwLine;

	if (IsOffsetVisible(stat.posCaret.qwOffset))
		return FALSE; // ERR

	qwLastVisibleOff = __min((ULONGLONG)uMaxLines * 16 + stat.qwCurOffset,
							 diData.qwSize);
	if (stat.posCaret.qwOffset < stat.qwCurOffset) // caret above ?
		qwLine = stat.posCaret.qwOffset / 16;
	else // caret below
		qwLine = stat.posCaret.qwOffset / 16 > uMaxLines ? stat.posCaret.qwOffset / 16 - uMaxLines + 1 : 0;

	if (qwLine > (ULONGLONG)INT_MAX)
		qwLine = INT_MAX;
	SetTopLine((int)qwLine);

	return TRUE; // OK
}

//
// corrects the information in a given HE_POS structure if it's out of range
//
// returns:
// whether sth was fixed
//
BOOL HexEditWnd::ValidatePos(PHE_POS ppos) {
	if (diData.qwSize == 0) {
		ppos->qwOffset = 0;
		ppos->bHiword  = TRUE;
		return TRUE; // OK
	}
	if ((LONGLONG)ppos->qwOffset < 0) {
		// underflow (e.g. moved up from 0): clamp to start
		ppos->qwOffset = 0;
		ppos->bHiword  = TRUE;
		return TRUE; // OK
	} else if (ppos->qwOffset >= diData.qwSize) {
		ppos->qwOffset = diData.qwSize - 1;
		ppos->bHiword  = FALSE;
		return TRUE; // OK
	} else
		return FALSE; // ERR
}

//
// returns:
// last caret status
//
BOOL HexEditWnd::SetCaretSet(BOOL bSet) {
	BOOL bRet;

	bRet = stat.bCaret;
	stat.bCaret = bSet;

	return bRet;
}

void HexEditWnd::SetupVScrollbar() {
	ULONGLONG qwTotalLines = GetTotalLineNum();
	LONGLONG llMax;

	// Win32 scrollbars take int; clamp for huge files (>~34GB).
	// Files >4GB (up to ~268M lines) fit fine.
	if (qwTotalLines == 0)
		llMax = 0;
	else if (qwTotalLines - 1 > (ULONGLONG)INT_MAX)
		llMax = INT_MAX;
	else
		llMax = (LONGLONG)(qwTotalLines - 1);

	SetScrollRange(hMainWnd, SB_VERT, 0, (int)llMax, TRUE);
	return;
}

ULONGLONG HexEditWnd::GetTotalLineNum() {
	ULONGLONG qwTotalLines;

	qwTotalLines = diData.qwSize / 16;
	if (diData.qwSize % 16)
		++qwTotalLines;

	return qwTotalLines;
}

BOOL HexEditWnd::HEHandleWM_VSCROLL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int          nPos;
	SCROLLINFO   scrollinfo = { sizeof(SCROLLINFO)};

	nPos = GetScrollPos(hWnd, SB_VERT);

	switch (LOWORD(wParam)) {
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		// get the tracking position
		scrollinfo.fMask = SIF_TRACKPOS;
		GetScrollInfo(hWnd, SB_VERT, &scrollinfo);
		nPos = scrollinfo.nTrackPos;
		break;

	case SB_TOP:
		nPos = 0;
		break;

	case SB_BOTTOM:
		{
			ULONGLONG qwTotal = GetTotalLineNum();
			if (qwTotal == 0)
				nPos = 0;
			else if (qwTotal - 1 > (ULONGLONG)INT_MAX)
				nPos = INT_MAX;
			else
				nPos = (int)(qwTotal - 1);
		}
		break;

	case SB_LINEDOWN:
		++nPos;
		break;

	case SB_LINEUP:
		--nPos;
		break;

	case SB_PAGEDOWN:
		nPos += uMaxLines;      
		break;

	case SB_PAGEUP:
		nPos -= uMaxLines;
		break;      
	}

	return SetTopLine((int)nPos);
}

//
// sets the line to the top of the visible data range
//
BOOL HexEditWnd::SetTopLine(int iNewLine) {
	LONGLONG llNewPos;
	ULONGLONG qwTotal;
	LONGLONG llMaxLine;

	llNewPos = iNewLine;

	// validation (64-bit aware, clamped to scrollbar int range)
	qwTotal = GetTotalLineNum();
	if (qwTotal == 0)
		llMaxLine = 0;
	else if (qwTotal - 1 > (ULONGLONG)INT_MAX)
		llMaxLine = INT_MAX;
	else
		llMaxLine = (LONGLONG)(qwTotal - 1);

	if (llNewPos < 0)
		llNewPos = 0;
	if (llNewPos > llMaxLine)
		llNewPos = llMaxLine;

	// avoid repainting ?
	if (llNewPos == stat.llLastLine)
		return TRUE; // OK

	// set new line
	SetScrollPos(hMainWnd, SB_VERT, (int)llNewPos, TRUE);
	stat.llLastLine = llNewPos;

	// set new offset
	stat.qwCurOffset = (ULONGLONG)llNewPos * 16;

	// reset caret
	if (stat.bCaretPosValid)
		SetCaret();

	// repaint
	RepaintClientArea();

	return TRUE; // OK
}

BOOL HexEditWnd::SetTopLine(ULONGLONG qwOffset) {
	ULONGLONG qwLine = qwOffset / 16;
	if (qwLine > (ULONGLONG)INT_MAX)
		return SetTopLine(INT_MAX);
	return SetTopLine((int)qwLine);
}

BOOL HexEditWnd::SetTopLine() {
	return SetTopLine(stat.posCaret.qwOffset);
}

void HexEditWnd::RepaintClientArea() {
	InvalidateRect(hMainWnd, NULL, FALSE);
	return;
}

void HexEditWnd::RepaintClientAreaNow() {
	RepaintClientArea();
	UpdateWindow(hMainWnd);

	return;
}

void HexEditWnd::HEHandleWM_MOUSEWHEEL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int         nPos;
	short int   iDelta;

	iDelta = (int)HIWORD(wParam);
	nPos = GetScrollPos(hWnd, SB_VERT);
	if (iDelta > 0) { // move up	
		nPos -= WHEEL_MOVE_STEPS;
	} else { // move down
		nPos += WHEEL_MOVE_STEPS;
	}

	SetTopLine((int)nPos);

	return;
}

void HexEditWnd::HEHandleWM_SHOWWINDOW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HEdit.HESetFont(HEdit.hFont);
	SetupVScrollbar();
	SetTopLine(stat.qwCurOffset);
	ConfigureTB();
	SetHEWndCaption();

	return;
}

void HexEditWnd::Beep() {
	MessageBeep(MB_ICONEXCLAMATION);
	return;
}

BOOL HexEditWnd::HEHandleWM_COMMAND(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	OFN          ofn;
	char         *szCmdl;
	int          iRet;

	switch (LOWORD(wParam)) {
	case TB_GOTO:
		if (DialogBoxParam( GetInstance(), (PSTR)IDD_GOTO, hWnd, GotoDlgProc, 0) &&
			stat.bCaretPosValid) {
			SetTopLine();
			SetCaret(&stat.posCaret);
		}
		return TRUE;

	case TB_OPTION:
		DialogBoxParam( GetInstance(), (PSTR)IDD_OPTION, hWnd, OptionDlgProc, 0);
		return TRUE;

	case TB_SELBLOCK:
		DialogBoxParam( GetInstance(), (PSTR)IDD_SELBLOCK, hWnd, SelBlockDlgProc, 0);
		return TRUE;

	case TB_SELALL:
		SelectAll();
		return TRUE;

	case TB_REPLACE:
		DialogBoxParam( GetInstance(), (PSTR)IDD_REPLACE, hWnd, ReplaceDlgProc, 0);
		return TRUE;

	case TB_SEARCH:
		DialogBoxParam( GetInstance(), (PSTR)IDD_SEARCH, hWnd, SearchDlgProc, 0);
		return TRUE;

	case TB_SEARCHDOWN:
	case TB_SEARCHUP:
		PerformSearchAgain(&search, LOWORD(wParam) == TB_SEARCHDOWN ? TRUE : FALSE);
		return TRUE;

	case TB_WIN2TOP:
		SetHEWnd2Top( bHEOnTop ^ 1 );
		ConfigureTB();
		return TRUE;

	case TB_SAVE:
		SaveChanges();
		SetHEWndCaption();
		ConfigureTB();
		RepaintClientAreaNow();
		return TRUE;

	case TB_OFFSET:
		bFileOffset = !bFileOffset;
		ConfigureTB();
		RepaintClientAreaNow();
		return TRUE;

	case TB_INSERT:
		bInsert = !bInsert;
		ConfigureTB();
		return TRUE;

	case TB_UNDO:
		UndoChanges();
		SetHEWndCaption();
		return TRUE;

	case TB_REDO:
		RedoChanges();
		SetHEWndCaption();
		return TRUE;

	case TB_DELETE:
		DeleteSelectedBlock();
		SetHEWndCaption();
		return TRUE;

	case TB_CUT:
		CutSelectedBlock();
		SetHEWndCaption();
		return TRUE;

	case TB_COPY:
		CopySelectedBlock();
		return TRUE;

	case TB_COPY_TEXT:
		CopySelectedBlockAsText();
		return TRUE;

	case TB_PASTE:
		PasteBlockFromCB();
		SetHEWndCaption();
		return TRUE;

	case TB_ABOUT:
		ShowAbout();
		return TRUE;

	case TB_CLOSE:
		HEditQuit();
		return TRUE;

	case IDT_RESTORE:
		HEditReturnFromTray();
		return TRUE;

	case IDT_EXIT:
		HEditKillTrayIcon();
		HEditQuit();
		return TRUE;

	case TB_MULTI:
		bDispMultiByte = !bDispMultiByte;
		ConfigureTB();
		RepaintClientAreaNow();
		return TRUE;

	case TB_READONLY:
		if (!diOrgData.bReadOnly) {
			bReadOnly = !bReadOnly;
			ConfigureTB();
			SetHEWndCaption();
		}
		return TRUE;
	case TB_SIZE:
		bResizingAllowed = !bResizingAllowed;
		ConfigureTB();
		return TRUE;
	case TB_OPEN:
		if (CanSave()) {
			iRet = MessageBox(hMainWnd, "File changed, save or not", "16Edit", MB_YESNOCANCEL);
			if (iRet == IDYES) {
				SaveChanges();
			} else if (iRet == IDCANCEL) {
				return TRUE;
			}
		}

		if (ofn.GetOpenFilePath()) {
			szCmdl = ofn.cPathOpen;

			QuitEdition();
			if (!HEdit.DoSpecifySettings(szCmdl)) {
				MessageBox(hMainWnd, "File access error!", "16Edit", MB_ICONERROR);
				return TRUE;
			}
			SetCaret();
			SetHEWndCaption();
			SetupVScrollbar();
			SetTopLine();
			ConfigureTB();
			RepaintClientAreaNow();
		}
		return TRUE;
	case TB_REFRESH:
		if (CanSave()) {
			iRet = MessageBox(hMainWnd, "File changed, save or not", "16Edit", MB_YESNOCANCEL);
			if (iRet == IDYES) {
				SaveChanges();
			} else if (iRet == IDCANCEL) {
				return TRUE;
			}
		}

		szCmdl = fInput.GetFilePath();

		QuitEdition();
		if (!HEdit.DoSpecifySettings(szCmdl)) {
			MessageBox(hMainWnd, "File access error!", "16Edit", MB_ICONERROR);
			return TRUE;
		}
		SetCaret();
		SetHEWndCaption();
		SetupVScrollbar();
		SetTopLine();
		ConfigureTB();
		RepaintClientAreaNow();
		return TRUE;
	}

	return FALSE; // ERR
}

BOOL HexEditWnd::HEHandleWM_NOTIFY(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (((LPNMHDR)lParam)->code == TTN_NEEDTEXT) {
		switch (((LPNMHDR) lParam)->idFrom) {
		case TB_SAVE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "save changes  ( CTRL+S )";
			return TRUE;

		case TB_UNDO:
			((LPTOOLTIPTEXT)lParam)->lpszText = "undo last changes  ( CTRL+Z )";
			return TRUE;

		case TB_REDO:
			((LPTOOLTIPTEXT)lParam)->lpszText = "undo last changes  ( CTRL+Y )";
			return TRUE;

		case TB_GOTO:
			((LPTOOLTIPTEXT)lParam)->lpszText = "goto offset  ( CTRL+G )";
			return TRUE;

		case TB_SELBLOCK:
			((LPTOOLTIPTEXT)lParam)->lpszText = "select block  ( CTRL+B )";
			return TRUE;

		case TB_SELALL:
			if ( IsAllSelected() )
				((LPTOOLTIPTEXT)lParam)->lpszText = "deselect all  ( CTRL+A )";
			else
				((LPTOOLTIPTEXT)lParam)->lpszText = "select all  ( CTRL+A )";
			return TRUE;

		case TB_REPLACE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "replace  ( CTRL+R )";
			return TRUE;

		case TB_SEARCH:
			((LPTOOLTIPTEXT)lParam)->lpszText = "search  ( CTRL+F )";
			return TRUE;

		case TB_SEARCHDOWN:
			((LPTOOLTIPTEXT)lParam)->lpszText = "search down again  ( F3 )";
			return TRUE;

		case TB_SEARCHUP:
			((LPTOOLTIPTEXT)lParam)->lpszText = "search up again  ( CTRL+F3 )";
			return TRUE;

		case TB_DELETE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "delete selected block( DELETE )";
			return TRUE;

		case TB_CUT:
			((LPTOOLTIPTEXT)lParam)->lpszText = "cut into clipboard  ( CTRL+X )";
			return TRUE;

		case TB_COPY:
			((LPTOOLTIPTEXT)lParam)->lpszText = "copy to clipboard  ( CTRL+C )";
			return TRUE;

		case TB_PASTE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "paste from clipboard  ( CTRL+V )";
			return TRUE;

		case TB_WIN2TOP:
			((LPTOOLTIPTEXT)lParam)->lpszText =
			bHEOnTop ? "set window state to non-top" : "set window state to top  ( CTRL+T )";
			return TRUE;

		case TB_ABOUT:
			((LPTOOLTIPTEXT)lParam)->lpszText = "about  ( F12 )";
			return TRUE;

		case TB_MULTI:
			((LPTOOLTIPTEXT)lParam)->lpszText = "ansi or native language  ( CTRL+D )";
			return TRUE;

		case TB_READONLY:
			((LPTOOLTIPTEXT)lParam)->lpszText = "readonly or readwrite ( CTRL+W )";
			return TRUE;

		case TB_INSERT:
			((LPTOOLTIPTEXT)lParam)->lpszText = "insert or overwrite ( CTRL+I )";
			return TRUE;

		case TB_SIZE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "size change allowed or not ( CTRL+L )";
			return TRUE;

		case TB_OFFSET:
			((LPTOOLTIPTEXT)lParam)->lpszText = "file offset or virtual address ( CTRL+E )";
			return TRUE;

		case TB_OPTION:
			((LPTOOLTIPTEXT)lParam)->lpszText = "16edit options ( CTRL+O )";
			return TRUE;

		case TB_CLOSE:
			((LPTOOLTIPTEXT)lParam)->lpszText = "close this window  ( ESC )";
			return TRUE;
		}
	}

	return FALSE;
}

void HexEditWnd::ErrMsg(HWND hWnd, char* szText) {
	MessageBox(hWnd, szText, "ERROR", MB_ICONERROR);
	return;
}

void HexEditWnd::ErrMsg(char* szText) {
	MessageBox(hMainWnd, szText, "ERROR", MB_ICONERROR);
	return;
}

void HexEditWnd::ErrMsg(HWND hWnd, char* szText, char* szCaption) {
	MessageBox(hWnd, szText, szCaption, MB_ICONERROR);
	return;
}

BOOL HexEditWnd::IsOutOfRange(ULONGLONG qwOffset) {
	return(qwOffset >= diData.qwSize) ? TRUE : FALSE;
}

BOOL HexEditWnd::IsOutOfRange(PHE_POS ppos) {
	return(ppos->qwOffset >= diData.qwSize) ? TRUE : FALSE;
}

void HexEditWnd::SetCaretPosData(PHE_POS ppos) {
	memcpy( &stat.posCaret, ppos, sizeof(HE_POS));
	stat.bCaretPosValid = TRUE;

	return;
}

void HexEditWnd::SetCaretPosData(ULONGLONG qwOffset) {
	HE_POS  posNew;

	posNew.bHiword       = TRUE;
	posNew.bTextSection  = FALSE;
	posNew.qwOffset      = qwOffset;
	SetCaretPosData(&posNew);

	return;
}

//
// returns:
// 0 - if the message was processed
//
LRESULT HexEditWnd::HEHandleWM_CHAR(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	BYTE       byNew, byMask, byOld;
	BOOL	   hiWord;

	// caret set ?
	if (!stat.bCaretVisible ||
		!stat.bCaretPosValid)
		return 1; // ERR

	// skip key combinations (CTRL + Z, ...)
	if (wParam < 0x20)
		return 1;

	if (IsReadOnly()) {
		Beep();
		return 1;
	}

	hiWord = stat.posCaret.bHiword;

	// process HE change
	byOld = *(diData.pDataBuff + stat.posCaret.qwOffset);
	if (stat.posCaret.bTextSection) {
		byNew = (BYTE)wParam;
	} else {
		// build new byte
		if (wParam >= '0' && wParam <= '9')
			byMask = wParam - 0x30;
		else if (wParam >= 'a' && wParam <= 'f')
			byMask = wParam - 87;
		else if (wParam >= 'A' && wParam <= 'F')
			byMask = wParam - 55;
		else
			return 1; // ERR

		if (hiWord)
			byNew = (byOld & 0x0F) + byMask * 16;
		else
			byNew = (byOld & 0xF0) + byMask;
	}

	if (byOld != byNew) {
		HE_OPER *op;

		op = current->getOper();
		if (current->getNext() == NULL &&
			op != NULL &&
			op->type == op_modify &&
			(!bSavePointValid || current != savepoint) &&
			op->qwOffset == stat.posCaret.qwOffset) {
			op = current->getOper();
			op->newData[0] 	= byNew;
		} else {
			op = new HE_OPER(op_modify, stat.posCaret.qwOffset, 1, 1);
			op->newData[0] 	= byNew;
			op->oldData[0] 	= byOld;
			AddOper(op);
		}

		ApplyOper(op);
	}

	// reset caret
	if (stat.posCaret.bTextSection) {
		++stat.posCaret.qwOffset;
	} else {
		if (!hiWord) {
			++stat.posCaret.qwOffset;
			stat.posCaret.bHiword = TRUE;
		} else {
			stat.posCaret.bHiword = FALSE;
		}
	}

	SetHEWndCaption();
	ConfigureTB();
	ValidatePos(&stat.posCaret);
	SetCaret();

	return 0; // OK
}

#define STATUS (bEnabled ? TBSTATE_ENABLED:TBSTATE_INDETERMINATE)
void HexEditWnd::ConfigureTBCCP() {
	BOOL bEnabled;

	// cut/delete button
	bEnabled = CanCut();
	SendMessage(hTB, TB_SETSTATE, TB_CUT, STATUS);
	SendMessage(hTB, TB_SETSTATE, TB_DELETE, STATUS);

	// copy button
	bEnabled = CanCopy();
	SendMessage(hTB, TB_SETSTATE, TB_COPY, STATUS);
	SendMessage(hTB, TB_SETSTATE, TB_COPY_TEXT, STATUS);

	// paste button
	bEnabled = CanPaste();
	SendMessage(hTB, TB_SETSTATE, TB_PASTE, STATUS);
	return;
}

void HexEditWnd::ConfigureTB() {
	BOOL bEnabled;

	//Cut copy paste
	ConfigureTBCCP();

	// save button
	bEnabled = CanSave();
	SendMessage(hTB, TB_SETSTATE, TB_SAVE, STATUS);

	// insert button
	bEnabled = bInsert;
	SendMessage(hTB, TB_CHANGEBITMAP, TB_INSERT, bEnabled ? 22 : 23);

	// offset type button
	if (!diData.pDataBuff || diData.qwSize < sizeof(IMAGE_DOS_HEADER) ||
		!file_type((char *)diData.pDataBuff)) {
		SendMessage(hTB, TB_CHANGEBITMAP, TB_OFFSET, 29);
		SendMessage(hTB, TB_SETSTATE, TB_OFFSET, FALSE);
	} else {
		bEnabled = bFileOffset;
		SendMessage(hTB, TB_CHANGEBITMAP, TB_OFFSET, bEnabled ? 29 : 28);
	}

	// undo button
	bEnabled = CanUndo();
	SendMessage(hTB, TB_SETSTATE, TB_UNDO, STATUS);

	// redo button
	bEnabled = CanRedo();
	SendMessage(hTB, TB_SETSTATE, TB_REDO, STATUS);

	// top button
	SendMessage(hTB, TB_CHANGEBITMAP, TB_WIN2TOP, bHEOnTop ? 5 : 6);

	// search buttons
	bEnabled = search.bInited;
	SendMessage(hTB, TB_SETSTATE, TB_SEARCHDOWN, STATUS);
	SendMessage(hTB, TB_SETSTATE, TB_SEARCHUP, STATUS);

	// cut button
	bEnabled = CanCut();
	SendMessage(hTB, TB_SETSTATE, TB_CUT, STATUS);

	// copy button
	bEnabled = CanCopy();
	SendMessage(hTB, TB_SETSTATE, TB_COPY, STATUS);

	// paste button
	bEnabled = CanPaste();
	SendMessage(hTB, TB_SETSTATE, TB_PASTE, STATUS);

	// Display mulitibyte button
	bEnabled = bDispMultiByte;
	SendMessage(hTB, TB_CHANGEBITMAP, TB_MULTI, bEnabled ? 16 : 17);

	// Display Readonly button
	bEnabled = IsReadOnly();
	SendMessage(hTB, TB_CHANGEBITMAP, TB_READONLY, bEnabled ? 18 : 19);
	SendMessage(hTB, TB_SETSTATE, TB_READONLY, 
				diOrgData.bReadOnly ? TBSTATE_INDETERMINATE : TBSTATE_ENABLED );

	bEnabled = IsResizingAllowed();
	SendMessage(hTB, TB_CHANGEBITMAP, TB_SIZE, bEnabled ? 25 : 24);

	return;
}

BOOL HexEditWnd::CanCut() {
	return stat.bSel && !IsReadOnly() && IsResizingAllowed();
}

BOOL HexEditWnd::CanCopy() {
	return stat.bSel;
}

BOOL HexEditWnd::CanPaste() {
	PHE_CLIPBOARD_DATA   pcbd = NULL;
	BOOL	bRet = FALSE;

	if (IsReadOnly() || !IsClipboardFormatOK()) {
		return FALSE;
	}

	if (IsResizingAllowed()) {
		return TRUE;
	}

	if (!OpenClipboard(hMainWnd))
		return FALSE;
	pcbd = GetClipboardData();
	// GetClipboardData already closes clipboard internally; ensure closed
	// (CloseClipboard is safe to call even if already closed? Guard.)
	// Note: GetClipboardData opens/closes itself, so don't double-close here.
	if (!pcbd) {
		return FALSE;
	}
	if (stat.bSel) {
		if (stat.qwOffSelEnd - stat.qwOffSelStart + 1 == pcbd->qwDataSize) {
			bRet = TRUE;
		} else {
			bRet = FALSE;
		}
	} else {
		if (bInsert) {
			bRet = FALSE;
		} else {
			bRet = TRUE;
		}
	}

	if (pcbd) {
		free(pcbd);
	}

	return bRet;
}

BOOL HexEditWnd::SaveChanges() {
	BOOL       bRet = FALSE;

	if (!CanSave()) {
		return FALSE;
	}

	SetStatusInfo("Saving...");
	RepaintClientAreaNow();

	if (fInput.OpenFileForSave()) {
		fInput.SetMapPtrSize(diData.pDataBuff, diData.qwSize);
		fInput.FlushFileMap();
		fInput.SetMapPtrSize(NULL, 0);
		fInput.Destroy();
		savepoint = current;
		bSavePointValid = TRUE;
		SetStatusInfo("Save OK!");

		return TRUE;
	} else {
		MessageBox(hMainWnd, "Unable open file for save", "16Edit", MB_OK | MB_ICONWARNING);
		SetStatusInfo("Save Failed!");
		return FALSE;
	}
}

void HexEditWnd::SetHEWndCaption() {
	char cCaption[400];

	if (!diOrgData.qwSize)
		SetWindowText(hMainWnd, HEDIT_WND_TITLE);

	lstrcpy(cCaption, HEDIT_WND_TITLE);
	lstrcat(cCaption, " - ");
	lstrcat(cCaption, fInput.GetFilePath());
	lstrcat(cCaption, " ");
	if (IsReadOnly())
		lstrcat(cCaption, "[READONLY]");
	else
		lstrcat(cCaption, "[READWRITE]");

	if (CanSave()) {
		lstrcat(cCaption, " *");
	}

	SetWindowText(hMainWnd, cCaption);
	return;
}

BOOL HexEditWnd::IsReadOnly() {
	return diOrgData.bReadOnly || bReadOnly;
}

BOOL HexEditWnd::SetSelection(ULONGLONG qwOffStart, ULONGLONG qwOffEnd) {
	if (IsOutOfRange(qwOffStart) || IsOutOfRange(qwOffEnd))
		return FALSE;

	stat.qwOffSelStart   = __min(qwOffStart, qwOffEnd);
	stat.qwOffSelEnd     = __max(qwOffStart, qwOffEnd);
	stat.bSel            = TRUE;

	if (stat.bCaretVisible) {
		HideCaret(hMainWnd);
		stat.bCaretVisible = FALSE;
	}

	if (qwOffEnd > qwOffStart) {
		stat.posCaret.bHiword       = FALSE;
		stat.posCaret.qwOffset      = qwOffEnd;
	} else {
		stat.posCaret.bHiword   = TRUE;
		stat.posCaret.qwOffset  = qwOffEnd;
	}

	if (!stat.bCaretPosValid) {
		stat.posCaret.bTextSection = FALSE;
		stat.bCaretPosValid        = TRUE;
	}

	ConfigureTBCCP();
	RepaintClientArea();

	return TRUE;
}

//
// returns:
// whether there was a selection before
//
BOOL HexEditWnd::KillSelection() {
	int iRet;

	if (stat.bSel) {
		stat.bSel = FALSE;
		ConfigureTBCCP();
		iRet = TRUE;
	} else {
		iRet = FALSE;
	}

	return iRet;
}

//
// args:
// ppClient - x and y position in client coordinates
//
// returns:
// whether the the input changed the selection
//
BOOL HexEditWnd::Point2Selection(LPPOINT ppClient)
{
	HE_POS  pos, *pposLast;

	if (!stat.bLastLBDownPos)
		return FALSE;

	if (!PointToPos(ppClient, &pos))
		return FALSE;

	// mouse moved since last button down ?
	pposLast = &stat.posLastLButtonDown;
	if (pos.bTextSection == pposLast->bTextSection) {
		if ((pos.qwOffset != pposLast->qwOffset) ||
			((pos.qwOffset == pposLast->qwOffset) && pos.bHiword != pposLast->bHiword)) {
			SetSelection(pposLast->qwOffset, pos.qwOffset);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL HexEditWnd::MouseMoveSelect(LPPOINT pos) {
	POINT  poi;
	BOOL   bRet;

	poi = *pos;

	// cursor not in text/hex region ?
	if ((DWORD)poi.x > PAIRS_X
		&& (DWORD)poi.x < CHARS_X + 16*uFontWidth
		&& stat.bMouseSelecting)
		if ((DWORD)poi.y > iyHETop + uMaxLines*uFontHeight) { // under ?
			// scroll down
			stat.posCaret.qwOffset += 2*16;
			ValidatePos( &stat.posCaret );
			MakeCaretVisible();

			if (stat.posCaret.bTextSection)
				poi.x = CHARS_X + 15*uFontWidth;
			else
				poi.x = PAIRS_X + 15*DIGIT_PAIR_WIDTH;
			poi.y = iyHETop + uFontHeight * (uMaxLines-1);
			if (timerId == 0) {
				SetTimer(hMainWnd, SELECT_TIMER, 50, NULL);
			}
		} else if ((DWORD)poi.y < iyHETop) { // over ?
			// scroll up
			stat.posCaret.qwOffset -= 2*16;
			ValidatePos( &stat.posCaret );
			MakeCaretVisible();

			if (stat.posCaret.bTextSection)
				poi.x = CHARS_X;
			else
				poi.x = PAIRS_X;
			poi.y = iyHETop;
			if (timerId == 0) {
				SetTimer(hMainWnd, SELECT_TIMER, 50, NULL);
			}
		} else {
			KillTimer(hMainWnd, SELECT_TIMER);
			timerId = 0;
		}

	bRet = Point2Selection(&poi);

	return bRet; // RET
}

//
// Args:
// hWnd - can be the window handle of the main window or of the TB
//
BOOL HexEditWnd::HEHandleWM_MOUSEMOVE(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	POINT  poi;

	//
	// handle selection
	//
	if ( !TESTBIT(wParam, MK_LBUTTON) )
		return FALSE; // ERR

	poi.x = LOWORD(lParam);
	poi.y = HIWORD(lParam);
	
	return MouseMoveSelect(&poi);
}

ULONGLONG HexEditWnd::GetCurrentLine() {
	return stat.qwCurOffset / 16;
}

//
// same as "SetTopLine" but fails if "iLine" is invalid
//
BOOL HexEditWnd::SetCurrentLine(ULONGLONG qwLine)
{
	if (qwLine > GetTotalLineNum())
		return FALSE; // ERR

	if (qwLine > (ULONGLONG)INT_MAX)
		return FALSE; // ERR - beyond scrollbar range
	SetTopLine((int)qwLine);

	return TRUE;
}

//
// returns:
// whether sth was fixed
//
BOOL HexEditWnd::ValidateLine(LONGLONG *pllLine)
{
	LONGLONG llLastLine = (LONGLONG)GetTotalLineNum();
	if (llLastLine > 0)
		llLastLine -= 1;
	else
		llLastLine = 0;
	if (llLastLine > INT_MAX)
		llLastLine = INT_MAX;

	if (*pllLine < 0) {
		*pllLine = 0;
		return TRUE; // OK
	} else if (*pllLine > llLastLine) {
		*pllLine = llLastLine;
		return TRUE; // OK
	} else
		return FALSE; // ERR
}

//
// returns:
// whether the last state
//
BOOL HexEditWnd::SetHEWnd2Top(BOOL bTop)
{
	RECT  rect;
	BOOL  bBefore;

	// set wnd state
	GetWindowRect(hMainWnd, &rect);
	SetWindowPos(
				hMainWnd,
				bTop ? HWND_TOPMOST : HWND_NOTOPMOST,
				rect.left,
				rect.top,
				rect.right - rect.left,
				rect.bottom - rect.top,
				SWP_SHOWWINDOW);

	// handle vars
	bBefore  = bHEOnTop;
	bHEOnTop = bTop;

	return bBefore; 
}

BOOL HexEditWnd::Search(PHE_SEARCHOPTIONS pso, ULONGLONG *pOffset) {
	ULONGLONG qwCurOff;
	BYTE*   pby;
	BOOL    bFound;

	if (!pso->bInited)
		return FALSE;

	if (pso->qwStr == 0 || pso->qwStr > diData.qwSize)
		return FALSE;

	qwCurOff = pso->qwStartOff;
	if (qwCurOff >= diData.qwSize)
		qwCurOff = diData.qwSize - 1;

	// correct a too big off (when up search)
	if (!pso->bDown &&
		diData.qwSize - qwCurOff < pso->qwStr)
	{
		if (diData.qwSize < pso->qwStr)
			return FALSE;
		qwCurOff = diData.qwSize - pso->qwStr;
	}

	pby      = diData.pDataBuff + qwCurOff;
	bFound   = FALSE;

	if (pso->bASCIIStr || pso->bWideCharStr) {
		//
		// non-case sensitive ASCII/UNICODE string search
		//
		if (!pso->bCaseSensitive) {
			if ( pso->bASCIIStr ) {
				// ascii
				if (pso->bDown)	// ...down
					while (qwCurOff + pso->qwStr <= diData.qwSize) {
						if (!strnicmp((PCSTR)pso->pData, (PCSTR)pby, (SIZE_T)pso->qwStr)) {
							bFound = TRUE;
							break;
						}
						++qwCurOff;
						++pby;
					}
				else // ...up
					while (qwCurOff != (ULONGLONG)-1) {
						if (!strnicmp((PCSTR)pso->pData, (PCSTR)pby, (SIZE_T)pso->qwStr)) {
							bFound = TRUE;
							break;
						}
						if (qwCurOff == 0)
							break;
						--qwCurOff;
						--pby;
					}
			} else {
				// unicode
				if (pso->bDown)	// ...down
					while (qwCurOff + pso->qwStr <= diData.qwSize) {
						if ( !wcsnicmp((LPWSTR)pso->pData, (LPWSTR)pby, (SIZE_T)(pso->qwStr/2)) ) {
							bFound = TRUE;
							break;
						}
						++qwCurOff;
						++pby;
					}
				else // ...up
					while (qwCurOff != (ULONGLONG)-1) {
						if ( !wcsnicmp((LPWSTR)pso->pData, (LPWSTR)pby, (SIZE_T)(pso->qwStr/2)) ) {
							bFound = TRUE;
							break;
						}
						if (qwCurOff == 0)
							break;
						--qwCurOff;
						--pby;
					}
			}
		}
		//
		// case sensitive ASCII/UNICODE string search
		//
		else {
			if (pso->bDown)	// ..down
				while (qwCurOff + pso->qwStr <= diData.qwSize) {
					if (!memcmp(pso->pData, pby, (SIZE_T)pso->qwStr)) {
						bFound = TRUE;
						break;
					}
					++qwCurOff;
					++pby;
				}
			else // ...up
				while (qwCurOff != (ULONGLONG)-1) {
					if (!memcmp(pso->pData, pby, (SIZE_T)pso->qwStr)) {
						bFound = TRUE;
						break;
					}
					if (qwCurOff == 0)
						break;
					--qwCurOff;
					--pby;
				}
		}
	} else {
		//
		// byte search
		//
		if (pso->bDown)	// ...down
			while (qwCurOff + pso->qwStr <= diData.qwSize) {
				if (!memcmp(pso->pData, pby, (SIZE_T)pso->qwStr)) {
					bFound = TRUE;
					break;
				}
				++qwCurOff;
				++pby;
			}
		else
			while (qwCurOff != (ULONGLONG)-1) {
					if (!memcmp(pso->pData, pby, (SIZE_T)pso->qwStr)) {
						bFound = TRUE;
						break;
					}
				if (qwCurOff == 0)
					break;
				--qwCurOff;
				--pby;
			}

	}

	if (bFound) {
		if (pOffset != NULL) {
			*pOffset = qwCurOff;
		}
	}
	return bFound;
}

//
// searchs the stuff in the HE_SEARCHOPTIONS structure and setups the GUI
//
// returns:
// whether sth was found
//
//#pragma optimize("", off)
BOOL HexEditWnd::PerformStrSearch(PHE_SEARCHOPTIONS pso) {
	ULONGLONG   qwCurOff;
	BOOL    bFound;

	SetStatusInfo("Searching...");
	//KillSelection();
	RepaintClientAreaNow();
	bFound = Search(pso, &qwCurOff);

	if (bFound) {
		SetSelection(qwCurOff, qwCurOff + pso->qwStr - 1);
		if (!IsOffsetVisible(qwCurOff))
			SetTopLine(qwCurOff);
		SetStatusInfo("Found!");
	} else {
		SetStatusInfo(pso->bDown ? "Buffer end reached." : "Buffer start reached.");
	}

	MakeCaretVisible();
	SetCaret();
	RepaintClientArea();
	return bFound;
}

BOOL HexEditWnd::PerformSearchAgain(PHE_SEARCHOPTIONS pso, BOOL bDown) {
	if (!pso->bInited)
		return FALSE; // ERR

	if (!stat.bCaretPosValid) {
		ErrMsg("Caret to set!");
		return FALSE; // ERR
	}

	if (bDown) {
		if (stat.bSel)
		{
			if (stat.qwOffSelEnd + 1 < stat.qwOffSelEnd)
				return FALSE; // overflow (file near 2^64, unrealistic)
			pso->qwStartOff = stat.qwOffSelEnd + 1;
		}
		else
			pso->qwStartOff	= stat.posCaret.qwOffset;
	} else {
		if (stat.bSel)
		{
			if (stat.qwOffSelStart == 0)
				pso->qwStartOff = 0;
			else
				pso->qwStartOff = stat.qwOffSelStart - 1;
		}
		else
			pso->qwStartOff	= stat.posCaret.qwOffset;
	}
	pso->bDown = bDown;

	return PerformStrSearch(pso);
}

BOOL HexEditWnd::PerformStrReplace(PHE_SEARCHOPTIONS pso) {
	ULONGLONG   qwCurOff;
	BOOL    bFound;

	if (IsReadOnly() || (pso->qwStr != pso->qwReplaceStr && !IsResizingAllowed())) {
		SetStatusInfo("Readonly or not sizable!");
		RepaintClientArea();
		return FALSE;
	}

	/*
	 * may be already selected
	 */
	if (pso->bDown) {
		if (stat.bSel)
			pso->qwStartOff = stat.qwOffSelStart;
	} else {
		if (stat.bSel)
			pso->qwStartOff = stat.qwOffSelEnd;
	}

	bFound = Search(pso, &qwCurOff);
	if (bFound) {
		HE_OPER *op = new HE_OPER(op_paste, qwCurOff, pso->qwStr, pso->qwReplaceStr);
		memcpy(op->oldData, diData.pDataBuff + qwCurOff, (SIZE_T)pso->qwStr);
		if (pso->qwReplaceStr > 0) {
			memcpy(op->newData, pso->pReplaceData, (SIZE_T)pso->qwReplaceStr);
		}
		AddOper(op);
		ApplyOper(op);
		if (!IsOffsetVisible(qwCurOff))
			SetTopLine(qwCurOff);
		SetStatusInfo("Repalced!");
	} else {
		SetStatusInfo("Not found!");
	}

	ConfigureTB();
	SetupVScrollbar();
	RepaintClientArea();
	return bFound;
}

BOOL HexEditWnd::PerformStrReplaceAll(PHE_SEARCHOPTIONS pso) {
	ULONGLONG   qwCurOff;
	BOOL    bFound;
	int		count;

	if (IsReadOnly() || (pso->qwStr != pso->qwReplaceStr && !IsResizingAllowed())) {
		SetStatusInfo("Readonly or not sizable!");
		return FALSE;
	}

	pso->qwStartOff = 0;
	pso->bDown = TRUE;

	count = 0;
	while (TRUE) {
		bFound = Search(pso, &qwCurOff);
		if (!bFound) {
			break;
		}
		count++;
		HE_OPER *op = new HE_OPER(op_paste, qwCurOff, pso->qwStr, pso->qwReplaceStr);
		memcpy(op->oldData, diData.pDataBuff + qwCurOff, (SIZE_T)pso->qwStr);
		if (pso->qwReplaceStr > 0) {
			memcpy(op->newData, pso->pReplaceData, (SIZE_T)pso->qwReplaceStr);
		}
		AddOper(op);
		ApplyOper(op);
		if (!IsOffsetVisible(qwCurOff))
			SetTopLine(qwCurOff);
		pso->qwStartOff = qwCurOff + pso->qwStr;
	}

	if (count > 0) {
		SetStatusInfo("%d occurance replaced!", count);
	} else {
		SetStatusInfo("Search string not found!");
	}
	ConfigureTB();
	SetupVScrollbar();
	RepaintClientArea();
	return bFound;
}

BOOL HexEditWnd::CopySelectedBlockAsText() {
	HANDLE               hMem;
	void                 *pMem;
	ULONGLONG 			 qwCount;

	if (!CanCopy()) return FALSE;
	return TRUE;

	qwCount = stat.qwOffSelEnd - stat.qwOffSelStart + 1;
	if (qwCount > (ULONGLONG)(SIZE_MAX) - 1)
	{
		ErrMsg(STR_NO_MEM);
		return FALSE;
	}
	if (!OpenClipboard(NULL)) {
		ErrMsg("Couldn't open clipboard!");
		return FALSE;
	}

	hMem = GlobalAlloc(GHND | GMEM_SHARE, (SIZE_T)qwCount + 1);
	if (!hMem) {
		CloseClipboard();
		ErrMsg(STR_NO_MEM);
		return FALSE;
	}

	pMem = GlobalLock(hMem);
	memcpy(pMem, diData.pDataBuff + stat.qwOffSelStart, (SIZE_T)qwCount);
	((char *)pMem)[(SIZE_T)qwCount] = 0;
	GlobalUnlock(hMem);

	if (!SetClipboardData(CF_TEXT, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		ErrMsg("Couldn't copy data to clipboard!");
		return FALSE;
	}

	CloseClipboard();
	ConfigureTB();
	return TRUE;
}

BOOL HexEditWnd::CopySelectedBlock()
{
	HANDLE               hMem;
	void                 *pMem;
	PHE_CLIPBOARD_DATA   pcbd;
	ULONGLONG            qwCount;

	if (!CanCopy()) return FALSE;

	qwCount = stat.qwOffSelEnd - stat.qwOffSelStart + 1;
	if (qwCount > (ULONGLONG)(SIZE_MAX) - 1 ||
		qwCount + sizeof(ULONGLONG) < qwCount)
	{
		ErrMsg(STR_NO_MEM);
		return FALSE;
	}
	if (!OpenClipboard(NULL)) {
		ErrMsg("Couldn't open clipboard!");
		return FALSE;
	}

	hMem = GlobalAlloc(GHND | GMEM_SHARE, (SIZE_T)qwCount + 1);
	if (!hMem) {
		CloseClipboard();
		ErrMsg(STR_NO_MEM);
		return FALSE;
	}
	pMem = GlobalLock(hMem);
	memcpy(pMem, diData.pDataBuff + stat.qwOffSelStart, (SIZE_T)qwCount);
	((char *)pMem)[(SIZE_T)qwCount] = 0;
	GlobalUnlock(hMem);

    EmptyClipboard();
	if (!SetClipboardData(CF_TEXT, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		ErrMsg("Couldn't copy data to clipboard!");
		return FALSE;
	}

	hMem  = GlobalAlloc(GHND | GMEM_SHARE, sizeof(ULONGLONG) + (SIZE_T)qwCount);
	if (!hMem) {
		CloseClipboard();
		ErrMsg(STR_NO_MEM);
		return FALSE;
	}
	pMem = GlobalLock(hMem);
	pcbd = (PHE_CLIPBOARD_DATA)pMem;
	memcpy(
		  &pcbd->byDataStart,
		  diData.pDataBuff + stat.qwOffSelStart,
		  (SIZE_T)qwCount);
	pcbd->qwDataSize = qwCount;
	GlobalUnlock(hMem);

	if (!SetClipboardData(cf16Edit, hMem)) {
		GlobalFree(hMem);
		CloseClipboard();
		ErrMsg("Couldn't copy data to clipboard!");
		return FALSE;
	}

	CloseClipboard();
	ConfigureTB();
	return TRUE;
}

BOOL HexEditWnd::DeleteSelectedBlock() {
	if (!CanCut()) return FALSE;

	ULONGLONG qwSelLen = stat.qwOffSelEnd - stat.qwOffSelStart + 1;

	HE_OPER *op = new HE_OPER(op_cut, stat.qwOffSelStart, qwSelLen, 0);
	memcpy(op->oldData, diData.pDataBuff + stat.qwOffSelStart, (SIZE_T)qwSelLen);
	AddOper(op);
	ApplyOper(op);

	KillSelection();
	ConfigureTB();
	SetupVScrollbar();
	RepaintClientArea();

	return TRUE;
}

BOOL HexEditWnd::CutSelectedBlock() {
	if (!CanCut()) return FALSE;

	if (!CopySelectedBlock())
		return FALSE;

	DeleteSelectedBlock();
	return TRUE;
}

BOOL HexEditWnd::PasteBlockFromCB() {
	PHE_CLIPBOARD_DATA   pcbd;

	if (!CanPaste()) return FALSE;

	if (!stat.bCaretPosValid) {
		ErrMsg("Please select one byte first!");
		return FALSE;
	}

	if (!OpenClipboard(hMainWnd)) {
		ErrMsg("Couldn't open clipboard!");
		return FALSE;
	}
	pcbd = GetClipboardData();
	if (!pcbd) {
		CloseClipboard();
		ErrMsg("Couldn't receive clipboard data!");
		return FALSE;
	}
	CloseClipboard();


	ULONGLONG qwOldLen, qwOffset;
	if (stat.bSel) {
		qwOldLen = stat.qwOffSelEnd - stat.qwOffSelStart + 1;
		qwOffset = stat.qwOffSelStart;
		KillSelection();
	} else {
		if (bInsert) {
			qwOldLen = 0;
		} else {
			qwOldLen = pcbd->qwDataSize;
		}
		qwOffset = stat.posCaret.qwOffset;
	}

	if (qwOldLen == pcbd->qwDataSize) {
		if (!memcmp(diData.pDataBuff + qwOffset, &pcbd->byDataStart, (SIZE_T)qwOldLen)) {
			if (pcbd) {
				free(pcbd);
			}
			return TRUE;
		}
	}

	HE_OPER *op = new HE_OPER(op_paste, qwOffset, qwOldLen, pcbd->qwDataSize);
	if (qwOldLen > 0) {
		memcpy(op->oldData, diData.pDataBuff + qwOffset, (SIZE_T)qwOldLen);
	}

	memcpy(op->newData, &pcbd->byDataStart, (SIZE_T)pcbd->qwDataSize);
	AddOper(op);
	ApplyOper(op);

	//
	// repaint
	//
	ConfigureTB();
	SetupVScrollbar();
	if (!IsOffsetVisible( stat.posCaret.qwOffset ) )
		SetTopLine();
	RepaintClientArea();

	if (pcbd) {
		free(pcbd);
	}

	return TRUE; // OK
}

BOOL HexEditWnd::UndoChanges() {
	if (!CanUndo()) {
		return FALSE; 
	}

	HE_OPER *op = current->getOper();
	UndoOper(op);
	current = current->getPrev();

	ValidatePos( &stat.posCaret );
	SetCaret();
	ConfigureTB();
	SetupVScrollbar();
	if (!IsOffsetVisible( stat.posCaret.qwOffset ) )
		SetTopLine();
	RepaintClientArea();

	return TRUE;
}

BOOL HexEditWnd::RedoChanges() {
	if (!CanRedo()) {
		return FALSE; 
	}

	current = current->getNext();
	HE_OPER *op = current->getOper();
	ApplyOper(op);

	ValidatePos( &stat.posCaret );
	SetCaret();
	ConfigureTB();
	SetupVScrollbar();
	if (!IsOffsetVisible( stat.posCaret.qwOffset ) )
		SetTopLine();
	RepaintClientArea();

	return TRUE;
}

void HexEditWnd::AddOper(HE_OPER *op) {
	if (current == NULL) {
		current = operList->addOper(op);
	} else {
		if (savepoint != NULL && current->before(savepoint)) {
			savepoint = operList;
			bSavePointValid = FALSE;
		}
		current->releaseAfter();
		current = current->addOper(op);
	}
}

BOOL HexEditWnd::CanSave() {
	if (!bSavePointValid || current != savepoint) {
		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL HexEditWnd::CanRedo() {
	if (IsReadOnly())
		return FALSE;
	if (current->getNext() != NULL) {
		return TRUE;
	} else {
		return FALSE;
	}
}

BOOL HexEditWnd::CanUndo() {
	if (IsReadOnly())
		return FALSE;
	if (current == operList) {
		return FALSE;
	}

	return TRUE;
}

void HexEditWnd::ApplyOper(HE_OPER *op) {
	HE_POS posNew;

	memcpy(&posNew, &stat.posCaret, sizeof(HE_POS));
	posNew.qwOffset = op->qwOffset;
	posNew.bHiword      = TRUE;
	switch (op->type) {
		case op_modify:
			((BYTE *)(diData.pDataBuff))[op->qwOffset] = op->newData[0];
			break;
		case op_cut:
		   	if (diData.qwSize > op->qwOffset + op->qwOldLen)
				MEMCPY(
				  (BYTE*)diData.pDataBuff + op->qwOffset,
				  (BYTE*)diData.pDataBuff + op->qwOffset + op->qwOldLen,
				  (SIZE_T)(diData.qwSize - op->qwOffset - op->qwOldLen));
			diData.qwSize -= op->qwOldLen;
			if (diData.qwSize == 0)
			{
				free(diData.pDataBuff);
				diData.pDataBuff = NULL;
			}
			else
			{
				BYTE *pNew = (BYTE*)realloc(diData.pDataBuff, (SIZE_T)diData.qwSize);
				if (pNew)
					diData.pDataBuff = pNew;
			}
			KillSelection();
			break;
		case op_paste:
			if (op->qwNewLen > op->qwOldLen) {
				ULONGLONG qwNewSize = diData.qwSize + op->qwNewLen - op->qwOldLen;
				BYTE *pNew = (BYTE*)realloc(diData.pDataBuff, (SIZE_T)qwNewSize);
				if (!pNew)
				{
					ErrMsg("Not enough memory available !");
					break;
				}
				diData.pDataBuff = pNew;
				MEMCPY(
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwNewLen, 
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwOldLen, 
					(SIZE_T)(diData.qwSize - op->qwOffset - op->qwOldLen)
					);
				diData.qwSize = qwNewSize;
			} else if (op->qwOldLen > op->qwNewLen) {
				MEMCPY(
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwNewLen,
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwOldLen, 
					(SIZE_T)(diData.qwSize - op->qwOffset - op->qwOldLen)
					);
				diData.qwSize -= op->qwOldLen - op->qwNewLen;
				if (diData.qwSize == 0)
				{
					free(diData.pDataBuff);
					diData.pDataBuff = NULL;
				}
				else
				{
					BYTE *pNew = (BYTE*)realloc(diData.pDataBuff, (SIZE_T)diData.qwSize);
					if (pNew)
						diData.pDataBuff = pNew;
				}
			}
			memcpy((BYTE*)diData.pDataBuff + op->qwOffset, op->newData, (SIZE_T)op->qwNewLen);
			break;
	}
	SetCaret(&posNew);
}

void HexEditWnd::UndoOper(HE_OPER *op) {
	HE_POS posNew;

	memcpy(&posNew, &stat.posCaret, sizeof(HE_POS));
	posNew.qwOffset = op->qwOffset;
	posNew.bHiword      = TRUE;
	switch (op->type) {
		case op_modify:
			((BYTE *)(diData.pDataBuff))[op->qwOffset] = op->oldData[0];
			break;
		case op_cut:
			{
				ULONGLONG qwNewSize = diData.qwSize + op->qwOldLen;
				BYTE *pNew = (BYTE*)realloc(diData.pDataBuff, (SIZE_T)qwNewSize);
				if (!pNew)
				{
					ErrMsg("Not enough memory available !");
					break;
				}
				diData.pDataBuff = pNew;
				if (diData.qwSize > op->qwOffset) {
					MEMCPY(
						(BYTE*)diData.pDataBuff + op->qwOffset + op->qwOldLen,
						(BYTE*)diData.pDataBuff + op->qwOffset,
						(SIZE_T)(diData.qwSize - op->qwOffset)
						  );
				}
				memcpy((BYTE*)diData.pDataBuff + op->qwOffset, op->oldData, (SIZE_T)op->qwOldLen);
				diData.qwSize = qwNewSize;
			}

			SetSelection(op->qwOffset, op->qwOffset + op->qwOldLen - 1);
			break;
		case op_paste:
			if (op->qwNewLen > op->qwOldLen) {
				MEMCPY(
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwOldLen, 
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwNewLen, 
					(SIZE_T)(diData.qwSize - op->qwOffset - op->qwNewLen)
					);
				diData.qwSize -= op->qwNewLen - op->qwOldLen;
				if (diData.qwSize == 0)
				{
					free(diData.pDataBuff);
					diData.pDataBuff = NULL;
				}
				else
				{
					BYTE *pNew = (BYTE*)realloc(diData.pDataBuff, (SIZE_T)diData.qwSize);
					if (pNew)
						diData.pDataBuff = pNew;
				}
			} else if (op->qwOldLen > op->qwNewLen) {
				ULONGLONG qwNewSize = diData.qwSize + op->qwOldLen - op->qwNewLen;
				BYTE *pNew = (BYTE*)realloc(diData.pDataBuff, (SIZE_T)qwNewSize);
				if (!pNew)
				{
					ErrMsg("Not enough memory available !");
					break;
				}
				diData.pDataBuff = pNew;
				MEMCPY(
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwOldLen,
					(BYTE*)diData.pDataBuff + op->qwOffset + op->qwNewLen, 
					(SIZE_T)(diData.qwSize - op->qwOffset - op->qwNewLen)
					);
				diData.qwSize = qwNewSize;
			}

			if (op->qwOldLen > 0) {
				memcpy((BYTE*)diData.pDataBuff + op->qwOffset, op->oldData, (SIZE_T)op->qwOldLen);
			}
			break;
	}
	SetCaret(&posNew);
}

void HexEditWnd::SelectAll() {
	if (diData.qwSize == 0)
		return;
	if (IsAllSelected()) {
		KillSelection();
		SetCaret();
	} else {
		SetSelection(0, diData.qwSize - 1);
	}

	return;
}

BOOL HexEditWnd::IsAllSelected() {
	if (diData.qwSize == 0)
		return FALSE;
	return(stat.bSel &&
		   stat.qwOffSelStart == 0 &&
		   stat.qwOffSelEnd == diData.qwSize - 1) ? TRUE : FALSE;
}

BOOL HexEditWnd::IsResizingAllowed() {
	return bResizingAllowed;
}

BOOL HexEditWnd::DoSpecifySettings(char *filename, ULONGLONG start, ULONGLONG len) {
	InitEdition();
	if (!DoEditFile(filename, FALSE)) {
		return FALSE;
	}

	bResizingAllowed = FALSE;
	bMinToTray       = FALSE;
	bSaveWinPos      = TRUE;
	bReadOnly = diOrgData.bReadOnly;
	if (diData.qwSize > 0 && start < diData.qwSize) {
		ULONGLONG qwEnd = start + (len > 0 ? len - 1 : 0);
		if (qwEnd >= diData.qwSize)
			qwEnd = diData.qwSize - 1;
		SetSelection(start, qwEnd);
		stat.qwCurOffset = start;
		stat.posCaret.qwOffset = start;
	}

	return TRUE;
}

void HexEditWnd::HEHandleWM_CLOSE(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HE_WIN_POS   wp;
	int          iRet;

	if (CanSave()) {
		iRet = MessageBox(hMainWnd, "File changed, save or not", "16Edit", MB_YESNOCANCEL);
		if (iRet == IDYES) {
			SaveChanges();
		} else if (iRet == IDCANCEL) {
			return;
		}
	}

	if (bSaveWinPos) {
		wp.ix    = rctLastPos.left;
		wp.iy    = rctLastPos.top;
		wp.icx   = rctLastPos.right - rctLastPos.left;
		wp.icy   = rctLastPos.bottom - rctLastPos.top;
		WritePrivateProfileStruct(INI_SECTION, INI_WINPOSKEY, &wp, sizeof(wp), cIniPath);
	}

	PostQuitMessage(0);
	return;
}

void HexEditWnd::HEHandleWM_TRAYMENU(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	POINT p;

	switch (lParam) {
	case WM_RBUTTONDOWN:
		GetCursorPos(&p);
		TrackPopupMenu(hmTray, TPM_RIGHTALIGN, p.x, p.y, 0, hWnd, NULL);
		break;

	case WM_LBUTTONDBLCLK:
		HEditReturnFromTray();
		break;
	}
	return;
}

void HexEditWnd::HEditQuit() {
	SendMessage(hMainWnd, WM_CLOSE, 0, 0);
	return;
}

void HexEditWnd::HEHandleWM_MOVE(HWND hWnd, WPARAM wParam, LPARAM lParam) {
	if (!IsIconic(hWnd) &&
		!IsZoomed(hWnd) )
		GetWindowRect( hWnd, &rctLastPos );
	return;
}

BOOL HexEditWnd::IsKeyDown(int iVKey) {
	return( HIWORD( GetAsyncKeyState(iVKey) ) != 0);
}

void HexEditWnd::SetTBHandle(HWND hTB) {
	this->hTB = hTB;
	return;
}

HINSTANCE HexEditWnd::GetInstance() {
	return hInst;
}

void HexEditWnd::SetStatusInfo(char *szFormat, ...) {
	va_list args;

	va_start(args, szFormat);
	wvsprintf(cSBInfo, szFormat, args);
	va_end(args);

	return;
}

void HexEditWnd::SetStatusText(char *szFormat, ...) {
	va_list args;

	va_start(args, szFormat);
	wvsprintf(cSBText, szFormat, args);
	va_end(args);

	if (strlen(cSBInfo) > 0) {
		strcat(cSBText, " | ");
		strcat(cSBText, cSBInfo);
	}
	return;
}

void HexEditWnd::SetStatusText() {
	char	msg[256];
	char	szOS[24], szNS[24], szFO[24], szVA[24];
	ULONGLONG	qwOffset;

	if (stat.bSel)
		qwOffset = stat.qwOffSelStart;
	else if (stat.bCaretPosValid)
		qwOffset = stat.posCaret.qwOffset;
	else
		qwOffset = 0;

	FormatOffset64(szOS, qwOldSize);
	FormatOffset64(szNS, diData.qwSize);
	FormatOffset64(szFO, qwOffset);
	wsprintf(msg, "OS:0x%s | NS:0x%s | FO:0x%s", szOS, szNS, szFO);
	if (diData.pDataBuff && diData.qwSize >= sizeof(IMAGE_DOS_HEADER) &&
		file_type((char *)diData.pDataBuff)) {
		FormatOffset64(szVA, GetVirtualAddress(qwOffset));
		wsprintf(msg, "%s | VA:0x%s", msg, szVA);
	}

	SetStatusText(msg);
	return;
}

void HexEditWnd::ShowAbout() {
	MSGBOXPARAMS args;

	ZERO(args);
	args.cbSize         = sizeof( args );
	args.hwndOwner      = hMainWnd;
	args.dwStyle        = MB_USERICON;
	args.hInstance      = GetInstance();
	args.lpszIcon       = (PSTR)IDI_16Edit;
	args.lpszCaption    = "About";
	args.lpszText       = "16Edit 1.04+ - HexEditor by slangmgh\n"\
						  "Created from yoda's 16Edit module\n\n"\
						  "Changelog 1.04+:\n"
						  "1. Copy text available in clipboard\n"
						  "2. Goto offset save in profile\n"
						  "3. Search/Replace text enable paste\n"
						  "4. Search/Replace text save in profile\n"
						  "5. Add [F5] for reloading file\n"
						  "6. Some small bugfix\n\n"
						  "Feature:\n"\
						  "1. Small/Green/Robust/Freeware\n"\
						  "2. Unlimited undo/redo\n"\
						  "3. Ascii/Ansi display switch\n"\
						  "4. Add/Modify/Add&Modify color indicator\n"\
						  "5. Size lock\n"\
						  "6. Shell integration\n"\
						  "7. Paste insert/overwrite mode\n"\
						  "8. Readonly/Readwrite mode\n"\
						  "9. File offset/Virtual address mode\n"\
						  "10. Ascii/Unicode/Binary search/replace\n"\
						  "11. You requesting...\n\n"\
						  "slangmgh@netease.com";
	MessageBoxIndirect( &args );

	return;
}

BOOL HexEditWnd::IsDBCSFirstByte(ULONGLONG qwOffset) {
	BYTE byCur;
	LONGLONG ll;

	if (qwOffset >= diData.qwSize)
		return FALSE;

	byCur = *(diData.pDataBuff + qwOffset);
	if (!IsDBCSLeadByte(byCur)) {
		return FALSE;
	}

	for (ll = (LONGLONG)qwOffset - 1; ll >= 0; ll--) {
		byCur = *(diData.pDataBuff + (ULONGLONG)ll);
		if (!IsDBCSLeadByte(byCur)) {
			if ((qwOffset - (ULONGLONG)ll)%2) {
				return TRUE;
			} else {
				return FALSE;
			}
		}
		if (ll == 0)
			break;
	}

	//
	// every byte before are dbcs lead byte
	//
	if (qwOffset%2) {
		return FALSE;
	} else {
		return TRUE;
	}
}

//
// Get the data status at the given offset
//
// return
// 0 : NO change
// 1 : modify
// 2 : add
// 3 : add & modify
int	HexEditWnd::GetDataStatus(ULONGLONG qwOffset) {
	int status = 0;
	EditOperList *oplist;
	HE_OPER *op;

	if (current == savepoint) {
		return 0;
	}

	if (savepoint->before(current)) {
		oplist = current;
		while (oplist != savepoint) {
			op = oplist->getOper();

			switch (op->type) {
				case op_modify:
					if (qwOffset == op->qwOffset) {
						status |= 1;
					}
					break;
				case op_paste:
					if (qwOffset >= op->qwOffset) {
						if (op->qwNewLen > op->qwOldLen) {
							if (qwOffset >= op->qwOffset + op->qwNewLen) {
								qwOffset -= op->qwNewLen - op->qwOldLen;
							} else if (qwOffset < op->qwOffset + op->qwOldLen) {
								status |= 1;
							} else {
								status |= 2;
								return status;
							}
						} else {
							if (qwOffset >= op->qwOffset + op->qwNewLen) {
								qwOffset += op->qwOldLen - op->qwNewLen;
							} else {
								status |= 1;
							}
						}
					}
					break;
				case op_cut:
					if (qwOffset >= op->qwOffset) {
						if (status & 2 && qwOffset < op->qwOffset + op->qwOldLen) {
							status = 1;
						}
						qwOffset += op->qwOldLen;
					}
					break;
			}
			oplist = oplist->getPrev();
		}
	} else {
		oplist = current->getNext();
		while (oplist != NULL) {
			op = oplist->getOper();

			switch (op->type) {
				case op_modify:
					if (qwOffset == op->qwOffset) {
						status |= 1;
					}
					break;
				case op_paste:
					if (qwOffset >= op->qwOffset) {
						if (op->qwOldLen > op->qwNewLen) {
							if (qwOffset >= op->qwOffset + op->qwOldLen) {
								qwOffset -= op->qwOldLen - op->qwNewLen;
							} else if (qwOffset < op->qwOffset + op->qwNewLen) {
								status |= 1;
							} else {
								status |= 2;
								return status;
							}
						} else {
							if (qwOffset >= op->qwOffset + op->qwOldLen) {
								qwOffset += op->qwNewLen - op->qwOldLen;
							} else {
								status |= 1;
							}
						}
					}
					break;
				case op_cut:
					if (qwOffset >= op->qwOffset && qwOffset < op->qwOffset + op->qwOldLen) {
						status |= 2;
						return status;
					} else if (qwOffset >= op->qwOffset + op->qwOldLen) {
						qwOffset -= op->qwOldLen;
					}
					break;
			}
			if (oplist == savepoint) {
				break;
			} else {
				oplist = oplist->getNext();
			}
		}
	}

	return status;
}

BOOL HexEditWnd::HEditToTray() {
	char *pch;

	ZERO(nidTray);
	nidTray.cbSize            = sizeof(nidTray);

	lstrcpy(nidTray.szTip, HEDIT_TRAY_TIP);
	if (pch = CPathString::ExtractFileName( fInput.GetFilePath())) {
		if (sizeof(nidTray.szTip) - sizeof(HEDIT_TRAY_TIP) - 3 >= lstrlen(pch) ) {
			lstrcat(nidTray.szTip, " - ");
			lstrcat(nidTray.szTip, pch);
			lstrlen(nidTray.szTip);
		}
	}

	nidTray.hWnd              = hMainWnd;
	nidTray.uID               = ID_TRAYICON;
	nidTray.uFlags            = NIF_TIP | NIF_ICON | NIF_MESSAGE;
	nidTray.hIcon             = (HICON)GetClassLongPtr(hMainWnd, GCLP_HICON);
	nidTray.uCallbackMessage  = WM_TRAYMENU;
	if (!Shell_NotifyIcon(NIM_ADD, &nidTray))
		return FALSE;

	ShowWindow(hMainWnd, SW_HIDE);
	return TRUE;
}

BOOL HexEditWnd::HEditKillTrayIcon() {
	return Shell_NotifyIcon(NIM_DELETE, &nidTray);
}

BOOL HexEditWnd::HEditReturnFromTray() {
	if (!HEditKillTrayIcon())
		return FALSE;

	ShowWindow(hMainWnd, SW_RESTORE);
	SetForegroundWindow(hMainWnd);

	return TRUE;
}

BOOL HexEditWnd::IsClipboardFormatOK() {
	return IsClipboardFormatAvailable(cf16Edit) || IsClipboardFormatAvailable(CF_TEXT);
}

PHE_CLIPBOARD_DATA HexEditWnd::GetClipboardData() {
	PHE_CLIPBOARD_DATA	pcbd = NULL;

	if (!OpenClipboard(hMainWnd))
		return NULL;
	if (IsClipboardFormatAvailable(cf16Edit)) {
		PHE_CLIPBOARD_DATA	pcbdold;
		ULONGLONG	qwLen;

		pcbdold = (PHE_CLIPBOARD_DATA)::GetClipboardData(cf16Edit);
		if (pcbdold) {
			qwLen = pcbdold->qwDataSize;
			if (qwLen <= (ULONGLONG)(SIZE_MAX) - sizeof(ULONGLONG))
			{
				pcbd = (PHE_CLIPBOARD_DATA)malloc((SIZE_T)qwLen + sizeof(ULONGLONG));
				if (pcbd)
				{
					pcbd->qwDataSize = qwLen;
					memcpy(&pcbd->byDataStart, &pcbdold->byDataStart, (SIZE_T)qwLen);
				}
			}
		}
	} else if (IsClipboardFormatAvailable(CF_TEXT)) {
		char	*pData;
		SIZE_T	len;

		pData = (char *)::GetClipboardData(CF_TEXT);
		if(pData) {
			len = strlen(pData);
			if (len <= SIZE_MAX - sizeof(ULONGLONG))
			{
				pcbd = (PHE_CLIPBOARD_DATA)malloc(len + sizeof(ULONGLONG));
				if (pcbd)
				{
					pcbd->qwDataSize = (ULONGLONG)len;
					memcpy(&pcbd->byDataStart, pData, len);
				}
			}
		}
	}
	CloseClipboard();
	return pcbd;
}
