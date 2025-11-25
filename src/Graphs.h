#pragma once
#include <vector>
#include <string>

struct Graphs {
    // configurable history length
    static constexpr int DEFAULT_HISTORY = 120; // 120 samples (e.g. 120 frames / ~2 minutes @1Hz)

    Graphs(int historyLength = DEFAULT_HISTORY);

    // push a new sample for CPU temp (C) and memory load (0-100)
    void AddCpuSample(float cpuTempC);
    void AddMemorySample(float memPercent);

    // call from your ImGui render code
    void DrawCpuGraph(const char* title = "CPU Temp (°C)");
    void DrawMemoryGraph(const char* title = "Memory Load (%)");

    // optional: change ranges
    void SetCpuRange(float minv, float maxv);
    void SetMemoryRange(float minv, float maxv);

private:
    std::vector<float> cpuHistory;
    std::vector<float> memHistory;
    int historyLen;
    int head; // next index to write (ring buffer)
    float cpuMin, cpuMax;
    float memMin, memMax;

    // helpers
    const float* cpuDataPtr() const { return cpuHistory.data(); }
    const float* memDataPtr() const { return memHistory.data(); }
};

