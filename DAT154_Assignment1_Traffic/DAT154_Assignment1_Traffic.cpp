// DAT154_Assignment1_Traffic.cpp :
#include "framework.h"
#include "DAT154_Assignment1_Traffic.h"
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>



#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ProbDlg(HWND, UINT, WPARAM, LPARAM);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);



    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DAT154ASSIGNMENT1TRAFFIC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DAT154ASSIGNMENT1TRAFFIC));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DAT154ASSIGNMENT1TRAFFIC));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DAT154ASSIGNMENT1TRAFFIC);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}



BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}


enum Phase { NS_GREEN, NS_YELLOW, NS_REDYELLOW, EW_GREEN, EW_YELLOW, EW_REDYELLOW };
Phase phase = NS_GREEN;


struct Car
{
    float x, y;
    float speed;
};

std::vector<Car> carsEW; // cars from west -> east
std::vector<Car> carsNS; // cars from north -> south

const int CAR_EW_W = 50, CAR_EW_H = 25;
const int CAR_NS_W = 25, CAR_NS_H = 50;

float stopX_EW = 0.0f;
float stopY_NS = 0.0f;

float pw = 0.30f; // probability per second from the west
float pn = 0.20f; // probability per second from north


void SpawnEW(HWND hWnd)
{
    RECT r; GetClientRect(hWnd, &r);
    int roadW = 140;
    int cy = (r.bottom - r.top) / 2;

    //don't spawn if previous car is still too close to start
    if (!carsEW.empty() && carsEW.back().x < 20) return;

    Car c;
    c.x = -40;
    c.y = (float)(cy - roadW / 4 - CAR_EW_H / 2);
    c.speed = 4.0f;
    carsEW.push_back(c);
}

