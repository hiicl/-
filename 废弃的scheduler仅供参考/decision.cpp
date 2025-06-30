#include "decision.h"
#include <algorithm>
#include <numeric>

SchedulerDecision::SchedulerDecision(const Topology& topo) 
    : topology_(topo) {}

Path SchedulerDecision::SelectPath(
    const std::string& src, 
    const std::string& dst,
    const SchedulerPolicy& policy) {
    
    // 1. 获取所有可能路径
    auto paths = topology_.FindAllPaths(src, dst);
    
    // 2. 应用调度策略
    switch(policy.strategy) {
        case Strategy::LOWEST_LATENCY:
            return *std::min_element(paths.begin(), paths.end(),
                [](const Path& a, const Path& b) {
                    return a.latency < b.latency;
                });
            
        case Strategy::HIGHEST_BANDWIDTH:
            return *std::max_element(paths.begin(), paths.end(),
                [](const Path& a, const Path& b) {
                    return a.bandwidth < b.bandwidth;
                });
            
        case Strategy::BALANCED:
            return SelectBalancedPath(paths, policy);
    }
}

Path SchedulerDecision::SelectBalancedPath(
    const std::vector<Path>& paths,
    const SchedulerPolicy& policy) {
    
    if (paths.empty()) {
        throw std::runtime_error("No available paths");
    }

    // 计算每条路径的得分
    std::vector<float> scores;
    for (const auto& path : paths) {
        float score = policy.bandwidth_weight * path.bandwidth + 
                      policy.latency_weight * (1.0f / path.latency);
        scores.push_back(score);
    }

    // 选择得分最高的路径
    auto max_it = std::max_element(scores.begin(), scores.end());
    return paths[std::distance(scores.begin(), max_it)];
}

void SchedulerDecision::UpdateMetrics(const ResourceStatus& status) {
    // 更新拓扑中的资源利用率信息
    for (auto& node : topology_.nodes) {
        if (status.utilization.find(node.id) != status.utilization.end()) {
            node.utilization = status.utilization.at(node.id);
        }
    }
}

void SchedulerDecision::UpdateTopology(std::shared_ptr<Topology> topo) {
    if (!topo) {
        throw std::invalid_argument("Invalid topology pointer");
    }
    topology_ = *topo;
}
