#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <string>
#include <vector>
#include <unordered_map>

struct Link {
    std::string target_gpu;
    float bandwidth;
};

struct GPUNode {
    std::string id;
    int numa_node;
    std::vector<Link> links;
};

class Topology {
public:
    void Update(const std::vector<GPUNode>& nodes);
    std::vector<std::string> FindOptimalPath(const std::string& src, const std::string& dst);
    float GetBandwidth(const std::string& src, const std::string& dst);

private:
    std::unordered_map<std::string, GPUNode> nodes_;
};

#endif // TOPOLOGY_H
