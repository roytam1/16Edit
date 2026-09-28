
#include "Common.h"
#include "HexEditWnd.h"

//
// constants
//
#define          CUT_CHAR             0x18
#define          COPY_CHAR            0x03
#define          PASTE_CHAR           0x16
#define          STRZ_CHAR            0x1a

//
// global variables
//

BOOL UnHookHexEditbox(HWND hEdit) {
	char clsName[256];
	GetClassName(hEdit, clsName, sizeof(clsName));

	if (stricmp("ComboBox", clsName) == 0) {
		hEdit = GetWindow(hEdit, GW_CHILD);
	}

	GetClassName(hEdit, clsName, sizeof(clsName));
	WNDCLASS  wc;
	GetClassInfo(NULL, clsName, &wc);

	if (SetWindowLongPtr(
		hEdit,
		GWLP_WNDPROC,
		(LONG_PTR)wc.lpfnWndProc))
		return TRUE;
	else
		return FALSE;
}

//
// window proc stub for hex editbox's
//
BOOL HookHexEditbox(HWND hEdit)
{
	char clsName[256];
	GetClassName(hEdit, clsName, sizeof(clsName));

	if (stricmp("ComboBox", clsName) == 0) {
		hEdit = GetWindow(hEdit, GW_CHILD);
	}

	if (SetWindowLongPtr(
		hEdit,
		GWLP_WNDPROC,
		(LONG_PTR)HexOnlyEditProc))
		return TRUE;
	else
		return FALSE;
}

//
// call this with an edit box as argument to monitor the occurrence of the ENTER key
//
BOOL HookEditboxEnter(HWND hEdit)
{
	char clsName[256];
	GetClassName(hEdit, clsName, sizeof(clsName));

	if (stricmp("ComboBox", clsName) == 0) {
		hEdit = GetWindow(hEdit, GW_CHILD);
	}

	if (SetWindowLongPtr(
		hEdit,
		GWLP_WNDPROC,
		(LONG_PTR)EditHookEnterProc))
		return TRUE;
	else
		return FALSE;
}

LRESULT FUNC_CALLBACK HexOnlyEditProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	CHAR      c = 0;
	/*
	char      cBuff[9], *pCH;
	DWORD     dw;
	*/
	WNDCLASS  wc;

	switch(Msg)
	{
	case WM_PASTE:
		// check whether the contents is a valid hex number string
		if (OpenClipboard(hWnd))
		{
			ULONGLONG	qwLen;
			char	*pData;

			if (IsClipboardFormatAvailable(cf16Edit)) {
				PHE_CLIPBOARD_DATA	pcbd;

				pcbd = (PHE_CLIPBOARD_DATA)::GetClipboardData(cf16Edit);
				qwLen = pcbd->qwDataSize;
				pData = (char *)&pcbd->byDataStart;
			} else if (IsClipboardFormatAvailable(CF_TEXT)) {
				pData = (char *)::GetClipboardData(CF_TEXT);
				qwLen = strlen(pData);
			} else {
				return 0;
			}

			for (ULONGLONG i = 0; i < qwLen; i++) {
				int ch1 = (pData[i] & 0xF0) >> 4;
				int ch2 = (pData[i] & 0x0F);

				if (ch1 >= 0 && ch1 <= 9) {
					ch1 += 0x30;
				} else {
					ch1 += 'A' - 0xA;
				}
				PostMessage(hWnd, WM_CHAR, ch1, 0);

				if (ch2 >= 0 && ch2 <= 9) {
					ch2 += 0x30;
				} else {
					ch2 += 'A' - 0xA;
				}
				PostMessage(hWnd, WM_CHAR, ch2, 0);
			}
			CloseClipboard();
			/*
			pCH = (char*)GetClipboardData(EnumClipboardFormats(CF_LOCALE)); // for win2k
			if (!pCH)
				pCH = (char*)GetClipboardData(EnumClipboardFormats(0));
			CloseClipboard();
			if (pCH)
				if (lstrcpyn(cBuff, pCH, sizeof(cBuff)))
					if (HexStrToInt(cBuff, &dw))
						break;			
			*/
		}
		return 0;

	case WM_KEYDOWN:
		// inform parent window via WM_COMMAND msg about ENTER key occurrence
		switch(wParam)
		{
		case VK_RETURN:
			SendMessage(
				GetParent(hWnd),
				WM_COMMAND,
				MAKEWPARAM(GetDlgCtrlID(hWnd), NM_HEXEDITENTER),
				(LPARAM)hWnd);
			return 0;

		//case VK_ESCAPE:
			// if the control handles this key it'll close the parent dlg - dunno why :(
		//	return 0;
		}
		break;

	case WM_CHAR:
		// force hex characters
		if (wParam != VK_BACK    &&
		   wParam  != CUT_CHAR   && 
		   wParam  != PASTE_CHAR &&
		   wParam  != COPY_CHAR  &&
		   wParam  != STRZ_CHAR)         
		{
			c = toupper(wParam);
			if ( (c < '0' || c > '9') &&
				 (c < 'A' || c > 'F'))
				 return 0;
			else
				wParam = c;
		}
		break;
	}

	char clsName[256];
	GetClassName(hWnd, clsName, sizeof(clsName));

	// get default edit wnd proc
	GetClassInfo(NULL, clsName, &wc);

	return CallWindowProc(TO_WNDPROC(wc.lpfnWndProc), hWnd, Msg, wParam, lParam);
}

