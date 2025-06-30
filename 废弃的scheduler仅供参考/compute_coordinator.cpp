#include "compute_coordinator.h"
#include <thread>
#include <mutex>

ComputeCoordinator::ComputeCoordinator(
    std::shared_ptr<CapnpClient> client)
    : rpc_client_(client) {}

void ComputeCoordinator::MultiGPULaunch(
    const std::vector<std::string>& gpu_uuids,
    const std::string& kernel_name,
    const std::vector<uint8_t>& params) {
    
    // 1. 创建同步事件
    auto event_id = CreateSyncEvent();
    
    // 2. 在每个设备上启动内核
    std::vector<std::thread> workers;
    for (const auto& uuid : gpu_uuids) {
        workers.emplace_back([=] {
            rpc_client_->LaunchKernel(uuid, kernel_name, params);
            rpc_client_->RecordEvent(uuid, event_id);
        });
    }
    
    // 3. 等待所有设备完成
    for (auto& t : workers) {
        t.join();
    }
    
    // 4. 同步事件
    WaitForEvent(event_id);
    DestroySyncEvent(event_id);
}

std::string ComputeCoordinator::CreateSyncEvent() {
    return rpc_client_->CreateEvent();
}

void ComputeCoordinator::DestroySyncEvent(const std::string& event_id) {
    rpc_client_->DestroyEvent(event_id);
}

void ComputeCoordinator::WaitForEvent(const std::string& event_id) {
    rpc_client_->WaitEvent(event_id);
}
