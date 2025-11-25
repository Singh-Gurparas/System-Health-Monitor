#include "EmailAlert.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>   // popen
#include <memory>
#include <chrono>
#include <cstdlib>

#ifdef HAS_NLOHMANN_JSON
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#endif

EmailAlert::EmailAlert(const std::string& configPath) {
    // set defaults
    recipient_ = "";
    sender_ = "system-monitor@localhost";
    useMailx_ = true;
    cooldownSeconds_ = 3600; // 1 hour default

    if(!loadConfig(configPath)) {
        std::cerr << "EmailAlert: failed to load config " << configPath << ", using defaults\n";
    }
}

bool EmailAlert::loadConfig(const std::string& configPath) {
    return parseConfigFile(configPath);
}

bool EmailAlert::parseConfigFile(const std::string& configPath) {
#ifdef HAS_NLOHMANN_JSON
    std::ifstream in(configPath);
    if(!in) return false;
    json j;
    try {
        in >> j;
    } catch(...) {
        return false;
    }
    if(j.contains("email")) {
        auto &e = j["email"];
        if(e.contains("to")) recipient_ = e["to"].get<std::string>();
        if(e.contains("from")) sender_ = e["from"].get<std::string>();
        if(e.contains("use_mailx")) useMailx_ = e["use_mailx"].get<bool>();
        if(e.contains("cooldown_seconds")) cooldownSeconds_ = e["cooldown_seconds"].get<long>();
    }
    return true;
#else
    // Fallback: simple parse key=val lines
    std::ifstream in(configPath);
    if(!in) return false;
    std::string line;
    while(std::getline(in, line)) {
        // very naive parsing
        auto pos = line.find('=');
        if(pos==std::string::npos) continue;
        auto key = line.substr(0,pos);
        auto val = line.substr(pos+1);
        if(key == "email.to") recipient_ = val;
        else if(key == "email.from") sender_ = val;
        else if(key == "email.use_mailx") useMailx_ = (val == "1" || val == "true");
        else if(key == "email.cooldown_seconds") cooldownSeconds_ = std::stol(val);
    }
    return true;
#endif
}

bool EmailAlert::canSend(const std::string& metricId) {
    auto now = std::chrono::system_clock::now();
    auto it = lastSent_.find(metricId);
    if(it == lastSent_.end()) return true;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
    return elapsed >= cooldownSeconds_;
}

void EmailAlert::maybeAlert(const std::string& metricId,
                            const std::string& subject,
                            const std::string& body,
                            double value) {
    if(!canSend(metricId)) {
        // already sent recently
        return;
    }
    if(sendEmail(subject, body)) {
        lastSent_[metricId] = std::chrono::system_clock::now();
    }
}

bool EmailAlert::sendEmail(const std::string& subject, const std::string& body) {
    if(useMailx_) {
        return sendWithMailx(subject, body);
    }
    // fallback: still use mailx
    return sendWithMailx(subject, body);
}

bool EmailAlert::sendWithMailx(const std::string& subject, const std::string& body) {
    if(recipient_.empty()) {
        std::cerr << "EmailAlert: no recipient configured\n";
        return false;
    }
    // Build command that pipes body to mailx. Use popen to avoid shell injection as best as possible.
    std::ostringstream cmd;
    // Use /usr/bin/mailx or mail -s "<subject>" <recipient>
    cmd << "mailx -s " << "'" << subject << "' " << recipient_;
    FILE* pipe = popen(cmd.str().c_str(), "w");
    if(!pipe) {
        std::cerr << "EmailAlert: popen failed\n";
        return false;
    }
    // write headers optionally
    std::string header = "From: " + sender_ + "\n";
    fwrite(header.c_str(), 1, header.size(), pipe);
    fwrite(body.c_str(), 1, body.size(), pipe);
    fwrite("\n",1,1,pipe);
    int rc = pclose(pipe);
    if(rc != 0) {
        std::cerr << "EmailAlert: mailx returned nonzero code " << rc << "\n";
        return false;
    }
    return true;
}