LRESULT FUNC_CALLBACK EditHookEnterProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	WNDCLASS  wc;

	switch(Msg)
	{
	case WM_KEYDOWN:
		// inform parent window via WM_COMMAND msg about ENTER key occurrence
		switch(wParam)
		{
		case VK_RETURN:
			SendMessage(
				GetParent(hWnd),
				WM_COMMAND,
				MAKEWPARAM(GetDlgCtrlID(hWnd), NM_HEXEDITENTER),
				(LPARAM)hWnd);
			return 0;

		//case VK_ESCAPE:
			// if the control handles this key it'll close the parent dlg - dunno why :(
		//	return 0;
		}
		break;
	}

	char clsName[256];
	GetClassName(hWnd, clsName, sizeof(clsName));
	GetClassInfo(NULL, clsName, &wc);

	return CallWindowProc(TO_WNDPROC(wc.lpfnWndProc), hWnd, Msg, wParam, lParam);
}

//
// convert a hex number string to a DW
//
BOOL HexStrToInt(char *szHexStr, DWORD *pdwHexVal)
{
	ULONGLONG qwVal;

	if (!HexStrToInt64(szHexStr, &qwVal))
		return FALSE;
	if (qwVal > (ULONGLONG)0xFFFFFFFFUL)
		return FALSE; // out of 32-bit range

	*pdwHexVal = (DWORD)qwVal;
	return TRUE;
}

//
// convert a hex number string to a 64-bit value (up to 16 hex digits).
// Used for offsets/sizes so files >4GB (9+ hex digits) work.
// Empty string yields 0 (preserves old HexStrToInt behaviour for callers
// that treat empty as 0).
//
BOOL HexStrToInt64(char *szHexStr, ULONGLONG *pqwHexVal)
{
	char *pCH, c;
	ULONGLONG qwVal = 0, qwDigit;
	int nDigits = 0;

	if (!szHexStr || !pqwHexVal)
		return FALSE;

	pCH = szHexStr;
	while (*pCH)
	{
		c = toupper(*pCH++);
		if (c >= 'A' && c <= 'F')
			qwDigit = (ULONGLONG)c - ((ULONGLONG)'A' - 10);
		else if (c >= '0' && c <= '9')
			qwDigit = (ULONGLONG)c - (ULONGLONG)'0';
		else
			return FALSE; // invalid hex char
		// overflow check: 16 hex digits max for 64-bit
		if (nDigits >= 16)
			return FALSE;
		qwVal = (qwVal << 4) + qwDigit;
		nDigits++;
	}

	*pqwHexVal = qwVal;
	return TRUE;
}