void SpawnNS(HWND hWnd)
{
    RECT r; GetClientRect(hWnd, &r);
    int roadW = 140;
    int cx = (r.right - r.left) / 2;

    if (!carsNS.empty() && carsNS.back().y < 20) return;

    Car c;
    c.x = (float)(cx - roadW / 4 - CAR_NS_W / 2);
    c.y = -40;
    c.speed = 4.0f;
    carsNS.push_back(c);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        // We repaint the whole client area ourselves (see WM_PAINT) and use
        // double-buffering, so skipping default background erase reduces flicker.
        return 1;

    case WM_TIMER:
    {
        if (wParam == 1)
        {
            switch (phase)
            {
            case NS_GREEN:      phase = NS_YELLOW;     break;
            case NS_YELLOW:     phase = EW_REDYELLOW;  break;
            case EW_REDYELLOW:  phase = EW_GREEN;      break;
            case EW_GREEN:      phase = EW_YELLOW;     break;
            case EW_YELLOW:     phase = NS_REDYELLOW;  break;
            case NS_REDYELLOW:  phase = NS_GREEN;      break;
            }
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        if (wParam == 2)
        {
            int roadW = 140;
            float interLeft = (float)(stopX_EW + 10);
            float interTop = (float)(stopY_NS + 10);
            float interRight = interLeft + roadW;
            float interBottom = interTop + roadW;

            auto ewInIntersection = [&](const Car& c)
                {
                    float frontX = c.x + CAR_EW_W;
                    return (frontX > interLeft && c.x < interRight);
                };

            auto nsInIntersection = [&](const Car& c)
                {
                    float frontY = c.y + CAR_NS_H;
                    return (frontY > interTop && c.y < interBottom);
                };

            bool anyEWInside = std::any_of(carsEW.begin(), carsEW.end(), ewInIntersection);
            bool anyNSInside = std::any_of(carsNS.begin(), carsNS.end(), nsInIntersection);

            bool ewCanGo = (phase == EW_GREEN) || (phase == EW_YELLOW);
            bool nsCanGo = (phase == NS_GREEN) || (phase == NS_YELLOW);

            // Cars that have already entered the intersection should keep moving,
            // even if the light changes while they're inside.
            for (size_t i = 0; i < carsEW.size(); i++)
            {
                auto& c = carsEW[i];

                float frontX = c.x + CAR_EW_W;
                bool inIntersection = ewInIntersection(c);
                bool atStopLine = (frontX >= stopX_EW);
                bool beforeIntersectionEntry = (frontX < interLeft);

                // 1) stop at red ONLY if we are between the stop line and the intersection entry.
                // Once the car's FRONT has crossed into the intersection (frontX >= interLeft), it keeps going.
                if (!ewCanGo && atStopLine && beforeIntersectionEntry)
                    continue;

                // 2) do not enter intersection if NS is inside (crash prevention)
                float nextFrontX = frontX + c.speed;
                if (beforeIntersectionEntry && anyNSInside && nextFrontX >= interLeft)
                    continue;

                // 3) queue: stop behind the car in front
                if (i > 0)
                {
                    const auto& prev = carsEW[i - 1];
                    float gap = prev.x - (c.x + CAR_EW_W); // distance to the car in front
                    if (gap < 15.0f)
                        continue;
                }

                c.x += c.speed;
            }

            for (size_t i = 0; i < carsNS.size(); i++)
            {
                auto& c = carsNS[i];

                float frontY = c.y + CAR_NS_H;
                bool inIntersection = nsInIntersection(c);
                bool atStopLine = (frontY >= stopY_NS);
                bool beforeIntersectionEntry = (frontY < interTop);

                // 1) stop at red ONLY if we are between the stop line and the intersection entry.
                // Once the car's FRONT has crossed into the intersection (frontY >= interTop), it keeps going.
                if (!nsCanGo && atStopLine && beforeIntersectionEntry)
                    continue;

                // 2) do not enter intersection if EW is inside (crash prevention)
                float nextFrontY = frontY + c.speed;
                if (beforeIntersectionEntry && anyEWInside && nextFrontY >= interTop)
                    continue;

                // 3) queue: stop behind the car in front
                if (i > 0)
                {
                    const auto& prev = carsNS[i - 1];
                    float gap = prev.y - (c.y + CAR_NS_H);
                    if (gap < 15.0f)
                        continue;
                }

                c.y += c.speed;
            }

            // Remove cars that are out of the window
            RECT r;
            GetClientRect(hWnd, &r);

            carsEW.erase(
                std::remove_if(carsEW.begin(), carsEW.end(),
                    [&](const Car& c) { return c.x > r.right + 60; }),
                carsEW.end());

            carsNS.erase(
                std::remove_if(carsNS.begin(), carsNS.end(),
                    [&](const Car& c) { return c.y > r.bottom + 60; }),
                carsNS.end());

            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        if (wParam == 3)
        {
            float u1 = (float)rand() / (float)RAND_MAX;
            float u2 = (float)rand() / (float)RAND_MAX;

            if (u1 < pw) SpawnEW(hWnd);
            if (u2 < pn) SpawnNS(hWnd);

            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        return 0;
    }

    case WM_KEYDOWN:
    {
        const float step = 0.10f;

        switch (wParam)
        {
        case 'D':
            DialogBox(hInst, MAKEINTRESOURCE(IDD_PROB), hWnd, ProbDlg);
            return 0;

        case VK_RIGHT:  // increase pw (cars from left)
            pw += step;
            break;

        case VK_LEFT:   // decrease pw
            pw -= step;
            break;

        case VK_UP:     // increase pn (cars going down)
            pn += step;
            break;

        case VK_DOWN:   // decrease pn
            pn -= step;
            break;
        }

        // Clamp between 0.0 and 1.0
        if (pw < 0.0f) pw = 0.0f;
        if (pw > 1.0f) pw = 1.0f;
        if (pn < 0.0f) pn = 0.0f;
        if (pn > 1.0f) pn = 1.0f;

        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }
    break;



    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC paintDC = BeginPaint(hWnd, &ps);

        RECT r;
        GetClientRect(hWnd, &r);
        int w = r.right - r.left;
        int h = r.bottom - r.top;

        // Double-buffer to reduce flicker
        HDC memDC = CreateCompatibleDC(paintDC);
        HBITMAP memBmp = CreateCompatibleBitmap(paintDC, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
        HDC hdc = memDC;

        // Background
        HBRUSH bg = CreateSolidBrush(RGB(220, 220, 220));
        FillRect(hdc, &r, bg);
        DeleteObject(bg);

        int roadW = 140;
        int cx = (r.right - r.left) / 2;
        int cy = (r.bottom - r.top) / 2;

        // Stoppinglines
        stopX_EW = (float)(cx - roadW / 2 - 10); // cars from the west stop before the vertical road
        stopY_NS = (float)(cy - roadW / 2 - 10); // cars from the north stop before horizontal road

        // Roads
        RECT hRoad{ r.left, cy - roadW / 2, r.right, cy + roadW / 2 };
        RECT vRoad{ cx - roadW / 2, r.top, cx + roadW / 2, r.bottom };

        HBRUSH road = CreateSolidBrush(RGB(50, 50, 50));
        FillRect(hdc, &hRoad, road);
        FillRect(hdc, &vRoad, road);
        DeleteObject(road);

        // Stoppinglines white stripes
        HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);

        // EW Stoppinglines (vertical line)
        MoveToEx(hdc, (int)stopX_EW, cy - roadW / 2, nullptr);
        LineTo(hdc, (int)stopX_EW, cy + roadW / 2);

        // NS Stoppinglines (horizontal line)
        MoveToEx(hdc, cx - roadW / 2, (int)stopY_NS, nullptr);
        LineTo(hdc, cx + roadW / 2, (int)stopY_NS);

        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        // drawLamp 
        auto drawLamp = [&](int x1, int y1, int x2, int y2, COLORREF onColor, bool isOn)
            {
                HBRUSH b = CreateSolidBrush(isOn ? onColor : RGB(60, 60, 60));
                HBRUSH old = (HBRUSH)SelectObject(hdc, b);
                Ellipse(hdc, x1, y1, x2, y2);
                SelectObject(hdc, old);
                DeleteObject(b);
            };


        bool nsGreen = (phase == NS_GREEN);
        bool nsYellow = (phase == NS_YELLOW);
        bool nsRed = (phase == EW_GREEN) || (phase == EW_YELLOW) || (phase == EW_REDYELLOW) || (phase == NS_REDYELLOW);
        bool nsRedYellow = (phase == NS_REDYELLOW);

        bool ewGreen = (phase == EW_GREEN);
        bool ewYellow = (phase == EW_YELLOW);
        bool ewRed = (phase == NS_GREEN) || (phase == NS_YELLOW) || (phase == NS_REDYELLOW) || (phase == EW_REDYELLOW);
        bool ewRedYellow = (phase == EW_REDYELLOW);

        // NS traffic light  
        Rectangle(hdc, cx + 80, cy - 240, cx + 140, cy - 100);
        drawLamp(cx + 95, cy - 225, cx + 125, cy - 195, RGB(255, 0, 0), nsRed || nsRedYellow);
        drawLamp(cx + 95, cy - 185, cx + 125, cy - 155, RGB(255, 255, 0), nsYellow || nsRedYellow);
        drawLamp(cx + 95, cy - 145, cx + 125, cy - 115, RGB(0, 200, 0), nsGreen);

        // EW traffic light
        Rectangle(hdc, cx - 240, cy + 80, cx - 100, cy + 140);
        drawLamp(cx - 225, cy + 95, cx - 195, cy + 125, RGB(255, 0, 0), ewRed || ewRedYellow);
        drawLamp(cx - 185, cy + 95, cx - 155, cy + 125, RGB(255, 255, 0), ewYellow || ewRedYellow);
        drawLamp(cx - 145, cy + 95, cx - 115, cy + 125, RGB(0, 200, 0), ewGreen);

        // Draw pw/pn next to the traffic lights as percentages.
        // Note: pw/pn are still stored and edited as decimals (0.10, 0.25, etc.).
        {
            int pwPercent = (int)(pw * 100.0f + 0.5f);
            int pnPercent = (int)(pn * 100.0f + 0.5f);

            HFONT font = CreateFont(
                22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                VARIABLE_PITCH, TEXT("Arial"));

            HFONT oldFont = (HFONT)SelectObject(hdc, font);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));

            wchar_t buf[16];

            // pn: controls cars going down (NS stream). Place near the vertical light.
            swprintf_s(buf, L"%d%%", pnPercent);
            TextOut(hdc, cx + 90, cy - 260, buf, (int)wcslen(buf));

            // pw: controls cars from the left (EW stream). Place near the horizontal light.
            swprintf_s(buf, L"%d%%", pwPercent);
            TextOut(hdc, cx - 285, cy + 100, buf, (int)wcslen(buf));

            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }

        // Cars
        HBRUSH carBrush = CreateSolidBrush(RGB(0, 120, 255));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, carBrush);

        for (const auto& c : carsEW)
            Rectangle(hdc, (int)c.x, (int)c.y, (int)c.x + CAR_EW_W, (int)c.y + CAR_EW_H);

        for (const auto& c : carsNS)
            Rectangle(hdc, (int)c.x, (int)c.y, (int)c.x + CAR_NS_W, (int)c.y + CAR_NS_H);

        SelectObject(hdc, oldBrush);
        DeleteObject(carBrush);

        // Present the back buffer
        BitBlt(paintDC, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hWnd, &ps);
    }
    return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN:
    {
        RECT r; GetClientRect(hWnd, &r);
        int cx = (r.right - r.left) / 2;
        int cy = (r.bottom - r.top) / 2;

        Car c;
        c.x = 20;
        c.y = cy - 20;
        c.speed = 4.0f;
        carsEW.push_back(c);

        InvalidateRect(hWnd, nullptr, FALSE);
    }
    return 0;

    case WM_RBUTTONDOWN:
    {
        RECT r; GetClientRect(hWnd, &r);
        int cx = (r.right - r.left) / 2;
        int cy = (r.bottom - r.top) / 2;

        Car c;
        c.x = cx - 20;
        c.y = 20;
        c.speed = 4.0f;
        carsNS.push_back(c);

        InvalidateRect(hWnd, nullptr, FALSE);
    }
    return 0;


    case WM_CREATE:
    {
        srand((unsigned)time(nullptr));
        SetTimer(hWnd, 1, 2000, nullptr); // traffic light
        SetTimer(hWnd, 2, 30, nullptr); // animation
        SetTimer(hWnd, 3, 1000, nullptr); // 1 time each second, try to spawn new cars based on probabilities
        SetFocus(hWnd);
    }
    return 0;



    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {

    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ProbDlg(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        wchar_t buf[32];
        swprintf_s(buf, L"%.2f", pw);
        SetDlgItemText(hDlg, IDC_PW, buf);

        swprintf_s(buf, L"%.2f", pn);
        SetDlgItemText(hDlg, IDC_PN, buf);
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            wchar_t b1[32], b2[32];
            GetDlgItemText(hDlg, IDC_PW, b1, 32);
            GetDlgItemText(hDlg, IDC_PN, b2, 32);

            pw = (float)_wtof(b1);
            pn = (float)_wtof(b2);

            // clamp 0-1
            if (pw < 0) pw = 0; if (pw > 1) pw = 1;
            if (pn < 0) pn = 0; if (pn > 1) pn = 1;

            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

