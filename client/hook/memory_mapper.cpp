#include "hook_cuda.h"
#include <fstream>
#include <nlohmann/json.hpp>

void MemoryMapper::AddMapping(CUdeviceptr dptr, RemoteAllocInfo info) {
    std::unique_lock lock(table_mutex);
    ptr_table[dptr] = info;
}

std::optional<RemoteAllocInfo> MemoryMapper::GetMapping(CUdeviceptr dptr) {
    std::shared_lock lock(table_mutex);
    auto it = ptr_table.find(dptr);
    if (it != ptr_table.end()) {
        return it->second;
    }
    return std::nullopt;
}

void MemoryMapper::RemoveMapping(CUdeviceptr dptr) {
    std::unique_lock lock(table_mutex);
    ptr_table.erase(dptr);
}

void MemoryMapper::SaveSnapshot(const std::string& path) {
    std::shared_lock lock(table_mutex);
    nlohmann::json j;
    
    for (const auto& entry : ptr_table) {
        j[std::to_string((uintptr_t)entry.first)] = {
            {"node_id", entry.second.node_id},
            {"size", entry.second.size},
            {"remote_handle", entry.second.remote_handle}
        };
    }
    
    std::ofstream out(path);
    out << j.dump(4);
}
