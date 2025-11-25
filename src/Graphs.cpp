#include "Graphs.h"
#include "imgui.h" // include ImGui headers as project does

Graphs::Graphs(int historyLength)
    : historyLen(historyLength),
      head(0),
      cpuMin(0.0f), cpuMax(100.0f),
      memMin(0.0f), memMax(100.0f)
{
    cpuHistory.assign(historyLen, 0.0f);
    memHistory.assign(historyLen, 0.0f);
}

void Graphs::AddCpuSample(float cpuTempC) {
    cpuHistory[head] = cpuTempC;
}

void Graphs::AddMemorySample(float memPercent) {
    memHistory[head] = memPercent;
    head = (head + 1) % historyLen;
}

void Graphs::DrawCpuGraph(const char* title) {
    ImGui::Text("%s", title);

    std::vector<float> tmp;
    tmp.reserve(historyLen);
    for (int i = 0; i < historyLen; ++i) {
        int idx = (head + i) % historyLen;
        tmp.push_back(cpuHistory[idx]);
    }

    char overlay[64];
    float latest = cpuHistory[(head + historyLen - 1) % historyLen];
    snprintf(overlay, sizeof(overlay), "Latest: %.1f°C", latest);

    // ==== YELLOW LINE ====
    ImGui::PushStyleColor(ImGuiCol_PlotLines,        ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // yellow
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(1.0f, 1.0f, 0.3f, 1.0f)); // brighter yellow

    ImGui::PlotLines("##cpu_plot",
                     tmp.data(), (int)tmp.size(),
                     0, overlay,
                     cpuMin, cpuMax,
                     ImVec2(0, 80));

    ImGui::PopStyleColor(2);
}

void Graphs::DrawMemoryGraph(const char* title) {
    ImGui::Text("%s", title);

    std::vector<float> tmp;
    tmp.reserve(historyLen);
    for (int i = 0; i < historyLen; ++i) {
        int idx = (head + i) % historyLen;
        tmp.push_back(memHistory[idx]);
    }

    char overlay[64];
    float latest = memHistory[(head + historyLen - 1) % historyLen];
    snprintf(overlay, sizeof(overlay), "%.0f%%", latest);

    // ==== YELLOW LINE ====
    ImGui::PushStyleColor(ImGuiCol_PlotLines,        ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));

    ImGui::PlotLines("##mem_plot",
                     tmp.data(), (int)tmp.size(),
                     0, overlay,
                     memMin, memMax,
                     ImVec2(0, 80));

    ImGui::PopStyleColor(2);
}

void Graphs::SetCpuRange(float minv, float maxv) {
    cpuMin = minv;
    cpuMax = maxv;
}

void Graphs::SetMemoryRange(float minv, float maxv) {
    memMin = minv;
    memMax = maxv;
}

