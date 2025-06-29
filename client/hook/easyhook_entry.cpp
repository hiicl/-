#include <Windows.h>
#include <easyhook.h>
#include "hook_cuda.h"

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}

// 安装Hook
extern "C" __declspec(dllexport) void InstallHook() {
    HMODULE hCuda = GetModuleHandleW(L"nvcuda.dll");
    if (!hCuda) {
        MessageBoxA(NULL, "Failed to find nvcuda.dll", "Error", MB_ICONERROR);
        return;
    }

    // 函数指针列表和对应的hook函数
    struct HookInfo {
        void** original;
        void* hook_function;
        const char* function_name;
    } hooks[] = {
        { (void**)&pOriginal_cuMemAlloc, Hooked_cuMemAlloc, "cuMemAlloc_v2" },
        { (void**)&pOriginal_cuMemFree, Hooked_cuMemFree, "cuMemFree_v2" },
        { (void**)&pOriginal_cuMemcpyHtoD, Hooked_cuMemcpyHtoD, "cuMemcpyHtoD_v2" },
        { (void**)&pOriginal_cuMemcpyDtoH, Hooked_cuMemcpyDtoH, "cuMemcpyDtoH_v2" },
        { (void**)&pOriginal_cuLaunchKernel, Hooked_cuLaunchKernel, "cuLaunchKernel" }
    };

    // 统一安装所有hook
    for (auto& hook : hooks) {
        *hook.original = GetProcAddress(hCuda, hook.function_name);
        if (!*hook.original) {
            char error_msg[256];
            sprintf_s(error_msg, "Failed to find %s", hook.function_name);
            MessageBoxA(NULL, error_msg, "Error", MB_ICONERROR);
            continue;
        }

        HOOK_TRACE_INFO hHook = { NULL };
        NTSTATUS result = LhInstallHook(
            *hook.original,
            hook.hook_function,
            NULL,
            &hHook
        );

        if (FAILED(result)) {
            char error_msg[256];
            sprintf_s(error_msg, "Failed to install hook for %s: 0x%X", 
                     hook.function_name, result);
            MessageBoxA(NULL, error_msg, "Error", MB_ICONERROR);
        }
    }
}

// 卸载Hook
extern "C" __declspec(dllexport) void UninstallHook() {
    LhUninstallAllHooks();
    LhWaitForPendingRemovals();
}
