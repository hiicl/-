@0xbac4cb17e11cf7b9;

using UUID = Text;

# ========= 控制结构 =========

struct GpuInfo {
    uuid @0 :UUID;
    name @1 :Text;
    totalMemory @2 :Int64;
}

struct Path {
    type @0 :PathType;
    steps @1 :List(Step);
    bandwidth @2 :Float32;
}

struct Step {
    device @0 :UUID;
    memType @1 :MemType;
    numaNode @2 :UInt32;
}

struct Metrics {
    throughput @0 :Float32;
    latency @1 :Float32;
    errorRate @2 :Float32;
}

enum PathType {
    nvlink @0;
    xbus @1;
    roce @2;
}

enum MemType {
    device @0;
    host @1;
    unified @2;
}

interface Scheduler {
    requestPath @0 (src :UUID, dst :UUID) -> (path :Path);
    reportMetrics @1 (metrics :Metrics) -> ();
    registerGpu @2 (info :GpuInfo) -> (success :Bool);
}
