@0x8517e4c5d0d0b3f1;

using UUID = Text;
using Handle = UInt64;
using Control = import "control.capnp";

# ========= 基础结构 =========

struct Void {}

struct GpuList {
    gpus @0 :List(Control.GpuInfo);  # 从控制面引入
}

struct GpuRequest {
    uuid @0 :UUID;
}

struct GpuStatus {
    usedMemory @0 :Int64;
    utilization @1 :Int32;
}

enum ErrorCode {
    ok @0;
    outOfMemory @1;
    gpuNotFound @2;
    streamError @3;
    kernelLaunchFail @4;
    unknown @5;
}

struct Ack {
    ok @0 :Bool;
    msg @1 :Text;
    code @2 :ErrorCode;
}

# ========= 计算命令 =========

struct RunRequest {
    uuid @0 :UUID;
    cmd @1 :Text;
    streamHandle @2 :Handle;
}

struct RunResponse {
    exitCode @0 :Int32;
    output @1 :Text;
}

# ========= 内存传输 =========

struct MemcpyParams {
    src @0 :Handle;
    dst @1 :Handle;
    size @2 :UInt64;
    direction @3 :Direction;
}

enum Direction {
    hostToDevice @0;
    deviceToHost @1;
    deviceToDevice @2;
}

struct CudaMemInfo {
    addr @0 :Handle;
    size @1 :UInt64;
}

# ========= CUDA 流与事件 =========

struct StreamHandle {
    handle @0 :Handle;
}

struct EventHandle {
    handle @0 :Handle;
}

struct StreamCreateParams {
    flags @0 :UInt32;
}

struct EventParams {
    flags @0 :UInt32;
}

# ========= 批处理与多GPU =========

struct BatchRunRequest {
    requests @0 :List(RunRequest);
    stream @1 :Handle;
}

struct BatchRunResponse {
    responses @0 :List(RunResponse);
}

struct MultiGpuRequest {
    uuids @0 :List(UUID);
    command @1 :Text;
    stream @2 :Handle;
}

# ========= 主服务接口 =========

interface GpuService {
    # 基础服务
    listGpus @0 () -> (gpus :GpuList);
    getGpuStatus @1 (request :GpuRequest) -> (status :GpuStatus);
    acquireGpu @2 (request :GpuRequest) -> (ack :Ack);
    releaseGpu @3 (request :GpuRequest) -> (ack :Ack);
    runCommand @4 (request :RunRequest) -> (response :RunResponse);

    # 初始化与内存
    cudaInit @5 () -> (ack :Ack);
    cudaMemAlloc @6 (info :CudaMemInfo) -> (result :CudaMemInfo);
    cudaMemcpy @7 (params :MemcpyParams) -> (ack :Ack);
    cudaMemFree @8 (info :CudaMemInfo) -> (ack :Ack);

    # CUDA 流
    createCudaStream @9 (params :StreamCreateParams) -> (handle :StreamHandle);
    destroyCudaStream @10 (handle :StreamHandle) -> (ack :Ack);
    synchronizeCudaStream @11 (handle :StreamHandle) -> (ack :Ack);

    # 内核执行
    cudaKernelLaunch @12 (request :RunRequest) -> (response :RunResponse);

    # 事件控制
    createEvent @13 (params :EventParams) -> (handle :EventHandle);
    recordEvent @14 (handle :EventHandle) -> (ack :Ack);
    eventSynchronize @15 (handle :EventHandle) -> (ack :Ack);
    destroyEvent @16 (handle :EventHandle) -> (ack :Ack);

    # 批量/多GPU
    batchKernelLaunch @17 (request :BatchRunRequest) -> (response :BatchRunResponse);
    multiGpuCooperation @18 (request :MultiGpuRequest) -> (ack :Ack);
}
