#ifndef COMPUTE_COORDINATOR_H
#define COMPUTE_COORDINATOR_H

#include <vector>
#include <string>
#include <memory>
#include "capnp_client.h"

class ComputeCoordinator {
public:
    explicit ComputeCoordinator(std::shared_ptr<CapnpClient> client);
    
    // 多GPU协同计算
    void MultiGPULaunch(
        const std::vector<std::string>& gpu_uuids,
        const std::string& kernel_name,
        const std::vector<uint8_t>& params);
        
    // 创建跨设备同步事件
    std::string CreateSyncEvent();
    
    // 销毁同步事件
    void DestroySyncEvent(const std::string& event_id);
    
    // 等待事件完成
    void WaitForEvent(const std::string& event_id);

private:
    std::shared_ptr<CapnpClient> rpc_client_;
    
    // 实现细节...
};

#endif // COMPUTE_COORDINATOR_H
