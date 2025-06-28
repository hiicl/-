#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>
#include <string>
#include <vector>
#include "resource_manager.h"
#include "compute_coordinator.h"
#include "decision.h"
#include "control.capnp.h"

struct Task {
    std::string kernel_name;
    std::vector<uint8_t> params;
    int timeout_ms;
    size_t data_size;
};

struct Allocation {
    std::vector<std::string> gpus;
    float bandwidth;
};

class Scheduler {
public:
    struct Request {
        std::string src_gpu;
        std::string dst_gpu;
        size_t data_size;
    };

    explicit Scheduler(std::shared_ptr<CapnpClient> client);
    
    // 任务调度接口
    void ScheduleTask(const Task& task);
    Allocation Schedule(const Request& req);
    
    // 系统管理接口
    void StartMonitoring();
    void StopMonitoring();
    void UpdateTopology();

private:
    void MonitorResources();
    void fetchTopologyFromServer();
    float calculateBandwidth(const std::string& src, const std::string& dst);

    ResourceManager resource_mgr_;
    ComputeCoordinator compute_coord_;
    DecisionEngine decision_engine_;
    std::shared_ptr<Topology> topology_;
    bool running_{false};
};

#endif // SCHEDULER_H
