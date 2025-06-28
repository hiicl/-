#include <Windows.h>
#include <easyhook.h>
#include "hook_cuda.h"

// 原始函数指针
typedef CUresult (__stdcall *cuMemAlloc_t)(CUdeviceptr*, size_t);
typedef CUresult (__stdcall *cuMemFree_t)(CUdeviceptr);
typedef CUresult (__stdcall *cuMemcpyHtoD_t)(CUdeviceptr, const void*, size_t);
typedef CUresult (__stdcall *cuMemcpyDtoH_t)(void*, CUdeviceptr, size_t);
typedef CUresult (__stdcall *cuLaunchKernel_t)(CUfunction, 
    unsigned int, unsigned int, unsigned int,
    unsigned int, unsigned int, unsigned int,
    unsigned int, CUstream, void**, void**);

// 全局变量存储原始函数
cuMemAlloc_t pOriginal_cuMemAlloc = nullptr;
cuMemFree_t pOriginal_cuMemFree = nullptr;
cuMemcpyHtoD_t pOriginal_cuMemcpyHtoD = nullptr;
cuMemcpyDtoH_t pOriginal_cuMemcpyDtoH = nullptr;
cuLaunchKernel_t pOriginal_cuLaunchKernel = nullptr;

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
