#pragma once

#include "pch.h"
#include "interception.h"
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;


int get_screen_width(void) {
    return GetSystemMetrics(SM_CXSCREEN);
}

int get_screen_height(void) {
    return GetSystemMetrics(SM_CYSCREEN);
}

struct point {
    double x;
    double y;
    point(double x, double y) : x(x), y(y) {}
};

inline bool is_color(int red, int green, int blue) {
        if (green >= 140) {
            return abs(red - blue) <= 8 &&
                red - green >= 50 &&
                blue - green >= 50 &&
                red >= 105 &&
                blue >= 105;
        }

        return abs(red - blue) <= 13 &&
            red - green >= 60 &&
            blue - green >= 60 &&
            red >= 110 &&
            blue >= 100;
}

inline bool is_color2(int red, int green, int blue) {
    if (green >= 140) {
        return abs(red - blue) <= 8 &&
            red - green >= 50 &&
            blue - green >= 50 &&
            red >= 105 &&
            blue >= 105;
    }

    return abs(red - blue) <= 13 &&
        red - green >= 60 &&
        blue - green >= 60 &&
        red >= 110 &&
        blue >= 100;
}

InterceptionContext context;
InterceptionDevice device;
InterceptionStroke stroke;
BYTE* screenData = 0;
bool run_threads = true;
const int screen_width = get_screen_width(), screen_height = get_screen_height();

int aim_x = 0;
int aim_y = 0;
int w = 0;
int h = 0;

void bot() {
    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_end = std::chrono::high_resolution_clock::now();

    HDC hScreen = GetDC(NULL);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    screenData = (BYTE*)malloc(5 * screen_width * screen_height);
    HDC hDC = CreateCompatibleDC(hScreen);
    point middle_screen(screen_width >> 1, screen_height >> 1);

    BITMAPINFOHEADER bmi = { 0 };
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biWidth = w;
    bmi.biHeight = -h;
    bmi.biCompression = BI_RGB;
    bmi.biSizeImage = 0;

    while (run_threads) {
        Sleep(6);
        HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
        BOOL bRet = BitBlt(hDC, 0, 0, w, h, hScreen, middle_screen.x - (w >> 1), middle_screen.y - (h >> 1), SRCCOPY);
        SelectObject(hDC, old_obj);
        GetDIBits(hDC, hBitmap, 0, h, screenData, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
        bool stop_loop = false;
        for (int j = 0; j < h; ++j) {
            for (int i = 0; i < w * 4; i += 4) {
                #define red screenData[i + (j*w*4) + 2]
                #define green screenData[i + (j*w*4) + 1]
                #define blue screenData[i + (j*w*4) + 0]

                if (green < 190 && is_color(red, green, blue)) {
                    aim_x = (i >> 2) - (w >> 1);
                    aim_y = j - (h >> 1) + 3;
                    stop_loop = true;
                    break;
                }
            }
            if (stop_loop) {
                break;
            }
        }
        if (!stop_loop) {
            aim_x = 0;
            aim_y = 0;
        }
    }
}

int main(void) {
    int zone = 50;
    double sensitivity = 0.52;
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    auto w_f = freopen("CON", "w", stdout);
    auto r_f = freopen("CON", "r", stdin);
    cin >> zone;
    cin >> sensitivity;
    fclose(w_f);
    fclose(r_f);
    FreeConsole();
    thread(bot).detach();
    w = zone;
    h = zone;
    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto left_start = std::chrono::high_resolution_clock::now();
    auto left_end = std::chrono::high_resolution_clock::now();
    double sensitivity_x = 1.0 / sensitivity / (screen_width / 1920.0) * 1.08;
    double sensitivity_y = 1.0 / sensitivity / (screen_height / 1080.0) * 1.08;
    bool left_down = false;
    
    context = interception_create_context();
    interception_set_filter(context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_ALL);

    while (interception_receive(context, device = interception_wait(context), &stroke, 1) > 0) {
        InterceptionMouseStroke& mstroke = *(InterceptionMouseStroke*)&stroke;
        t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        CURSORINFO cursorInfo = { 0 };
        cursorInfo.cbSize = sizeof(cursorInfo);
        GetCursorInfo(&cursorInfo);
        if (cursorInfo.flags != 1) {
            if ((mstroke.state & INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN) || (mstroke.state & INTERCEPTION_MOUSE_BUTTON_5_DOWN)) {
                if (elapsed_time_ms > 7) {
                    t_start = std::chrono::high_resolution_clock::now();
                    left_start = std::chrono::high_resolution_clock::now();
                    if (aim_x >= -w && aim_x <= w && aim_y >= -h && aim_y <= h) {
                        mstroke.x += double(aim_x) * sensitivity_x;
                        mstroke.y += double(aim_y + 5) * sensitivity_y;
                    }
                }
            }
            if (GetAsyncKeyState(VK_ADD))
            {
                if (sensitivity <= 1.0)
                {
                    sensitivity += 0.01;
                    double sensitivity_x = 1.0 / sensitivity / (screen_width / 1920.0) * 1.08;
                    double sensitivity_y = 1.0 / sensitivity / (screen_height / 1080.0) * 1.08;
                }
                Sleep(40);
            }

            if (GetAsyncKeyState(VK_SUBTRACT))
            {
                if (sensitivity > 0.35)
                {
                    sensitivity -= 0.01;
                    double sensitivity_x = 1.0 / sensitivity / (screen_width / 1920.0) * 1.08;
                    double sensitivity_y = 1.0 / sensitivity / (screen_height / 1080.0) * 1.08;
                }
                Sleep(40);
            }
        }

        interception_send(context, device, &stroke, 1);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main, 0, 0, 0);
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            interception_destroy_context(context);
            if (screenData) {
                free(screenData);
            }
            break;
    }

    return TRUE;
}