#include "capnp_client.h"
#include <memory>
#include <thread>
#include <mutex>
#include <capnp/serialize.h>
#include <vector>

struct GPUStatus {
    std::string id;
    std::string name;
    float utilization;
    uint64_t memory_used;
    float temperature;
};

class GPUMonitorClient {
public:
    GPUMonitorClient(std::shared_ptr<CapnpClient> client) 
        : client_(client) {}
        
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
            auto status = FetchStatus();
            if (!status.empty()) {
                std::lock_guard<std::mutex> lock(mutex_);
                status_ = std::move(status);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::vector<GPUStatus> FetchStatus() {
        // 调用服务端RPC获取状态
        auto result = client_->Call("GPU.GetStatus");
        return ParseStatus(result);
    }
    
    std::vector<GPUStatus> ParseStatus(const capnp::FlatArrayMessageReader& msg) {
        std::vector<GPUStatus> status;
        // 解析capnp消息
        // ...
        return status;
    }

    std::shared_ptr<CapnpClient> client_;
    std::thread worker_;
    std::mutex mutex_;
    std::vector<GPUStatus> status_;
    bool running_ = false;
};
