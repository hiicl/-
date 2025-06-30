#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <tchar.h>
#include <commdlg.h>
#include <thread>
#include <string>
#include <chrono>
#include <atomic>
#include <iostream>

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#include "easyhook.h"

using namespace D2D1;

// Direct2D上下文
struct D2DContext {
    ID2D1Factory1* pFactory = nullptr;
    ID2D1HwndRenderTarget* pRenderTarget = nullptr;
    ID2D1SolidColorBrush* pBrush = nullptr;
};
D2DContext g_d2d;

// 控件结构
struct {
    HWND hPathEdit, hDllBtn, hDllText, hInjectBtn;
    HWND hIpEdit, hConnBtn, hStatusText, hLatencyText;
    HWND hGpuText[2];
    HWND hModuleChk[3];
} g_controls;

std::wstring dllPath;
std::wstring serverIp = L"127.0.0.1";

// 选择DLL文件
void selectDllFile(HWND hwnd) {
    OPENFILENAMEW ofn = {};
    wchar_t szFile[MAX_PATH] = {};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        dllPath = szFile;
        SetWindowTextW(g_controls.hDllText, dllPath.c_str());
    }
}

// 注入DLL到新进程
void injectDll(std::wstring exePath) {
    if (dllPath.empty()) {
        SetWindowTextW(g_controls.hStatusText, L"❌ 请先选择DLL文件");
        return;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessW(exePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        SetWindowTextW(g_controls.hStatusText, L"❌ 无法启动目标程序");
        return;
    }

    Sleep(500);

    NTSTATUS res = RhInjectLibrary(
        pi.dwProcessId,
        0,
        EASYHOOK_INJECT_DEFAULT,
        (WCHAR*)dllPath.c_str(),
        NULL, NULL, 0
    );

    if (res == 0)
        SetWindowTextW(g_controls.hStatusText, L"✅ DLL 注入成功");
    else
        SetWindowTextW(g_controls.hStatusText, L"❌ 注入失败");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"目标程序路径:", WS_CHILD | WS_VISIBLE, 10, 10, 100, 20, hwnd, NULL, NULL, NULL);
        g_controls.hPathEdit = CreateWindowW(L"EDIT", L"C:\\path\\to\\target.exe", WS_CHILD | WS_VISIBLE | WS_BORDER, 120, 10, 300, 25, hwnd, NULL, NULL, NULL);

        g_controls.hDllBtn = CreateWindowW(L"BUTTON", L"选择DLL", WS_CHILD | WS_VISIBLE, 10, 50, 100, 25, hwnd, (HMENU)2, NULL, NULL);
        g_controls.hDllText = CreateWindowW(L"STATIC", L"未选择DLL", WS_CHILD | WS_VISIBLE, 120, 50, 300, 25, hwnd, NULL, NULL, NULL);

        g_controls.hInjectBtn = CreateWindowW(L"BUTTON", L"启动并注入", WS_CHILD | WS_VISIBLE, 10, 90, 150, 30, hwnd, (HMENU)1, NULL, NULL);

        CreateWindowW(L"STATIC", L"服务器IP:", WS_CHILD | WS_VISIBLE, 10, 140, 100, 20, hwnd, NULL, NULL, NULL);
        g_controls.hIpEdit = CreateWindowW(L"EDIT", serverIp.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER, 120, 140, 150, 25, hwnd, NULL, NULL, NULL);
        g_controls.hConnBtn = CreateWindowW(L"BUTTON", L"测试连接", WS_CHILD | WS_VISIBLE, 280, 140, 100, 25, hwnd, (HMENU)3, NULL, NULL);

        g_controls.hStatusText = CreateWindowW(L"STATIC", L"状态: 未连接", WS_CHILD | WS_VISIBLE, 10, 180, 370, 25, hwnd, NULL, NULL, NULL);
        g_controls.hLatencyText = CreateWindowW(L"STATIC", L"延迟: - ms", WS_CHILD | WS_VISIBLE, 10, 210, 150, 25, hwnd, NULL, NULL, NULL);

        g_controls.hGpuText[0] = CreateWindowW(L"STATIC", L"GPU0: 离线", WS_CHILD | WS_VISIBLE, 400, 10, 180, 100, hwnd, NULL, NULL, NULL);
        g_controls.hGpuText[1] = CreateWindowW(L"STATIC", L"GPU1: 离线", WS_CHILD | WS_VISIBLE, 400, 120, 180, 100, hwnd, NULL, NULL, NULL);

        g_controls.hModuleChk[0] = CreateWindowW(L"BUTTON", L"CUDA Hook", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 10, 250, 100, 25, hwnd, (HMENU)4, NULL, NULL);
        g_controls.hModuleChk[1] = CreateWindowW(L"BUTTON", L"DirectX Hook", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 120, 250, 120, 25, hwnd, (HMENU)5, NULL, NULL);
        g_controls.hModuleChk[2] = CreateWindowW(L"BUTTON", L"Vulkan Hook", WS_CHILD | WS_VISIBLE | BS_CHECKBOX, 250, 250, 120, 25, hwnd, (HMENU)6, NULL, NULL);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: {
            wchar_t pathBuffer[MAX_PATH];
            GetWindowTextW(g_controls.hPathEdit, pathBuffer, MAX_PATH);
            std::thread(injectDll, std::wstring(pathBuffer)).detach();
            break;
        }
        case 2:
            selectDllFile(hwnd);
            break;
        case 3:
            SetWindowTextW(g_controls.hStatusText, L"🟡 正在连接服务器...");
            // TODO: 连接测试
            break;
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        if (g_d2d.pRenderTarget) {
            g_d2d.pRenderTarget->BeginDraw();
            g_d2d.pRenderTarget->Clear(ColorF(ColorF::DarkSlateGray));

            D2D1_RECT_F rect1 = { 600, 10, 950, 200 };
            D2D1_RECT_F rect2 = { 600, 220, 950, 350 };

            g_d2d.pRenderTarget->DrawRectangle(rect1, g_d2d.pBrush);
            g_d2d.pRenderTarget->DrawRectangle(rect2, g_d2d.pBrush);

            HRESULT hr = g_d2d.pRenderTarget->EndDraw();
            if (hr == D2DERR_RECREATE_TARGET) {
                g_d2d.pRenderTarget->Release();
                g_d2d.pRenderTarget = nullptr;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const wchar_t CLASS_NAME[] = L"GPUOverIPLauncher";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"GPU-over-IP Launcher", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 600, NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);

    // 初始化Direct2D
    D2D1_FACTORY_OPTIONS opts = {};
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opts, (void**)&g_d2d.pFactory);

    RECT rc;
    GetClientRect(hwnd, &rc);

    g_d2d.pFactory->CreateHwndRenderTarget(
        RenderTargetProperties(),
        HwndRenderTargetProperties(hwnd, SizeU(rc.right - rc.left, rc.bottom - rc.top)),
        &g_d2d.pRenderTarget);

    g_d2d.pRenderTarget->CreateSolidColorBrush(ColorF(ColorF::White), &g_d2d.pBrush);

    std::atomic<bool> running = true;
    std::thread monitorThread([&]() {
        while (running) {
            InvalidateRect(hwnd, NULL, FALSE);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    running = false;
    monitorThread.join();

    if (g_d2d.pBrush) g_d2d.pBrush->Release();
    if (g_d2d.pRenderTarget) g_d2d.pRenderTarget->Release();
    if (g_d2d.pFactory) g_d2d.pFactory->Release();

    return 0;
}
