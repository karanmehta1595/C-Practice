#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define IDC_DISPLAY 1002

typedef struct {
    RECT rect;
    char label[12];
    int id;
} GlassButton;

GlassButton buttons[60];
int buttonCount = 0;

// Core States
char displayBuffer[64] = "0";
double current_val = 0.0;
double memory_reg = 0.0;
char last_op = 0;
BOOL start_new_num = TRUE;

COLORREF glass_bg, glass_btn, text_color, accent_orange;
HBRUSH hDisplayBrush = NULL;
RECT rcClose, rcMin, rcMax;

void SyncSystemTheme() {
    HKEY hKey;
    DWORD val = 0; // Default Dark
    DWORD size = sizeof(val);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueEx(hKey, "AppsUseLightTheme", NULL, NULL, (LPBYTE)&val, &size);
        RegCloseKey(hKey);
    }
    if (val == 0) {
        glass_bg = RGB(20, 20, 22);
        glass_btn = RGB(36, 36, 40);
        text_color = RGB(245, 245, 250);
        accent_orange = RGB(255, 159, 10);
    } else {
        glass_bg = RGB(240, 240, 245);
        glass_btn = RGB(218, 218, 222);
        text_color = RGB(20, 20, 25);
        accent_orange = RGB(242, 115, 12);
    }
    if (hDisplayBrush) DeleteObject(hDisplayBrush);
    hDisplayBrush = CreateSolidBrush(glass_bg);
}

// Pixel-Perfect Ultra Compact Layout Matrix Framework
void BuildLayoutGrid() {
    buttonCount = 0;
    
    // Exact Casio Mathematical Key Grid Setup (Packed Matrix)
    int startY = 110;
    int btnW = 54;
    int btnH = 32;
    int gapX = 6;  // Gaps tight kar diye taaki free space na bache
    int gapY = 6;

    // Row 1: Casio Top Control Block
    char row1[6][8] = {"SHIFT", "ALPHA", "REPLAY", "MODE", "CLR", "ON"};
    for(int i=0; i<6; i++) {
        int x = 12 + i * (btnW + gapX);
        buttons[buttonCount++] = (GlassButton){{x, startY, x + btnW, startY + btnH}, ""};
        strcpy(buttons[buttonCount-1].label, row1[i]);
    }

    // Row 2: Powers & Inverse Functions
    char row2[6][8] = {"x!", "nPr", "Rec(", "Pol(", "x^3", "x^-1"};
    startY += btnH + gapY;
    for(int i=0; i<6; i++) {
        int x = 12 + i * (btnW + gapX);
        buttons[buttonCount++] = (GlassButton){{x, startY, x + btnW, startY + btnH}, ""};
        strcpy(buttons[buttonCount-1].label, row2[i]);
    }

    // Row 3: Fractions & Roots
    char row3[6][8] = {"a b/c", "sqrt", "x^2", "^", "log", "ln"};
    startY += btnH + gapY;
    for(int i=0; i<6; i++) {
        int x = 12 + i * (btnW + gapX);
        buttons[buttonCount++] = (GlassButton){{x, startY, x + btnW, startY + btnH}, ""};
        strcpy(buttons[buttonCount-1].label, row3[i]);
    }

    // Row 4: Trigonometry Base
    char row4[6][8] = {"(-)", "0,,,", "hyp", "sin", "cos", "tan"};
    startY += btnH + gapY;
    for(int i=0; i<6; i++) {
        int x = 12 + i * (btnW + gapX);
        buttons[buttonCount++] = (GlassButton){{x, startY, x + btnW, startY + btnH}, ""};
        strcpy(buttons[buttonCount-1].label, row4[i]);
    }

    // Row 5: Registers Memory
    char row5[6][8] = {"RCL", "ENG", "(", ")", "M-", "M+"};
    startY += btnH + gapY;
    for(int i=0; i<6; i++) {
        int x = 12 + i * (btnW + gapX);
        buttons[buttonCount++] = (GlassButton){{x, startY, x + btnW, startY + btnH}, ""};
        strcpy(buttons[buttonCount-1].label, row5[i]);
    }

    // --- Standard Master Numpad Blocks (Merged tightly below scientific keys) ---
    int numW = 66; // Numpad width scaled for 5 columns format structure
    int numH = 42;
    int numGapX = 8;
    int numGapY = 8;
    startY += btnH + 15; // Small visual divider separator

    char numGrid[4][5][8] = {
        {"7", "8", "9", "DEL", "AC"},
        {"4", "5", "6", "x", "/"},
        {"1", "2", "3", "+", "-"},
        {"0", "pi", ".", "Ans", "="}
    };

    for(int r=0; r<4; r++) {
        for(int c=0; c<5; c++) {
            int x = 12 + c * (numW + numGapX);
            int y = startY + r * (numH + numGapY);
            buttons[buttonCount++] = (GlassButton){{x, y, x + numW, y + numH}, ""};
            strcpy(buttons[buttonCount-1].label, numGrid[r][c]);
        }
    }
}

