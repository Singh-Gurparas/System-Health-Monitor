#pragma once

#include <string>
#include <chrono>
#include <unordered_map>

class EmailAlert {
public:
    EmailAlert(const std::string& configPath);
    ~EmailAlert() = default;

    // Load or reload config file (returns true on success)
    bool loadConfig(const std::string& configPath);

    // Check metric and maybe send alert: metricId is a short key ("cpu_temp", "mem")
    void maybeAlert(const std::string& metricId,
                    const std::string& subject,
                    const std::string& body,
                    double value);

    // Direct send (bypass cooldown) - optional
    bool sendEmail(const std::string& subject, const std::string& body);

private:
    std::string recipient_;
    std::string sender_;
    bool useMailx_;
    long cooldownSeconds_;

    // When the last alert was sent for a metric
    std::unordered_map<std::string, std::chrono::system_clock::time_point> lastSent_;

    // small helper to check cooldown
    bool canSend(const std::string& metricId);

    // internal send using mailx (Option A)
    bool sendWithMailx(const std::string& subject, const std::string& body);

    // parse helpers
    bool parseConfigFile(const std::string& configPath);
};

