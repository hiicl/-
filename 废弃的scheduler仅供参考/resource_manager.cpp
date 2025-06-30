#include "resource_manager.h"
#include <chrono>
#include <thread>

ResourceManager::ResourceManager(std::shared_ptr<CapnpClient> client)
    : rpc_client_(client) {}

bool ResourceManager::Acquire(const std::string& gpu_uuid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查资源是否可用
    auto it = resources_.find(gpu_uuid);
    if (it != resources_.end() && it->second.in_use) {
        return false;
    }

    // 向服务器申请资源
    if (!rpc_client_->acquireGpu(gpu_uuid)) {
        return false;
    }

    // 更新本地状态
    resources_[gpu_uuid] = {
        .in_use = true,
        .acquire_time = std::chrono::steady_clock::now()
    };
    return true;
}

void ResourceManager::Release(const std::string& gpu_uuid) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = resources_.find(gpu_uuid);
    if (it != resources_.end()) {
        rpc_client_->releaseGpu(gpu_uuid);
        resources_.erase(it);
    }
}

void ResourceManager::AutoRelease(const std::string& gpu_uuid, int timeout_ms) {
    std::thread([this, gpu_uuid, timeout_ms]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
        Release(gpu_uuid);
    }).detach();
}

bool ResourceManager::IsInUse(const std::string& gpu_uuid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = resources_.find(gpu_uuid);
    return it != resources_.end() && it->second.in_use;
}
