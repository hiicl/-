#ifndef DECISION_H
#define DECISION_H

#include "topology.h"
#include <vector>
#include <string>

struct SchedulerPolicy {
    enum class Strategy {
        LOWEST_LATENCY,
        HIGHEST_BANDWIDTH, 
        BALANCED
    };
    
    Strategy strategy;
    float bandwidth_weight;
    float latency_weight;
};

class SchedulerDecision {
public:
    explicit SchedulerDecision(const Topology& topo);
    
    Path SelectPath(
        const std::string& src,
        const std::string& dst,
        const SchedulerPolicy& policy);

private:
    Path SelectBalancedPath(
        const std::vector<Path>& paths,
        const SchedulerPolicy& policy);
        
    const Topology& topology_;
};

#endif // DECISION_H
