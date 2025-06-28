#include "capnp_client.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>
#include <kj/async-io.h>
#ifdef _WIN32
#include <Windows.h> 
#else
#include <kj/async-unix.h>
#endif
#include <kj/debug.h>
#include <iostream>
#include <stdexcept>


using namespace std;

CapnpClient::CapnpClient(const std::string& server_address) {
    try {
        ioContext_ = std::make_unique<kj::AsyncIoContext>(kj::setupAsyncIo());
        waitScope_ = &ioContext_->waitScope;

        // 解析 host:port
        auto pos = server_address.find(":");
        if (pos == std::string::npos) throw std::runtime_error("server_address must be host:port");
        std::string host = server_address.substr(0, pos);
        uint16_t port = static_cast<uint16_t>(std::stoi(server_address.substr(pos + 1)));

        auto address = ioContext_->provider->getNetwork()->parseAddress(host, port).wait(*waitScope_);
        auto connection = address->connect().wait(*waitScope_);

        client_ = std::make_unique<capnp::TwoPartyClient>(std::move(connection));
        auto bootstrap = client_->bootstrap();
        gpuService_ = bootstrap.castAs<GpuService>();
        schedulerService_ = bootstrap.castAs<Scheduler>();
    } catch (const kj::Exception& e) {
        std::cerr << "Failed to initialize CapnpClient: " << e.getDescription().cStr() << std::endl;
        throw;
    }
}

CapnpClient::~CapnpClient() {
    // unique_ptr 自动析构
}

Path::Reader CapnpClient::requestPath(const std::string& src, const std::string& dst) {
    auto request = schedulerService_.castAs<Scheduler>().requestPathRequest();
    request.setSrc(src);
    request.setDst(dst);
    auto response = request.send().wait(*waitScope_);
    return response.getPath();
}

void CapnpClient::reportMetrics(float throughput, float latency, float errorRate) {
    auto request = schedulerService_.castAs<Scheduler>().reportMetricsRequest();
    auto metrics = request.initMetrics();
    metrics.setThroughput(throughput);
    metrics.setLatency(latency);
    metrics.setErrorRate(errorRate);
    request.send().wait(*waitScope_);
}

bool CapnpClient::registerGpu(const std::string& uuid, const std::string& name, int64_t totalMemory) {
    auto request = schedulerService_.castAs<Scheduler>().registerGpuRequest();
    auto info = request.initInfo();
    info.setUuid(uuid);
    info.setName(name);
    info.setTotalMemory(totalMemory);
    auto response = request.send().wait(*waitScope_);
    return response.getSuccess();
}

GpuList::Reader CapnpClient::listGpus() {
    auto request = gpuService_.castAs<GpuService>().listGpusRequest();
    auto response = request.send().wait(*waitScope_);
    return response.getGpus();
}

GpuStatus::Reader CapnpClient::getGpuStatus(const std::string& uuid) {
    auto request = gpuService_.castAs<GpuService>().getGpuStatusRequest();
    auto req = request.initRequest();
    req.setUuid(uuid);
    auto response = request.send().wait(*waitScope_);
    return response.getStatus();
}

Ack::Reader CapnpClient::acquireGpu(const std::string& uuid) {
    auto request = gpuService_.castAs<GpuService>().acquireGpuRequest();
    auto req = request.initRequest();
    req.setUuid(uuid);
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

Ack::Reader CapnpClient::releaseGpu(const std::string& uuid) {
    auto request = gpuService_.castAs<GpuService>().releaseGpuRequest();
    auto req = request.initRequest();
    req.setUuid(uuid);
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

RunResponse::Reader CapnpClient::runCommand(const std::string& uuid, const std::string& cmd) {
    auto request = gpuService_.castAs<GpuService>().runCommandRequest();
    auto req = request.initRequest();
    req.setUuid(uuid);
    req.setCmd(cmd);
    auto response = request.send().wait(*waitScope_);
    return response.getResponse();
}

Ack::Reader CapnpClient::cudaInit() {
    auto request = gpuService_.castAs<GpuService>().cudaInitRequest();
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

CudaMemInfo::Reader CapnpClient::cudaMemAlloc(uint64_t size) {
    auto request = gpuService_.castAs<GpuService>().cudaMemAllocRequest();
    auto info = request.initInfo();
    info.setSize(size);
    auto response = request.send().wait(*waitScope_);
    return response.getResult();
}

Ack::Reader CapnpClient::cudaMemcpy(uint64_t dst, uint64_t src, uint64_t count, Direction direction) {
    auto request = gpuService_.castAs<GpuService>().cudaMemcpyRequest();
    auto params = request.initParams();
    params.setDst(dst);
    params.setSrc(src);
    params.setSize(count);
    params.setDirection(direction);
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

Ack::Reader CapnpClient::cudaMemFree(uint64_t addr) {
    auto request = gpuService_.castAs<GpuService>().cudaMemFreeRequest();
    auto info = request.initInfo();
    info.setAddr(addr);
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

StreamHandle::Reader CapnpClient::createCudaStream(uint32_t flags) {
    auto request = gpuService_.castAs<GpuService>().createCudaStreamRequest();
    auto params = request.initParams();
    params.setFlags(flags);
    auto response = request.send().wait(*waitScope_);
    return response.getHandle();
}

Ack::Reader CapnpClient::destroyCudaStream(uint64_t handle) {
    auto request = gpuService_.castAs<GpuService>().destroyCudaStreamRequest();
    auto h = request.initHandle();
    h.setHandle(handle);
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

Ack::Reader CapnpClient::synchronizeCudaStream(uint64_t handle) {
    auto request = gpuService_.castAs<GpuService>().synchronizeCudaStreamRequest();
    auto h = request.initHandle();
    h.setHandle(handle);
    auto response = request.send().wait(*waitScope_);
    return response.getAck();
}

RunResponse::Reader CapnpClient::cudaKernelLaunch(const std::string& uuid, const std::string& cmd) {
    auto request = gpuService_.castAs<GpuService>().cudaKernelLaunchRequest();
    auto req = request.initRequest();
    req.setUuid(uuid);
    req.setCmd(cmd);
    auto response = request.send().wait(*waitScope_);
    return response.getResponse();
}
