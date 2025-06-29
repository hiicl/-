#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include "capnp_client.h"

// 远程节点信息
struct RemoteNode {
    // 默认构造函数
    RemoteNode() = default;
    
    // 带参数的构造函数
    RemoteNode(std::string id, std::string name, std::string address, 
               std::string roce_interface, int priority,
               size_t total_memory, size_t available_memory,
               double network_latency, double cpu_usage, 
               double gpu_utilization)
        : id(std::move(id)), name(std::move(name)), address(std::move(address)),
          roce_interface(std::move(roce_interface)), priority(priority),
          total_memory(total_memory), available_memory(available_memory),
          network_latency(network_latency), cpu_usage(cpu_usage),
          gpu_utilization(gpu_utilization) {}
    
    // 移动语义
    RemoteNode(RemoteNode&&) = default;
    RemoteNode& operator=(RemoteNode&&) = default;
    
    // 删除拷贝语义
    RemoteNode(const RemoteNode&) = delete;
    RemoteNode& operator=(const RemoteNode&) = delete;

    std::string id;
    std::string name;  // 添加 name 字段
    std::string address;
    std::string roce_interface;
    int priority;
    size_t total_memory;
    size_t available_memory;
    double network_latency;  // 单位：毫秒
    double cpu_usage;        // CPU使用率百分比
    double gpu_utilization;  // GPU利用率百分比
    std::unique_ptr<CapnpClient> capnp_client;
};

// 负载均衡调度器
class Dispatcher {
    std::vector<RemoteNode> nodes;
    std::mutex mutex;

public:
    // 添加节点（使用移动语义）
    void AddNode(RemoteNode&& node);
    
    // 获取所有节点
    std::vector<RemoteNode>& GetNodes();
    
    // 从YAML文件加载节点配置
    bool LoadConfig(const std::string& config_path);
    
    // 选择最佳节点
    RemoteNode* PickNode(size_t required_memory);
    
    // 根据ID获取节点
    RemoteNode* GetNodeById(const std::string& id);

    // 获取默认节点
    RemoteNode* GetDefaultNode();
};
