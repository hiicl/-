#include "scheduler.h"
#include <thread>
#include <chrono>

Scheduler::Scheduler(std::shared_ptr<CapnpClient> client)
    : resource_mgr_(client),
      compute_coord_(client),
      decision_engine_() {}

void Scheduler::ScheduleTask(const Task& task) {
    // 1. 决策资源分配
    auto decision = decision_engine_.Evaluate(task.requirements);
    
    // 2. 申请资源
    for (const auto& gpu : decision.gpus) {
        if (!resource_mgr_.Acquire(gpu.uuid)) {
            throw std::runtime_error("Failed to acquire GPU resource");
        }
    }

    // 3. 执行计算任务
    compute_coord_.MultiGPULaunch(
        decision.gpus, 
        task.kernel_name,
        task.params
    );

    // 4. 设置自动释放
    for (const auto& gpu : decision.gpus) {
        resource_mgr_.AutoRelease(gpu.uuid, task.timeout_ms);
    }
}

void Scheduler::MonitorResources() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 检查资源状态
        auto status = resource_mgr_.CheckStatus();
        
        // 更新决策引擎
        decision_engine_.UpdateMetrics(status);
    }
}

Scheduler::Allocation Scheduler::Schedule(const Request& req) {
    // 1. 获取当前拓扑
    if (!topology_) {
        UpdateTopology();
    }

    // 2. 计算最优路径
    auto path = decision_engine_.FindPath(
        req.src_gpu,
        req.dst_gpu,
        req.data_size
    );

    // 3. 分配资源
    for (const auto& gpu : path.gpus) {
        if (!resource_mgr_.Acquire(gpu)) {
            throw std::runtime_error("Failed to acquire GPU resource");
        }
    }

    return Allocation{
        .gpus = path.gpus,
        .bandwidth = calculateBandwidth(req.src_gpu, req.dst_gpu)
    };
}

void Scheduler::UpdateTopology() {
    topology_ = fetchTopologyFromServer();
    decision_engine_.UpdateTopology(topology_);
}

void Scheduler::StartMonitoring() {
    if (!running_) {
        running_ = true;
        monitor_thread_ = std::thread(&Scheduler::MonitorResources, this);
    }
}

void Scheduler::StopMonitoring() {
    if (running_) {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }
}

std::shared_ptr<Topology> Scheduler::fetchTopologyFromServer() {
    try {
        auto topology = std::make_shared<Topology>();
        auto response = client_->GetTopology();
        
        for (const auto& node : response.getNodes()) {
            topology->addNode({
                .id = node.getId(),
                .type = static_cast<NodeType>(node.getType()),
                .gpus = node.getGpus(),
                .bandwidth = node.getBandwidth()
            });
        }

        for (const auto& link : response.getLinks()) {
            topology->addLink({
                .source = link.getSource(),
                .target = link.getTarget(),
                .latency = link.getLatency(),
                .capacity = link.getCapacity()
            });
        }

        return topology;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to fetch topology: " + std::string(e.what()));
    }
}

float Scheduler::calculateBandwidth(const std::string& src, const std::string& dst) {
    if (!topology_) {
        throw std::runtime_error("Topology not initialized");
    }

    auto path = topology_->findPath(src, dst);
    if (path.empty()) {
        return 0.0f;
    }

    float min_bandwidth = std::numeric_limits<float>::max();
    for (size_t i = 0; i < path.size() - 1; ++i) {
        const auto& link = topology_->getLink(path[i], path[i+1]);
        min_bandwidth = std::min(min_bandwidth, link.capacity);
    }

    return min_bandwidth;
}
