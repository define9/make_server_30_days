#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <string>
#include <map>
#include <vector>

class ConfigFile {
public:
    ConfigFile() = default;
    ~ConfigFile() = default;

    bool load(const std::string& filepath);
    bool save(const std::string& filepath);

    std::string get(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const;
    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const;

    void set(const std::string& section, const std::string& key, const std::string& value);
    void set(const std::string& section, const std::string& key, int value);
    void set(const std::string& section, const std::string& key, bool value);
    void set(const std::string& section, const std::string& key, double value);

    bool hasSection(const std::string& section) const;
    bool hasKey(const std::string& section, const std::string& key) const;

    std::vector<std::string> sections() const;
    std::vector<std::string> keys(const std::string& section) const;

private:
    std::string trim(const std::string& s) const;
    std::string trimComment(const std::string& s) const;

    using Section = std::map<std::string, std::string>;
    std::map<std::string, Section> data_;
};

#endif // CONFIG_FILE_H