void ExecuteMathInput(const char* label) {
    if ((label[0] >= '0' && label[0] <= '9') && label[1] == '\0') {
        if (start_new_num || strcmp(displayBuffer, "0") == 0) {
            strcpy(displayBuffer, label);
            start_new_num = FALSE;
        } else if (strlen(displayBuffer) < 30) {
            strcat(displayBuffer, label);
        }
    } else if (strcmp(label, "AC") == 0 || strcmp(label, "C") == 0) {
        strcpy(displayBuffer, "0");
        start_new_num = TRUE;
        current_val = 0; last_op = 0;
    } else if (strcmp(label, "DEL") == 0) {
        int len = strlen(displayBuffer);
        if (len > 1) displayBuffer[len - 1] = '\0';
        else { strcpy(displayBuffer, "0"); start_new_num = TRUE; }
    } else if (strcmp(label, "+") == 0 || strcmp(label, "-") == 0 || strcmp(label, "x") == 0 || strcmp(label, "/") == 0) {
        current_val = atof(displayBuffer);
        last_op = (strcmp(label, "x") == 0) ? '*' : label[0];
        start_new_num = TRUE;
    } else if (strcmp(label, "=") == 0) {
        double second_val = atof(displayBuffer);
        double result = 0.0;
        switch (last_op) {
            case '+': result = current_val + second_val; break;
            case '-': result = current_val - second_val; break;
            case '*': result = current_val * second_val; break;
            case '/': result = (second_val != 0) ? (current_val / second_val) : 0; break;
        }
        sprintf(displayBuffer, "%g", result);
        start_new_num = TRUE; last_op = 0;
    } else if (strcmp(label, "sqrt") == 0) {
        sprintf(displayBuffer, "%g", sqrt(atof(displayBuffer))); start_new_num = TRUE;
    } else if (strcmp(label, "sin") == 0) {
        sprintf(displayBuffer, "%g", sin(atof(displayBuffer) * 3.14159265 / 180.0)); start_new_num = TRUE;
    } else if (strcmp(label, "cos") == 0) {
        sprintf(displayBuffer, "%g", cos(atof(displayBuffer) * 3.14159265 / 180.0)); start_new_num = TRUE;
    } else if (strcmp(label, "tan") == 0) {
        sprintf(displayBuffer, "%g", tan(atof(displayBuffer) * 3.14159265 / 180.0)); start_new_num = TRUE;
    } else if (strcmp(label, "pi") == 0) {
        strcpy(displayBuffer, "3.1415926535"); start_new_num = FALSE;
    }
    SetWindowText(hDisplay, displayBuffer);
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            SyncSystemTheme();
            // Stretched display box across all 6 columns cleanly
            hDisplay = CreateWindow("STATIC", "0", WS_VISIBLE | WS_CHILD | SS_RIGHT, 12, 50, 356, 48, hwnd, (HMENU)IDC_DISPLAY, NULL, NULL);
            HFONT hFont = CreateFont(34, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            SendMessage(hDisplay, WM_SETFONT, (WPARAM)hFont, TRUE);
            BuildLayoutGrid();
            break;
        }

        case WM_SETTINGCHANGE: {
            SyncSystemTheme();
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wp;
            SetTextColor(hdcStatic, text_color);
            SetBkColor(hdcStatic, glass_bg);
            return (LRESULT)hDisplayBrush;
        }

        case WM_NCHITTEST: {
            LRESULT hit = DefWindowProc(hwnd, msg, wp, lp);
            if (hit == HTCLIENT) {
                POINT pt; pt.x = LOWORD(lp); pt.y = HIWORD(lp);
                ScreenToClient(hwnd, &pt);
                if (pt.y < 45) return HTCAPTION;
            }
            return hit;
        }

        case WM_KEYDOWN: {
            int key = wp;
            if (key >= '0' && key <= '9') { char buf[2] = {key, 0}; ExecuteMathInput(buf); }
            else if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) { char buf[2] = {'0' + (key - VK_NUMPAD0), 0}; ExecuteMathInput(buf); }
            else if (key == VK_BACK) ExecuteMathInput("DEL");
            else if (key == VK_DELETE || key == VK_ESCAPE) ExecuteMathInput("AC");
            else if (key == VK_ADD) ExecuteMathInput("+");
            else if (key == VK_SUBTRACT) ExecuteMathInput("-");
            else if (key == VK_MULTIPLY) ExecuteMathInput("x");
            else if (key == VK_DIVIDE) ExecuteMathInput("/");
            else if (key == VK_RETURN) ExecuteMathInput("=");
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }

        case WM_LBUTTONDOWN: {
            POINT pt; pt.x = LOWORD(lp); pt.y = HIWORD(lp);
            if (PtInRect(&rcClose, pt)) { DestroyWindow(hwnd); PostQuitMessage(0); }
            else if (PtInRect(&rcMin, pt)) ShowWindow(hwnd, SW_MINIMIZE);
            
            for(int i=0; i<buttonCount; i++) {
                if (PtInRect(&buttons[i].rect, pt)) {
                    ExecuteMathInput(buttons[i].label);
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                }
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect; GetClientRect(hwnd, &rect);

            HBRUSH hBack = CreateSolidBrush(glass_bg);
            FillRect(hdc, &rect, hBack);
            DeleteObject(hBack);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, text_color);
            HFONT hTitleFont = CreateFont(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            SelectObject(hdc, hTitleFont);
            TextOut(hdc, 16, 16, "1OS Casio-Matrix Engine", 24);

            // Top Control Dots Alignment Vectors (RHS)
            int startX = rect.right - 68;
            rcMin   = (RECT){startX, 18, startX + 12, 30};
            rcMax   = (RECT){startX + 18, 18, startX + 30, 30};
            rcClose = (RECT){startX + 36, 18, startX + 48, 30};

            HBRUSH redBrush = CreateSolidBrush(RGB(255, 95, 87));
            HBRUSH yellowBrush = CreateSolidBrush(RGB(254, 188, 46));
            HBRUSH greenBrush = CreateSolidBrush(RGB(40, 200, 64));

            SelectObject(hdc, yellowBrush); Ellipse(hdc, rcMin.left, rcMin.top, rcMin.right, rcMin.bottom);
            SelectObject(hdc, greenBrush);  Ellipse(hdc, rcMax.left, rcMax.top, rcMax.right, rcMax.bottom);
            SelectObject(hdc, redBrush);    Ellipse(hdc, rcClose.left, rcClose.top, rcClose.right, rcClose.bottom);

            // Scientific Small Keys Labels Paint Engine
            HFONT hSciFont = CreateFont(11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            HFONT hNumFont = CreateFont(18, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

            for(int i=0; i<buttonCount; i++) {
                BOOL isNumpad = (buttons[i].rect.bottom - buttons[i].rect.top > 35);
                BOOL isAction = (strcmp(buttons[i].label, "=") == 0 || strcmp(buttons[i].label, "AC") == 0 || strcmp(buttons[i].label, "DEL") == 0);
                
                COLORREF bColor = isAction ? accent_orange : glass_btn;
                HBRUSH hb = CreateSolidBrush(bColor);
                HPEN hp = CreatePen(PS_SOLID, 1, isAction ? accent_orange : RGB(85, 85, 90));
                
                SelectObject(hdc, hb);
                SelectObject(hdc, hp);
                RoundRect(hdc, buttons[i].rect.left, buttons[i].rect.top, buttons[i].rect.right, buttons[i].rect.bottom, 5, 5);
                
                SelectObject(hdc, isNumpad ? hNumFont : hSciFont);
                SetTextColor(hdc, isAction ? RGB(255,255,255) : text_color);
                DrawText(hdc, buttons[i].label, -1, &buttons[i].rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                
                DeleteObject(hb);
                DeleteObject(hp);
            }

            DeleteObject(redBrush); DeleteObject(yellowBrush); DeleteObject(greenBrush);
            DeleteObject(hTitleFont); DeleteObject(hSciFont); DeleteObject(hNumFont);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY:
            if (hDisplayBrush) DeleteObject(hDisplayBrush);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
    WNDCLASS wc = {0};
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hInstance = hInst;
    wc.lpszClassName = "1OSCasioMatrixEngine";
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClass(&wc)) return -1;

    // Window width and height locked tightly to prevent any trailing canvas spaces
    HWND hwnd = CreateWindow("1OSCasioMatrixEngine", "1OS App Environment", 
                             WS_POPUP | WS_VISIBLE | WS_SYSMENU, 
                             CW_USEDEFAULT, CW_USEDEFAULT, 382, 510, 
                             NULL, NULL, hInst, NULL);
    
    ShowWindow(hwnd, ncmdshow);
    UpdateWindow(hwnd);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}