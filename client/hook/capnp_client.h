#pragma once

// 系统头文件
#include <string>
#include <cstdint>
#include <capnp/message.h>
#include <capnp/rpc.h>
#include <capnp/rpc-twoparty.h>
#include <kj/async.h>
#include <kj/async-io.h>
#include <kj/memory.h>
#include <kj/debug.h>
#include <memory>
#include <iostream>
#include <vector>
#include <mutex>

// 项目头文件
#include "proto/gpu.capnp.h"
#include "proto/control.capnp.h"

class CapnpClient {
public:
    explicit CapnpClient(const std::string& server_address);
    CapnpClient(const CapnpClient&) = delete;
    CapnpClient& operator=(const CapnpClient&) = delete;
    ~CapnpClient();

    // 控制面操作
    Path::Reader requestPath(const std::string& src, const std::string& dst);
    void reportMetrics(float throughput, float latency, float errorRate);
    bool registerGpu(const std::string& uuid, const std::string& name, std::int64_t totalMemory);

    // GPU基础操作
    GpuList::Reader listGpus();
    GpuStatus::Reader getGpuStatus(const std::string& uuid);
    Ack::Reader acquireGpu(const std::string& uuid);
    Ack::Reader releaseGpu(const std::string& uuid);
    RunResponse::Reader runCommand(const std::string& uuid, const std::string& cmd);

    // CUDA操作
    Ack::Reader cudaInit();
    CudaMemInfo::Reader cudaMemAlloc(std::uint64_t size);
    Ack::Reader cudaMemcpy(std::uint64_t dst, std::uint64_t src, std::uint64_t count, Direction direction);
    Ack::Reader cudaMemFree(std::uint64_t addr);
    StreamHandle::Reader createCudaStream(std::uint32_t flags);
    Ack::Reader destroyCudaStream(std::uint64_t handle);
    Ack::Reader synchronizeCudaStream(std::uint64_t handle);
    RunResponse::Reader cudaKernelLaunch(const std::string& uuid, const std::string& cmd);

private:
    std::unique_ptr<kj::AsyncIoContext> ioContext_;
    kj::WaitScope* waitScope_;

    kj::Own<capnp::TwoPartyClient> client_;  // ✅ 用 kj::Own 替代 std::unique_ptr
    std::unique_ptr<GpuService::Client> gpuService_;
    std::unique_ptr<Scheduler::Client> schedulerService_;
};