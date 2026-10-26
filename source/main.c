/*
 * ============================================================================
 *  ImageToHTML Payload Converter
 * ----------------------------------------------------------------------------
 *  Purpose     : Loads an image file (PNG/JPG/JPEG/GIF/BMP/WEBP/SVG), encodes
 *                its raw bytes into a Base64 string, and wraps it into a
 *                ready-to-paste HTML/CSS "data:" payload
 *                (e.g.  <img src="data:image/png;base64,....." />).
 *
 *  Language    : ANSI C (C11), Win32 API only - no external dependencies.
 *  Target      : Windows 7 and newer, x86 / x64.
 *
 *  Developer   : Demiyan Dissanayake
 *  Support     : https://github.com/dexel-software-solutions
 *  Contact     : demiyandissanayake@gmail.com
 *                dexelsoftwaresolutions@gmail.com
 *
 *  License     : Free to use / modify for personal and commercial projects.
 *                Please keep the developer credit in the About dialog.
 * ============================================================================
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef _MSC_VER
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

/* ---------------------------------------------------------------------- */
/*  Application constants                                                 */
/* ---------------------------------------------------------------------- */

#define APP_TITLE         L"ImageToHTML Payload Converter"
#define APP_VERSION       L"1.0.0"
#define DEV_NAME          L"Demiyan Dissanayake"
#define DEV_SUPPORT_URL   L"https://github.com/dexel-software-solutions"
#define DEV_EMAIL_1       L"demiyandissanayake@gmail.com"
#define DEV_EMAIL_2       L"dexelsoftwaresolutions@gmail.com"

#define ID_BTN_BROWSE     1001
#define ID_BTN_CONVERT    1002
#define ID_BTN_COPY       1003
#define ID_BTN_SAVE_HTML  1004
#define ID_BTN_SAVE_TXT   1005
#define ID_BTN_CLEAR      1006
#define ID_EDIT_FILEPATH  1007
#define ID_EDIT_OUTPUT    1008
#define ID_COMBO_TAG      1009
#define ID_STATIC_INFO    1010
#define ID_STATIC_SIZE    1011
#define ID_MENU_ABOUT     2001
#define ID_MENU_EXIT      2002
#define ID_MENU_OPEN      2003
#define ID_MENU_SAVE      2004

/* ---------------------------------------------------------------------- */
/*  Globals                                                                */
/* ---------------------------------------------------------------------- */

static HWND g_hMainWnd      = NULL;
static HWND g_hEditPath     = NULL;
static HWND g_hEditOutput   = NULL;
static HWND g_hComboTag     = NULL;
static HWND g_hStaticInfo   = NULL;
static HWND g_hStaticSize   = NULL;
static HFONT g_hFont        = NULL;
static HFONT g_hFontBold    = NULL;

static wchar_t g_selectedFile[MAX_PATH] = L"";
static char   *g_base64Payload = NULL;   /* full data:...;base64,... string */
static size_t  g_payloadLen    = 0;

/* ---------------------------------------------------------------------- */
/*  Base64 encoding (RFC 4648) - self contained, no external libraries     */
/* ---------------------------------------------------------------------- */

static const char g_b64Table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Encodes 'inLen' bytes from 'in' into a newly malloc'd NUL terminated
 * Base64 string. Caller must free() the returned pointer.                 */
