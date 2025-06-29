#include "capnp_client.h"
#include <memory>
#include <thread>
#include <mutex>
#include <capnp/serialize.h>
#include <vector>
#include <iostream> // 添加错误处理

struct GPUStatus {
    std::string id;
    std::string name;
    float utilization;
    uint64_t memory_used;
    float temperature;
};

class GPUMonitorClient {
public:
    GPUMonitorClient(std::shared_ptr<CapnpClient> client, const std::string& gpu_uuid) 
        : client_(client), gpu_uuid_(gpu_uuid) {}
        
    void Start() {
        running_ = true;
        worker_ = std::thread(&GPUMonitorClient::UpdateLoop, this);
    }
    
    void Stop() {
        running_ = false;
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    
    std::vector<GPUStatus> GetStatus() {
        std::lock_guard<std::mutex> lock(mutex_);
        return status_;
    }

private:
    void UpdateLoop() {
        while (running_) {
            try {
                auto status = FetchStatus();
                if (!status.empty()) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    status_ = std::move(status);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error fetching GPU status: " << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::vector<GPUStatus> FetchStatus() {
        std::vector<GPUStatus> status;
        // 使用强类型RPC调用
        auto status_reader = client_->getGpuStatus(gpu_uuid_);
        
        GPUStatus gpu_status;
        gpu_status.id = gpu_uuid_;
        gpu_status.utilization = status_reader.getUtilization();
        gpu_status.memory_used = status_reader.getUsedMemory();
        // 温度信息需要根据协议扩展
        gpu_status.temperature = 0.0f; 
        
        status.push_back(gpu_status);
        return status;
    }
    
    // 删除废弃的ParseStatus函数

    std::shared_ptr<CapnpClient> client_;
    std::string gpu_uuid_; // 添加GPU UUID成员
    std::thread worker_;
    std::mutex mutex_;
    std::vector<GPUStatus> status_;
    bool running_ = false;
};
