#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include "capnp_client.h"

class ResourceManager {
public:
    explicit ResourceManager(std::shared_ptr<CapnpClient> client);
    
    // 资源申请
    bool Acquire(const std::string& gpu_uuid);
    
    // 资源释放
    void Release(const std::string& gpu_uuid);
    
    // 自动释放(超时机制)
    void AutoRelease(const std::string& gpu_uuid, int timeout_ms);
    
    // 状态检查
    bool IsInUse(const std::string& gpu_uuid) const;

private:
    struct ResourceEntry {
        bool in_use;
        std::chrono::steady_clock::time_point acquire_time;
    };
    
    std::shared_ptr<CapnpClient> rpc_client_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ResourceEntry> resources_;
};

#endif // RESOURCE_MANAGER_H