static char *Base64Encode(const unsigned char *in, size_t inLen, size_t *outLen)
{
    size_t encLen = 4 * ((inLen + 2) / 3);
    char *out = (char *)malloc(encLen + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    unsigned char a3[3];
    unsigned char a4[4];

    while (inLen > 0) {
        size_t chunk = (inLen >= 3) ? 3 : inLen;
        for (i = 0; i < chunk; i++) a3[i] = in[i];
        for (i = chunk; i < 3; i++) a3[i] = 0;

        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;

        for (i = 0; i < chunk + 1; i++) out[j++] = g_b64Table[a4[i]];
        for (i = chunk + 1; i < 4; i++) out[j++] = '=';

        in += chunk;
        inLen -= chunk;
    }

    out[j] = '\0';
    if (outLen) *outLen = j;
    return out;
}

/* ---------------------------------------------------------------------- */
/*  Helpers                                                                */
/* ---------------------------------------------------------------------- */

/* Returns a static pointer (no need to free) to the MIME type string that
 * matches the file extension of 'path'. Defaults to octet-stream.         */
static const wchar_t *GetMimeTypeFromExt(const wchar_t *path)
{
    const wchar_t *ext = wcsrchr(path, L'.');
    if (!ext) return L"application/octet-stream";
    ext++;

    if (_wcsicmp(ext, L"png")  == 0) return L"image/png";
    if (_wcsicmp(ext, L"jpg")  == 0) return L"image/jpeg";
    if (_wcsicmp(ext, L"jpeg") == 0) return L"image/jpeg";
    if (_wcsicmp(ext, L"gif")  == 0) return L"image/gif";
    if (_wcsicmp(ext, L"bmp")  == 0) return L"image/bmp";
    if (_wcsicmp(ext, L"webp") == 0) return L"image/webp";
    if (_wcsicmp(ext, L"svg")  == 0) return L"image/svg+xml";
    if (_wcsicmp(ext, L"ico")  == 0) return L"image/x-icon";
    if (_wcsicmp(ext, L"tiff") == 0) return L"image/tiff";
    if (_wcsicmp(ext, L"tif")  == 0) return L"image/tiff";

    return L"application/octet-stream";
}

/* Reads the full content of a file into a malloc'd buffer. Returns NULL
 * on failure. 'outSize' receives the byte count read.                     */
static unsigned char *ReadFileBytes(const wchar_t *path, size_t *outSize)
{
    HANDLE hFile = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(hFile, &size)) {
        CloseHandle(hFile);
        return NULL;
    }

    size_t fileSize = (size_t)size.QuadPart;
    unsigned char *buffer = (unsigned char *)malloc(fileSize ? fileSize : 1);
    if (!buffer) {
        CloseHandle(hFile);
        return NULL;
    }

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, buffer, (DWORD)fileSize, &bytesRead, NULL);
    CloseHandle(hFile);

    if (!ok || bytesRead != fileSize) {
        free(buffer);
        return NULL;
    }

    if (outSize) *outSize = fileSize;
    return buffer;
}

/* Formats a byte count as a human readable string (B / KB / MB).          */
static void FormatFileSize(size_t bytes, wchar_t *out, size_t outCap)
{
    double b = (double)bytes;
    if (b < 1024.0)
        swprintf(out, outCap, L"%.0f B", b);
    else if (b < 1024.0 * 1024.0)
        swprintf(out, outCap, L"%.2f KB", b / 1024.0);
    else
        swprintf(out, outCap, L"%.2f MB", b / (1024.0 * 1024.0));
}

static void SetStatus(const wchar_t *msg)
{
    SetWindowTextW(g_hStaticInfo, msg);
}

/* Builds the final wrapped payload string (data URI, optional HTML tag)
 * based on the raw base64 body and the user's selected wrapper mode.      */
static void BuildWrappedOutput(const char *base64Body, const wchar_t *mime,
                                int comboSel, char **outStr)
{
    size_t bodyLen = strlen(base64Body);

    /* UTF-8 narrow mime for building the C string */
    char mimeA[64];
    WideCharToMultiByte(CP_UTF8, 0, mime, -1, mimeA, sizeof(mimeA), NULL, NULL);

    char *prefix = NULL;
    char *suffix = NULL;

    switch (comboSel) {
        case 0: /* Raw data URI only */
            prefix = "data:"; break;
        case 1: /* <img> tag */
            prefix = "<img src=\"data:"; break;
        case 2: /* CSS background-image */
            prefix = "background-image: url(\"data:"; break;
        case 3: /* Plain base64 only (no prefix) */
            prefix = ""; break;
        default:
            prefix = "data:"; break;
    }

    switch (comboSel) {
        case 1: suffix = "\" alt=\"\" />"; break;
        case 2: suffix = "\");"; break;
        default: suffix = ""; break;
    }

    size_t midLen = (comboSel == 3) ? 0 : (strlen(mimeA) + strlen(";base64,"));
    size_t totalLen = strlen(prefix) + midLen + bodyLen + strlen(suffix) + 1;

    char *result = (char *)malloc(totalLen);
    if (!result) { *outStr = NULL; return; }

    result[0] = '\0';
    strcat(result, prefix);
    if (comboSel != 3) {
        strcat(result, mimeA);
        strcat(result, ";base64,");
    }
    strcat(result, base64Body);
    strcat(result, suffix);

    *outStr = result;
}

