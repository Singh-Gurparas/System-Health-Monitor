//
// Created by whuty on 3/17/22.
//
#include <DisplayInformation.h>
#include <SystemInformation.h>
#include "imgui.h"
#include <Helper.h>
#include <Formatting.h>
#include "Graphs.h"
#include <chrono>
#include <GLFW/glfw3.h>
#include <fstream>               // *** REQUIRED for reading config file ***
#include "EmailAlert.h"          // *** REQUIRED for sending alerts ***
#include "json.hpp"              // JSON parser (nlohmann)

using json = nlohmann::json;

// Graph history buffer
static Graphs graphs(120); // store 120 samples (~120 seconds)

// ---- Thresholds loaded from config ----
static bool thresholds_loaded = false;
static float cpu_temp_threshold = 80.0f;
static float mem_percent_threshold = 90.0f;

// ---- Email Alert instance (needed in THIS file) ----
static EmailAlert g_emailAlert("config/config.json");

// ---- Load thresholds once from config/config.json ----
static void load_thresholds_from_config()
{
    if (thresholds_loaded) return;

    std::ifstream f("config/config.json");
    if (!f.is_open()) {
        std::cerr << "[WARN] Could not open config/config.json, using default thresholds.\n";
        thresholds_loaded = true;
        return;
    }

    json j;
    try {
        f >> j;
    } catch (...) {
        std::cerr << "[ERROR] Failed to parse config/config.json. Using defaults.\n";
        thresholds_loaded = true;
        return;
    }

    if (j.contains("thresholds"))
    {
        if (j["thresholds"].contains("cpu_temp_c"))
            cpu_temp_threshold = j["thresholds"]["cpu_temp_c"].get<float>();

        if (j["thresholds"].contains("memory_percent"))
            mem_percent_threshold = j["thresholds"]["memory_percent"].get<float>();
    }

    thresholds_loaded = true;
}

// -----------------------------------------------------------------------------
// --------------------------- MAIN HISTOGRAM ----------------------------------
// -----------------------------------------------------------------------------
void DisplayInformation::display_main_histogram() 
{
    load_thresholds_from_config();

    // --- SAMPLE CPU TEMP ---
    std::string cpu_raw = SystemInformation::cpu_temperature();
    float cpuTemp = std::stof(cpu_raw);

    // --- SAMPLE MEMORY LOAD ---
    std::vector<std::string> memory_information = SystemInformation::memory_information();
    std::string available = memory_information[0];
    std::string used      = memory_information[2];

    std::string a, b;
    for (int i = 0; i < used.length(); i++) {
        if (isdigit(available[i])) a.push_back(available[i]);
        if (isdigit(used[i]))      b.push_back(used[i]);
    }

    float available_f = static_cast<float>(Helper::string_to_int(a));
    float used_f      = static_cast<float>(Helper::string_to_int(b));
    float memPercent  = (used_f / available_f) * 100.0f;

    // --- MEMORY ALERT CHECK ---
    if (memPercent > mem_percent_threshold) 
    {
        std::string subject = "SystemMonitor: Memory Usage Alert";
        std::string body =
            "Memory usage exceeded threshold.\n"
            "Current: " + std::to_string(memPercent) + "%\n"
            "Threshold: " + std::to_string(mem_percent_threshold) + "%\n";

        g_emailAlert.maybeAlert("memory", subject, body, memPercent);
    }

    // --- CPU ALERT CHECK inside main histogram ---
    if (cpuTemp > cpu_temp_threshold)
    {
        std::string subject = "SystemMonitor: CPU Temperature Alert";
        std::string body =
            "CPU temperature exceeded threshold.\n"
            "Current: " + std::to_string(cpuTemp) + "°C\n"
            "Threshold: " + std::to_string(cpu_temp_threshold) + "°C\n";

        g_emailAlert.maybeAlert("cpu_temp", subject, body, cpuTemp);
    }

    // --- SAMPLE ONCE PER SECOND ---
    static double lastSample = 0.0;
    double now = glfwGetTime();
    if (lastSample == 0.0) lastSample = now;

    if (now - lastSample >= 1.0) {
        lastSample = now;
        graphs.AddCpuSample(cpuTemp);
        graphs.AddMemorySample(memPercent);
    }

    // --- DRAW GRAPHS ---
    ImGui::Begin("System Monitor");

    ImGui::Text("CPU Temperature History");
    graphs.DrawCpuGraph();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Memory Load History");
    graphs.DrawMemoryGraph();

    ImGui::End();
}

