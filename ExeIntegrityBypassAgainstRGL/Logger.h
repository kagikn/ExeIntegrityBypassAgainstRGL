#pragma once
#include <string>
#include <fstream>

enum class LogType : int {
    Info,
    // the same type as the ERROR info GTAVLauncherBypass by alloc8or can write
    Error,
};

class Logger
{
private:
    std::ofstream log_file_;

public:
    Logger() {
    };
    Logger(const std::string& file_name) {
        log_file_ = std::ofstream(file_name);
    };
    void AddLine(const LogType type, const std::string& message);

    explicit operator bool() const
    {
        return static_cast<bool>(log_file_);
    }
};