/*
 * PE helpers below parse the on-disk headers explicitly (fixed offsets +
 * memcpy) instead of IMAGE_NT_HEADERS, which switches between the 32-bit
 * and 64-bit layout depending on the BUILD architecture. Using it made a
 * 64-bit 16Edit misread 32-bit PEs (and a 32-bit 16Edit misread 64-bit
 * PEs), because ImageBase lives at a different offset/size in PE32
 * (DWORD at opt+28) vs PE32+ (QWORD at opt+24). The magic decides.
 */

#ifndef IMAGE_NT_OPTIONAL_HDR32_MAGIC
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#endif
#ifndef IMAGE_NT_OPTIONAL_HDR64_MAGIC
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#endif

// Windows loader supports at most 96 sections; cap the walk so a
// malformed header can't send us off into garbage.
#define PE_MAX_SECTIONS 96

static BOOL pe_bad(const void *p, SIZE_T cb) {
	if (!p || cb == 0)
		return TRUE;
	return IsBadReadPtr((CONST VOID*)p, (UINT_PTR)cb) ? TRUE : FALSE;
}

// Parse NT/file header location + section count. Returns FALSE when the
// buffer doesn't hold a readable PE header. On success fills lfanew,
// section count, optional-header size and the section table base.
static BOOL pe_headers(char *base, LONG *pFanew, WORD *pNsec, WORD *pOptSize, ULONGLONG *pSecBase) {
	WORD wMagic;
	LONG lFanew;
	DWORD dwSig;
	WORD wNsec, wOptSize;

	if (!base)
		return FALSE;
	if (pe_bad(base, 2))
		return FALSE;
	memcpy(&wMagic, base, 2); // e_magic at 0
	if (wMagic != IMAGE_DOS_SIGNATURE)
		return FALSE;
	if (pe_bad(base + 0x3C, 4))
		return FALSE;
	memcpy(&lFanew, base + 0x3C, 4); // e_lfanew
	if (lFanew < 0 || lFanew > 0x1000000)
		return FALSE;
	// Signature + Machine + NumberOfSections
	if (pe_bad(base + (SIZE_T)lFanew, 8))
		return FALSE;
	memcpy(&dwSig, base + lFanew, 4);
	if (dwSig != IMAGE_NT_SIGNATURE)
		return FALSE;
	memcpy(&wNsec, base + lFanew + 4 + 2, 2);
	memcpy(&wOptSize, base + lFanew + 4 + 16, 2);
	if (wNsec > PE_MAX_SECTIONS)
		return FALSE;
	if (pFanew)
		*pFanew = lFanew;
	if (pNsec)
		*pNsec = wNsec;
	if (pOptSize)
		*pOptSize = wOptSize;
	if (pSecBase)
		*pSecBase = (ULONGLONG)lFanew + 4 + sizeof(IMAGE_FILE_HEADER) + (ULONGLONG)wOptSize;
	return TRUE;
}

// Read ImageBase per optional-header magic. Returns FALSE on unknown magic
// or unreadable bytes.
static BOOL pe_imagebase(char *base, LONG lFanew, ULONGLONG *pBase) {
	WORD wMagic;
	ULONGLONG qwOpt = (ULONGLONG)lFanew + 4 + sizeof(IMAGE_FILE_HEADER);

	if (pe_bad(base + (SIZE_T)qwOpt, 2))
		return FALSE;
	memcpy(&wMagic, base + (SIZE_T)qwOpt, 2);
	if (wMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
		DWORD dwBase;
		if (pe_bad(base + (SIZE_T)(qwOpt + 28), 4))
			return FALSE;
		memcpy(&dwBase, base + (SIZE_T)(qwOpt + 28), 4);
		*pBase = (ULONGLONG)dwBase;
		return TRUE;
	} else if (wMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
		ULONGLONG qwBase;
		if (pe_bad(base + (SIZE_T)(qwOpt + 24), 8))
			return FALSE;
		memcpy(&qwBase, base + (SIZE_T)(qwOpt + 24), 8);
		*pBase = qwBase;
		return TRUE;
	}
	return FALSE;
}