/* Converts a narrow (UTF-8/ASCII) C string to a wide string for display
 * in the Win32 Edit control. Caller must free() the result.               */
static wchar_t *NarrowToWide(const char *s)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *w = (wchar_t *)malloc(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, len);
    return w;
}

/* ---------------------------------------------------------------------- */
/*  File dialogs                                                           */
/* ---------------------------------------------------------------------- */

static BOOL BrowseForImage(wchar_t *outPath, DWORD outPathCap)
{
    OPENFILENAMEW ofn;
    wchar_t fileBuf[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = g_hMainWnd;
    ofn.lpstrFilter  = L"Image Files\0*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.webp;*.svg;*.ico;*.tiff\0"
                        L"All Files\0*.*\0\0";
    ofn.lpstrFile    = fileBuf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle   = L"Select an image to convert";

    if (GetOpenFileNameW(&ofn)) {
        wcsncpy(outPath, fileBuf, outPathCap - 1);
        outPath[outPathCap - 1] = L'\0';
        return TRUE;
    }
    return FALSE;
}

static BOOL SaveTextToFile(const wchar_t *defaultExt, const wchar_t *filter,
                            const char *content)
{
    OPENFILENAMEW ofn;
    wchar_t fileBuf[MAX_PATH] = L"output";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = g_hMainWnd;
    ofn.lpstrFilter  = filter;
    ofn.lpstrFile    = fileBuf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrDefExt  = defaultExt;
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle   = L"Save payload as";

    if (!GetSaveFileNameW(&ofn)) return FALSE;

    HANDLE hFile = CreateFileW(fileBuf, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD written = 0;
    WriteFile(hFile, content, (DWORD)strlen(content), &written, NULL);
    CloseHandle(hFile);
    return TRUE;
}

/* ---------------------------------------------------------------------- */
/*  Core conversion routine                                                 */
/* ---------------------------------------------------------------------- */

static void DoConvert(void)
{
    if (wcslen(g_selectedFile) == 0) {
        MessageBoxW(g_hMainWnd, L"Please select an image file first.",
                    APP_TITLE, MB_OK | MB_ICONWARNING);
        return;
    }

    size_t rawSize = 0;
    unsigned char *raw = ReadFileBytes(g_selectedFile, &rawSize);
    if (!raw) {
        MessageBoxW(g_hMainWnd, L"Could not read the selected file.",
                    APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }

    size_t b64Len = 0;
    char *b64 = Base64Encode(raw, rawSize, &b64Len);
    free(raw);

    if (!b64) {
        MessageBoxW(g_hMainWnd, L"Base64 encoding failed (out of memory).",
                    APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }

    const wchar_t *mime = GetMimeTypeFromExt(g_selectedFile);
    int comboSel = (int)SendMessageW(g_hComboTag, CB_GETCURSEL, 0, 0);

    char *wrapped = NULL;
    BuildWrappedOutput(b64, mime, comboSel, &wrapped);
    free(b64);

    if (!wrapped) {
        MessageBoxW(g_hMainWnd, L"Failed to build output payload.",
                    APP_TITLE, MB_OK | MB_ICONERROR);
        return;
    }

    if (g_base64Payload) { free(g_base64Payload); }
    g_base64Payload = wrapped;
    g_payloadLen = strlen(wrapped);

    wchar_t *wideOut = NarrowToWide(wrapped);
    SetWindowTextW(g_hEditOutput, wideOut);
    free(wideOut);

    wchar_t sizeBuf[64], statusBuf[256];
    FormatFileSize(rawSize, sizeBuf, 64);
    swprintf(statusBuf, 256,
             L"Converted successfully  |  Source: %s  |  Payload: %zu characters",
             sizeBuf, g_payloadLen);
    SetStatus(statusBuf);
}

static void DoCopyToClipboard(void)
{
    if (!g_base64Payload) {
        MessageBoxW(g_hMainWnd, L"Nothing to copy yet. Convert an image first.",
                    APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t *wide = NarrowToWide(g_base64Payload);
    size_t wideBytes = (wcslen(wide) + 1) * sizeof(wchar_t);

    if (!OpenClipboard(g_hMainWnd)) { free(wide); return; }
    EmptyClipboard();

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wideBytes);
    if (hMem) {
        void *dst = GlobalLock(hMem);
        memcpy(dst, wide, wideBytes);
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
    free(wide);

    SetStatus(L"Payload copied to clipboard.");
}

static void DoSaveAsHtml(void)
{
    if (!g_base64Payload) {
        MessageBoxW(g_hMainWnd, L"Nothing to save yet. Convert an image first.",
                    APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }

    char *doc = (char *)malloc(g_payloadLen + 1024);
    sprintf(doc,
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>Embedded Image Payload</title>\n"
        "</head>\n"
        "<body>\n"
        "<!-- Generated by ImageToHTML Payload Converter - Dexel Software Solutions -->\n"
        "%s\n"
        "</body>\n"
        "</html>\n",
        g_base64Payload);

    if (SaveTextToFile(L"html", L"HTML File\0*.html\0All Files\0*.*\0\0", doc))
        SetStatus(L"Saved as HTML file.");

    free(doc);
}

static void DoSaveAsText(void)
{
    if (!g_base64Payload) {
        MessageBoxW(g_hMainWnd, L"Nothing to save yet. Convert an image first.",
                    APP_TITLE, MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (SaveTextToFile(L"txt", L"Text File\0*.txt\0All Files\0*.*\0\0", g_base64Payload))
        SetStatus(L"Saved as text file.");
}

static void DoClear(void)
{
    g_selectedFile[0] = L'\0';
    if (g_base64Payload) { free(g_base64Payload); g_base64Payload = NULL; }
    g_payloadLen = 0;
    SetWindowTextW(g_hEditPath, L"");
    SetWindowTextW(g_hEditOutput, L"");
    SetStatus(L"Ready. Select an image to begin.");
}

/* ---------------------------------------------------------------------- */
/*  About box                                                               */
/* ---------------------------------------------------------------------- */

static void ShowAboutBox(HWND hParent)
{
    wchar_t msg[1024];
    swprintf(msg, 1024,
        L"%s\n"
        L"Version %s\n\n"
        L"Converts image files into Base64-encoded HTML/CSS payloads\n"
        L"(data URIs) that can be embedded directly in web pages,\n"
        L"stylesheets, or any markup that accepts a \"data:\" URL.\n\n"
        L"----------------------------------------\n"
        L"Developer   : %s\n"
        L"Support     : %s\n"
        L"Email       : %s\n"
        L"              %s\n"
        L"----------------------------------------\n\n"
        L"\u00A9 2026 Dexel Software Solutions. All rights reserved.",
        APP_TITLE, APP_VERSION, DEV_NAME, DEV_SUPPORT_URL, DEV_EMAIL_1, DEV_EMAIL_2);

    MessageBoxW(hParent, msg, L"About - ImageToHTML Payload Converter",
                MB_OK | MB_ICONINFORMATION);
}

/* ---------------------------------------------------------------------- */
/*  Layout / control creation                                              */
/* ---------------------------------------------------------------------- */

static void CreateControls(HWND hwnd)
{
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    g_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_hFontBold = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    /* --- Row 1 : file path + browse ------------------------------------ */
    HWND lblFile = CreateWindowW(L"STATIC", L"Image file:",
        WS_CHILD | WS_VISIBLE, 16, 18, 90, 22, hwnd, NULL, hInst, NULL);

    g_hEditPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
        110, 15, 460, 24, hwnd, (HMENU)(INT_PTR)ID_EDIT_FILEPATH, hInst, NULL);

    HWND btnBrowse = CreateWindowW(L"BUTTON", L"Browse...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        580, 14, 100, 26, hwnd, (HMENU)(INT_PTR)ID_BTN_BROWSE, hInst, NULL);

    /* --- Row 2 : output format + convert -------------------------------- */
    HWND lblFormat = CreateWindowW(L"STATIC", L"Output format:",
        WS_CHILD | WS_VISIBLE, 16, 54, 90, 22, hwnd, NULL, hInst, NULL);

    g_hComboTag = CreateWindowW(L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        110, 50, 300, 200, hwnd, (HMENU)(INT_PTR)ID_COMBO_TAG, hInst, NULL);
    SendMessageW(g_hComboTag, CB_ADDSTRING, 0, (LPARAM)L"Raw data: URI");
    SendMessageW(g_hComboTag, CB_ADDSTRING, 0, (LPARAM)L"HTML <img> tag");
    SendMessageW(g_hComboTag, CB_ADDSTRING, 0, (LPARAM)L"CSS background-image");
    SendMessageW(g_hComboTag, CB_ADDSTRING, 0, (LPARAM)L"Plain Base64 (no wrapper)");
    SendMessageW(g_hComboTag, CB_SETCURSEL, 1, 0);

    HWND btnConvert = CreateWindowW(L"BUTTON", L"Convert to Payload",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        420, 49, 150, 26, hwnd, (HMENU)(INT_PTR)ID_BTN_CONVERT, hInst, NULL);

    HWND btnClear = CreateWindowW(L"BUTTON", L"Clear",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        580, 49, 100, 26, hwnd, (HMENU)(INT_PTR)ID_BTN_CLEAR, hInst, NULL);

    /* --- Row 3 : output label -------------------------------------------- */
    HWND lblOutput = CreateWindowW(L"STATIC", L"Generated payload (editable / selectable):",
        WS_CHILD | WS_VISIBLE, 16, 88, 350, 20, hwnd, NULL, hInst, NULL);

    /* --- Output text box --------------------------------------------------- */
    g_hEditOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
        16, 110, 664, 300, hwnd, (HMENU)(INT_PTR)ID_EDIT_OUTPUT, hInst, NULL);

    /* --- Action buttons row -------------------------------------------- */
    HWND btnCopy = CreateWindowW(L"BUTTON", L"Copy to Clipboard",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        16, 420, 160, 30, hwnd, (HMENU)(INT_PTR)ID_BTN_COPY, hInst, NULL);

    HWND btnSaveHtml = CreateWindowW(L"BUTTON", L"Save as .html",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        186, 420, 130, 30, hwnd, (HMENU)(INT_PTR)ID_BTN_SAVE_HTML, hInst, NULL);

    HWND btnSaveTxt = CreateWindowW(L"BUTTON", L"Save as .txt",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        326, 420, 130, 30, hwnd, (HMENU)(INT_PTR)ID_BTN_SAVE_TXT, hInst, NULL);

    /* --- Status bar style label ------------------------------------------ */
    g_hStaticInfo = CreateWindowW(L"STATIC", L"Ready. Select an image to begin.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        16, 462, 664, 20, hwnd, (HMENU)(INT_PTR)ID_STATIC_INFO, hInst, NULL);

    g_hStaticSize = CreateWindowW(L"STATIC", L"Developed by Demiyan Dissanayake  |  Dexel Software Solutions",
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        16, 486, 664, 20, hwnd, (HMENU)(INT_PTR)ID_STATIC_SIZE, hInst, NULL);

    /* Apply font to every child control */
    HWND children[] = { lblFile, g_hEditPath, btnBrowse, lblFormat, g_hComboTag,
                         btnConvert, btnClear, lblOutput, g_hEditOutput, btnCopy,
                         btnSaveHtml, btnSaveTxt, g_hStaticInfo };
    for (size_t i = 0; i < sizeof(children) / sizeof(children[0]); i++)
        SendMessageW(children[i], WM_SETFONT, (WPARAM)g_hFont, TRUE);
    SendMessageW(g_hStaticSize, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
}

/* ---------------------------------------------------------------------- */
/*  Main window menu                                                       */
/* ---------------------------------------------------------------------- */

static HMENU CreateAppMenu(void)
{
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hHelpMenu = CreatePopupMenu();

    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_OPEN, L"&Open Image...\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_SAVE, L"&Save Payload as HTML...\tCtrl+S");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hFileMenu, MF_STRING, ID_MENU_EXIT, L"E&xit");

    AppendMenuW(hHelpMenu, MF_STRING, ID_MENU_ABOUT, L"&About...");

    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

    return hMenuBar;
}

/* ---------------------------------------------------------------------- */
/*  Window procedure                                                       */
/* ---------------------------------------------------------------------- */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            CreateControls(hwnd);
            DragAcceptFiles(hwnd, TRUE);
            return 0;

        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            if (DragQueryFileW(hDrop, 0, g_selectedFile, MAX_PATH)) {
                SetWindowTextW(g_hEditPath, g_selectedFile);
                SetStatus(L"Image dropped. Click \"Convert to Payload\" to continue.");
            }
            DragFinish(hDrop);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_BROWSE:
                case ID_MENU_OPEN:
                    if (BrowseForImage(g_selectedFile, MAX_PATH)) {
                        SetWindowTextW(g_hEditPath, g_selectedFile);
                        SetStatus(L"Image selected. Click \"Convert to Payload\" to continue.");
                    }
                    return 0;

                case ID_BTN_CONVERT:
                    DoConvert();
                    return 0;

                case ID_BTN_COPY:
                    DoCopyToClipboard();
                    return 0;

                case ID_BTN_SAVE_HTML:
                case ID_MENU_SAVE:
                    DoSaveAsHtml();
                    return 0;

                case ID_BTN_SAVE_TXT:
                    DoSaveAsText();
                    return 0;

                case ID_BTN_CLEAR:
                    DoClear();
                    return 0;

                case ID_MENU_ABOUT:
                    ShowAboutBox(hwnd);
                    return 0;

                case ID_MENU_EXIT:
                    DestroyWindow(hwnd);
                    return 0;
            }
            break;

        case WM_SIZE: {
            /* Keep the output box growing with the window */
            RECT rc;
            GetClientRect(hwnd, &rc);
            int w = rc.right - 32;
            int h = rc.bottom - 140;
            if (h < 60) h = 60;
            if (g_hEditOutput) MoveWindow(g_hEditOutput, 16, 110, w, h, TRUE);
            if (g_hStaticInfo) MoveWindow(g_hStaticInfo, 16, 110 + h + 42, w, 20, TRUE);
            if (g_hStaticSize) MoveWindow(g_hStaticSize, 16, 110 + h + 66, w, 20, TRUE);
            return 0;
        }

        case WM_DESTROY:
            if (g_base64Payload) free(g_base64Payload);
            if (g_hFont) DeleteObject(g_hFont);
            if (g_hFontBold) DeleteObject(g_hFontBold);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ---------------------------------------------------------------------- */
/*  Entry point                                                            */
/* ---------------------------------------------------------------------- */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC  = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"ImageToHTMLPayloadConverterWndClass";

    WNDCLASSEXW wc = { 0 };
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(1)); /* from .rc */
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.lpszMenuName  = NULL;

    if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = LoadIconW(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Window registration failed.", L"Error", MB_ICONERROR);
        return 1;
    }

    g_hMainWnd = CreateWindowExW(
        0, CLASS_NAME, APP_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 600,
        NULL, NULL, hInstance, NULL);

    if (!g_hMainWnd) {
        MessageBoxW(NULL, L"Window creation failed.", L"Error", MB_ICONERROR);
        return 1;
    }

    SetMenu(g_hMainWnd, CreateAppMenu());
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
