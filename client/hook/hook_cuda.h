#pragma once
#include <optional>

#include <cuda.h>
#include <shared_mutex>
#include <unordered_map>
#include <memory>
#include <optional>
#include <iostream>

#include "context_manager.h"

// Hook函数声明
CUresult __stdcall Hooked_cuMemAlloc(CUdeviceptr* dev_ptr, size_t byte_size);
CUresult __stdcall Hooked_cuMemFree(CUdeviceptr dptr);
CUresult __stdcall Hooked_cuMemcpyHtoD(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount);
CUresult __stdcall Hooked_cuMemcpyDtoH(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount);
CUresult __stdcall Hooked_cuLaunchKernel(CUfunction f,
    unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ,
    unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ,
    unsigned int sharedMemBytes, CUstream hStream, void** kernelParams, void** extra);