/*
 * Return DWORD
 * 		2 : EXE
 * 		1 : DLL
 * 		0 : NOT PE
 * (FileHeader layout is identical for PE32/PE32+, so this is
 * architecture-independent.)
 */
DWORD file_type(char *base) {
	LONG lFanew;
	WORD wChars;

	if (!pe_headers(base, &lFanew, NULL, NULL, NULL))
		return 0;

	// Characteristics follows NumberOfSections/SizeOfOptionalHeader in FileHeader.
	if (pe_bad(base + (SIZE_T)lFanew + 4 + 18, 2))
		return 0;
	memcpy(&wChars, base + lFanew + 4 + 18, 2);

	if (wChars & IMAGE_FILE_DLL) {
		return 1;
	} else {
		return 2;
	}
}

#define isin(address,start,length) ((address)>=(start) && (address)<(start)+(length))

// Lowest PointerToRawData among sections carrying raw data. Bytes below
// it are image headers, which the loader maps at ImageBase.
static BOOL pe_min_raw(char *base, ULONGLONG qwSecBase, WORD wNsec, ULONGLONG *pMin) {
	ULONGLONG qwMin = (ULONGLONG)-1;
	WORD sect;
	BOOL bAny = FALSE;

	for (sect = 0; sect < wNsec; sect++) {
		ULONGLONG qwEnt = qwSecBase + (ULONGLONG)sect * 40;
		DWORD dwRaw, dwRawSize;

		if (pe_bad(base + (SIZE_T)qwEnt, 40))
			break;
		memcpy(&dwRawSize, base + (SIZE_T)(qwEnt + 16), 4); // SizeOfRawData
		memcpy(&dwRaw, base + (SIZE_T)(qwEnt + 20), 4);     // PointerToRawData
		if (dwRawSize > 0 && (ULONGLONG)dwRaw < qwMin) {
			qwMin = (ULONGLONG)dwRaw;
			bAny = TRUE;
		}
	}
	if (!bAny)
		return FALSE;
	*pMin = qwMin;
	return TRUE;
}

// Highest image-relative end (VirtualAddress + data) over all sections.
ULONGLONG pe_max_va(char *base) {
	LONG lFanew;
	WORD wNsec;
	ULONGLONG qwImageBase, qwSecBase, qwMax = 0;
	WORD sect;

	if (!pe_headers(base, &lFanew, &wNsec, NULL, &qwSecBase))
		return 0;
	if (!pe_imagebase(base, lFanew, &qwImageBase))
		return 0;

	for (sect = 0; sect < wNsec; sect++) {
		ULONGLONG qwEnt = qwSecBase + (ULONGLONG)sect * 40;
		DWORD dwVA, dwRawSize, dwVirtSize;
		ULONGLONG qwEnd;

		if (pe_bad(base + (SIZE_T)qwEnt, 40))
			break;
		memcpy(&dwVirtSize, base + (SIZE_T)(qwEnt + 8), 4);  // VirtualSize
		memcpy(&dwVA, base + (SIZE_T)(qwEnt + 12), 4);       // VirtualAddress
		memcpy(&dwRawSize, base + (SIZE_T)(qwEnt + 16), 4);  // SizeOfRawData
		qwEnd = (ULONGLONG)dwVA +
			((ULONGLONG)dwVirtSize > (ULONGLONG)dwRawSize ?
			 (ULONGLONG)dwVirtSize : (ULONGLONG)dwRawSize);
		if (qwEnd > qwMax)
			qwMax = qwEnd;
	}
	if (qwMax > (ULONGLONG)-1 - qwImageBase)
		return (ULONGLONG)-1; // saturate on absurd headers
	return qwImageBase + qwMax;
}

/*
 * Get the vitual offset from file offset (64-bit for files >4GB).
 * ImageBase and section walks follow the on-disk magic, so 32-bit and
 * 64-bit PEs translate correctly regardless of the 16Edit build.
 * Header bytes below the first raw-data section map to ImageBase+offset
 * (where the loader puts them); bytes past the last section (e.g.
 * Authenticode overlay) have no VA and pass through as file offsets.
 */
