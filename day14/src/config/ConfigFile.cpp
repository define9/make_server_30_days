#include "ConfigFile.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

bool ConfigFile::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    data_.clear();
    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        line = trimComment(line);
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.length() - 2);
            currentSection = trim(currentSection);
            if (data_.find(currentSection) == data_.end()) {
                data_[currentSection] = Section();
            }
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = trim(line.substr(0, eqPos));
        std::string value = trim(line.substr(eqPos + 1));

        if (value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }
        if (value.front() == '\'' && value.back() == '\'') {
            value = value.substr(1, value.length() - 2);
        }

        if (!currentSection.empty()) {
            data_[currentSection][key] = value;
        }
    }

    return true;
}

bool ConfigFile::save(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& sectionPair : data_) {
        file << "[" << sectionPair.first << "]\n";
        for (const auto& keyPair : sectionPair.second) {
            file << keyPair.first << " = " << keyPair.second << "\n";
        }
        file << "\n";
    }

    return true;
}

std::string ConfigFile::get(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) return defaultValue;

    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) return defaultValue;

    return keyIt->second;
}

int ConfigFile::getInt(const std::string& section, const std::string& key, int defaultValue) const {
    std::string value = get(section, key);
    if (value.empty()) return defaultValue;

    try {
        return std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigFile::getBool(const std::string& section, const std::string& key, bool defaultValue) const {
    std::string value = get(section, key);
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);

    if (value == "true" || value == "yes" || value == "1" || value == "on") {
        return true;
    }
    if (value == "false" || value == "no" || value == "0" || value == "off") {
        return false;
    }

    return defaultValue;
}

double ConfigFile::getDouble(const std::string& section, const std::string& key, double defaultValue) const {
    std::string value = get(section, key);
    if (value.empty()) return defaultValue;

    try {
        return std::stod(value);
    } catch (...) {
        return defaultValue;
    }
}

void ConfigFile::set(const std::string& section, const std::string& key, const std::string& value) {
    data_[section][key] = value;
}

void ConfigFile::set(const std::string& section, const std::string& key, int value) {
    data_[section][key] = std::to_string(value);
}

void ConfigFile::set(const std::string& section, const std::string& key, bool value) {
    data_[section][key] = value ? "true" : "false";
}

void ConfigFile::set(const std::string& section, const std::string& key, double value) {
    data_[section][key] = std::to_string(value);
}

bool ConfigFile::hasSection(const std::string& section) const {
    return data_.find(section) != data_.end();
}

bool ConfigFile::hasKey(const std::string& section, const std::string& key) const {
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) return false;
    return sectionIt->second.find(key) != sectionIt->second.end();
}

std::vector<std::string> ConfigFile::sections() const {
    std::vector<std::string> result;
    for (const auto& pair : data_) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<std::string> ConfigFile::keys(const std::string& section) const {
    std::vector<std::string> result;
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) return result;

    for (const auto& pair : sectionIt->second) {
        result.push_back(pair.first);
    }
    return result;
}

std::string ConfigFile::trim(const std::string& s) const {
    size_t start = 0;
    while (start < s.length() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }

    size_t end = s.length();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }

    return s.substr(start, end - start);
}

std::string ConfigFile::trimComment(const std::string& s) const {
    size_t commentPos = s.find('#');
    if (commentPos == 0) return "";
    if (commentPos != std::string::npos) {
        return s.substr(0, commentPos);
    }
    return s;
}