// -----------------------------------------------------------------------------
// ---------------------------- CPU TEMPERATURE --------------------------------
// -----------------------------------------------------------------------------
void DisplayInformation::display_cpu_temperature()
{
    load_thresholds_from_config();

    std::string raw = SystemInformation::cpu_temperature();
    float cpuTemp = std::stof(raw);

    // --- CPU ALERT CHECK ---
    if (cpuTemp > cpu_temp_threshold) {
        std::string subject = "SystemMonitor: CPU Temperature Alert";
        std::string body =
            "CPU temperature exceeded threshold.\n"
            "Current: " + std::to_string(cpuTemp) + "°C\n"
            "Threshold: " + std::to_string(cpu_temp_threshold) + "°C\n";

        g_emailAlert.maybeAlert("cpu_temp", subject, body, cpuTemp);
    }

    if (ImGui::CollapsingHeader("CPU Information")) {
        ImGui::BeginTable("CPU Information", 2);
        ImGui::TableNextColumn();
        ImGui::Text("CPU Temperature");

        ImGui::TableNextColumn();
        ImGui::Text("%s C°", raw.c_str());
        ImGui::EndTable();
    }
}

// -----------------------------------------------------------------------------
// ---------------------------- GPU TEMPERATURE --------------------------------
// -----------------------------------------------------------------------------
void DisplayInformation::display_gpu_temperature() 
{
    if (ImGui::CollapsingHeader("GPU Information")) {
        ImGui::BeginTable("GPU Information", 2);
        ImGui::TableNextColumn();

        ImGui::Text("GPU Temperature");

        ImGui::TableNextColumn();

        std::string gpu_temp_placeholder = "<placeholder>";
        ImGui::Text("%s", gpu_temp_placeholder.c_str());

        ImGui::EndTable();
    }
}

// -----------------------------------------------------------------------------
// ------------------------------- UPTIME --------------------------------------
// -----------------------------------------------------------------------------
void DisplayInformation::display_uptime() 
{
    if (ImGui::CollapsingHeader("Uptime")) {
        ImGui::BeginTable("Uptime Table", 2);
        ImGui::TableNextColumn();

        ImGui::Text("Uptime");

        ImGui::TableNextColumn();

        SystemInformation::current_uptime_from_proc();
        ImGui::Text("%d hours %d minutes %d seconds",
                    SystemInformation::uptime_hours(),
                    SystemInformation::uptime_minutes(),
                    SystemInformation::uptime_seconds());

        ImGui::EndTable();
    }
}

// -----------------------------------------------------------------------------
// ------------------------------ MEMORY INFO ----------------------------------
// -----------------------------------------------------------------------------
void DisplayInformation::display_memory_information() 
{
    std::vector<std::string> memory_information = SystemInformation::memory_information();

    if (ImGui::CollapsingHeader("Memory Information")) {
        ImGui::BeginTable("MemoryInformation", 3);
        ImGui::TableNextColumn();

        for (auto const& line: memory_information) {
            std::string memory_topic;
            std::string memory_data;
            bool string_selection_flag = false;

            auto const memory_info_line_length = line.length();
            for (int i = 0; i < memory_info_line_length - 2; i++) {
                if (line[i] == ' ') {
                    string_selection_flag = true;
                    continue;
                }
                string_selection_flag ?
                    memory_data.push_back(line[i]) :
                    memory_topic.push_back(line[i]);
            }

            ImGui::Text("%s", memory_topic.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", memory_data.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("kB");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
        }
        ImGui::EndTable();
    }
}