ULONGLONG get_va(char *base, ULONGLONG file_offset) {
	LONG lFanew;
	WORD wNsec;
	ULONGLONG qwImageBase, qwSecBase, qwMinRaw;
	WORD sect;

	if (!pe_headers(base, &lFanew, &wNsec, NULL, &qwSecBase))
		return file_offset;
	if (!pe_imagebase(base, lFanew, &qwImageBase))
		return file_offset;

	if (pe_min_raw(base, qwSecBase, wNsec, &qwMinRaw) &&
		file_offset < qwMinRaw)
		return qwImageBase + file_offset;
	if (!pe_imagebase(base, lFanew, &qwImageBase))
		return file_offset;

	for (sect = 0; sect < wNsec; sect++) {
		ULONGLONG qwEnt = qwSecBase + (ULONGLONG)sect * 40;
		DWORD dwVA, dwRaw, dwRawSize;

		if (pe_bad(base + (SIZE_T)qwEnt, 40))
			break;
		memcpy(&dwVA, base + (SIZE_T)(qwEnt + 12), 4);      // VirtualAddress
		memcpy(&dwRawSize, base + (SIZE_T)(qwEnt + 16), 4); // SizeOfRawData
		memcpy(&dwRaw, base + (SIZE_T)(qwEnt + 20), 4);     // PointerToRawData
		if (isin(file_offset, (ULONGLONG)dwRaw, (ULONGLONG)dwRawSize)) {
			return (ULONGLONG)dwVA +
				file_offset - (ULONGLONG)dwRaw + qwImageBase;
		}
	}
	return file_offset;
}

/*
 * Get the file offset from vitual offset (64-bit for files >4GB).
 * ImageBase and section walks follow the on-disk magic, so 32-bit and
 * 64-bit PEs translate correctly regardless of the 16Edit build.
 * Header VAs below the first section map back to file offsets so
 * get_va/get_fo round-trip; overlay VAs fall through unchanged.
 */
ULONGLONG get_fo(char *base, ULONGLONG va_offset) {
	LONG lFanew;
	WORD wNsec;
	ULONGLONG qwImageBase, qwSecBase, qwMinRaw;
	ULONGLONG	va;
	WORD sect;

	if (!pe_headers(base, &lFanew, &wNsec, NULL, &qwSecBase))
		return va_offset;
	if (!pe_imagebase(base, lFanew, &qwImageBase))
		return va_offset;

	if (va_offset < qwImageBase)
		return va_offset;
	va = va_offset - qwImageBase;

	if (pe_min_raw(base, qwSecBase, wNsec, &qwMinRaw) && va < qwMinRaw)
		return va;

	for (sect = 0; sect < wNsec; sect++) {
		ULONGLONG qwEnt = qwSecBase + (ULONGLONG)sect * 40;
		DWORD dwVA, dwRaw, dwRawSize;

		if (pe_bad(base + (SIZE_T)qwEnt, 40))
			break;
		memcpy(&dwVA, base + (SIZE_T)(qwEnt + 12), 4);      // VirtualAddress
		memcpy(&dwRawSize, base + (SIZE_T)(qwEnt + 16), 4); // SizeOfRawData
		memcpy(&dwRaw, base + (SIZE_T)(qwEnt + 20), 4);     // PointerToRawData
		if (isin(va, (ULONGLONG)dwVA, (ULONGLONG)dwRawSize)) {
			return (ULONGLONG)dwRaw + va - (ULONGLONG)dwVA;
		}
	}
	return va_offset;
}

#if _MSC_VER < 1200
LONGLONG _atoi64 (const char * nptr) {
  char *s = (char *)nptr;
  LONGLONG acc = 0;
  int neg = 0;
 
  if (nptr == NULL)
    return 0;
 
  while(isspace((int)*s))
    s++;
  if (*s == '-')
    {
      neg = 1;
      s++;
    }
  else if (*s == '+')
    s++;
 
  while (isdigit((int)*s))
    {
      acc = 10 * acc + ((int)*s - '0');
      s++;
    }
 
  if (neg)
    acc *= -1;
  return acc;
}
#endif
