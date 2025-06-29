#include "hook_cuda.h"
#include "dispatcher.h"
#include "capnp_client.h"
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <iostream>
#include <mutex>
#include <vector>
#include <cstring>
#include <sstream>
#include <nlohmann/json.hpp>
#include "gpu.capnp.h"
using Direction = ::capnp::schemas::Direction_fc061cce9bf5918c;

// 定义函数指针类型
typedef CUresult (CUDAAPI *cuMemAlloc_t)(CUdeviceptr* dptr, size_t bytesize);
typedef CUresult (CUDAAPI *cuMemFree_t)(CUdeviceptr dptr);
typedef CUresult (CUDAAPI *cuMemcpyHtoD_t)(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount);
typedef CUresult (CUDAAPI *cuMemcpyDtoH_t)(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult (CUDAAPI *cuLaunchKernel_t)(
    CUfunction f,
    unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ,
    unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ,
    unsigned int sharedMemBytes, CUstream hStream,
    void** kernelParams, void** extra);

using json = nlohmann::json;

// 全局内存映射器
MemoryMapper g_memory_mapper;

// 负载均衡调度器
Dispatcher g_dispatcher;

// Cap'n Proto客户端实例
std::unique_ptr<CapnpClient> g_capnp_client;

// 原始函数指针声明
cuMemAlloc_t pOriginal_cuMemAlloc = nullptr;
cuMemFree_t pOriginal_cuMemFree = nullptr;
cuMemcpyHtoD_t pOriginal_cuMemcpyHtoD = nullptr;
cuMemcpyDtoH_t pOriginal_cuMemcpyDtoH = nullptr;
cuLaunchKernel_t pOriginal_cuLaunchKernel = nullptr;

CUresult __stdcall Hooked_cuMemAlloc(CUdeviceptr* dev_ptr, size_t byte_size) {
    // 选择目标节点
    RemoteNode* node = g_dispatcher.PickNode(byte_size);
    if (!node) {
        std::cerr << "[Hook] Failed to pick node for allocation" << std::endl;
        return CUDA_ERROR_UNKNOWN;
    }

    // 使用Cap'n Proto分配内存
    auto mem_info = g_capnp_client->cudaMemAlloc(byte_size);
    uint64_t remote_handle = mem_info.getAddr();

    // 保存映射关系
    RemoteAllocInfo info = {
        .node_id = node->id,
        .size = byte_size,
        .remote_handle = remote_handle
    };
    g_memory_mapper.AddMapping(*dev_ptr, info);
    
    // 更新节点内存状态
    node->available_memory -= byte_size;

    std::cout << "[Hook] Allocated " << byte_size << " bytes at remote node " 
              << node->id << ", remote handle: 0x" << std::hex << remote_handle << std::dec << std::endl;
    
    return CUDA_SUCCESS;
}

CUresult __stdcall Hooked_cuMemFree(CUdeviceptr dptr) {
    auto info = g_memory_mapper.GetMapping(dptr);
    if (!info) {
        std::cerr << "[Hook] Invalid pointer for free: 0x" << std::hex << dptr << std::dec << std::endl;
        return CUDA_ERROR_INVALID_VALUE;
    }

    // 获取对应节点
    RemoteNode* node = g_dispatcher.GetNodeById(info->node_id);
    if (!node) {
        std::cerr << "[Hook] Node not found for free: " << info->node_id << std::endl;
        return CUDA_ERROR_UNKNOWN;
    }

    // 使用Cap'n Proto释放内存
    g_capnp_client->cudaMemFree(info->remote_handle);

    // 移除映射
    g_memory_mapper.RemoveMapping(dptr);
    
    // 更新节点内存状态
    node->available_memory += info->size;
    
    std::cout << "[Hook] Freed memory at remote node " << node->id 
              << ", remote ptr: 0x" << std::hex << info->remote_handle << std::dec << std::endl;
    
    return CUDA_SUCCESS;
}

// 内存传输实现
CUresult __stdcall Hooked_cuMemcpyHtoD(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount) {
    auto info = g_memory_mapper.GetMapping(dstDevice);
    if (!info) {
        return pOriginal_cuMemcpyHtoD(dstDevice, srcHost, ByteCount);
    }
    
    // 获取对应节点
    RemoteNode* node = g_dispatcher.GetNodeById(info->node_id);
    if (!node) {
        return CUDA_ERROR_UNKNOWN;
    }
    
    // 使用Cap'n Proto传输数据
    g_capnp_client->cudaMemcpy(
        info->remote_handle, 
        reinterpret_cast<uint64_t>(srcHost), 
        ByteCount, 
        Direction::DEVICE_TO_HOST
    );
    
    return CUDA_SUCCESS;
}

CUresult __stdcall Hooked_cuMemcpyDtoH(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount) {
    auto info = g_memory_mapper.GetMapping(srcDevice);
    if (!info) {
        return pOriginal_cuMemcpyDtoH(dstHost, srcDevice, ByteCount);
    }
    
    RemoteNode* node = g_dispatcher.GetNodeById(info->node_id);
    if (!node) {
        return CUDA_ERROR_UNKNOWN;
    }
    
    g_capnp_client->cudaMemcpy(
        reinterpret_cast<uint64_t>(dstHost),
        info->remote_handle,
        ByteCount,
        Direction::DEVICE_TO_HOST
    );
    
    return CUDA_SUCCESS;
}

// 内核启动增强实现
CUresult __stdcall Hooked_cuLaunchKernel(
    CUfunction f,
    unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ,
    unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ,
    unsigned int sharedMemBytes, CUstream hStream,
    void** kernelParams, void** extra) 
{
    RemoteNode* node = g_dispatcher.GetDefaultNode();
    if (!node) return CUDA_ERROR_UNKNOWN;

    // 序列化内核参数为JSON
    json kernel_config;
    kernel_config["gridDim"] = {gridDimX, gridDimY, gridDimZ};
    kernel_config["blockDim"] = {blockDimX, blockDimY, blockDimZ};
    kernel_config["sharedMemBytes"] = sharedMemBytes;
    kernel_config["stream"] = reinterpret_cast<uint64_t>(hStream);
    kernel_config["params"] = json::array();

    if (kernelParams) {
        for (int i = 0; kernelParams[i] != nullptr; i++) {
            kernel_config["params"].push_back(reinterpret_cast<uint64_t>(kernelParams[i]));
        }
    }

    std::string config_str = kernel_config.dump();
    
    auto response = g_capnp_client->cudaKernelLaunch(
        "default-gpu",
        config_str
    );
    
    if (response.getExitCode() != 0) {
        return CUDA_ERROR_LAUNCH_FAILED;
    }
    return CUDA_SUCCESS;
}

// 状态监控线程
void StatusMonitorThread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 收集所有节点状态
        for (auto& node : g_dispatcher.GetNodes()) {
            auto status = g_capnp_client->getGpuStatus(node.id);
            node.available_memory = status.getAvailableMemory();
            node.gpu_utilization = status.getUtilization();
        }
    }
}

// 初始化函数 - 在DLL加载时调用
void InitializeHook() {
    // 创建Cap'n Proto客户端
    g_capnp_client = std::make_unique<CapnpClient>("localhost:50051");
    
    // 初始化CUDA环境
    g_capnp_client->cudaInit();
    
    // 初始化负载均衡调度器
    auto gpuListReader = g_capnp_client->listGpus();      // GpuList::Reader
    auto gpus = gpuListReader.getGpus();                 // ::capnp::List<GpuInfo>::Reader

    for (auto gpu : gpus) {
        std::string uuid = gpu.getUuid().cStr();
        std::string name = gpu.getName().cStr();
        int64_t totalMemory = gpu.getTotalMemory();

    g_dispatcher.AddNode(RemoteNode(
        uuid, 
        name, 
        "",   // address
        "",   // roce_interface
        50,   // priority
        static_cast<size_t>(totalMemory), 
        static_cast<size_t>(totalMemory),
        0.0,  // network_latency
        0.0,  // cpu_usage
        0.0   // gpu_utilization
    ));
    }
    
    // 启动状态监控线程
    std::thread(StatusMonitorThread).detach();
}

// 清理函数 - 在DLL卸载时调用
void CleanupHook() {
    g_capnp_client.reset();
}
