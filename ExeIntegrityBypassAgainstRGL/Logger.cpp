#include "pch.h"
#include "Logger.h"
#include <format>
#include <chrono>

std::string CreateStringWithBracket(const std::string& str) {
    std::string str_with_bracket = std::string(str);
    str_with_bracket.insert(0, "[");
    str_with_bracket += "]";

    return str_with_bracket;
}

// Using std::chrono stuff will result in a large dll
std::string CreateIso8061LocalTimeStringNoTSeparator(const std::time_t unixTime) {
    __time64_t time;
    _time64(&time);

    struct tm local_time;
    _localtime64_s(&local_time, &time);

    constexpr size_t BUF_SIZE_TIME_STR = 0x50u;
    std::string temp_buf;
    temp_buf.resize(BUF_SIZE_TIME_STR);

    const size_t written_char_count = strftime(temp_buf.data(), BUF_SIZE_TIME_STR, "%Y-%m-%d %H:%M:%S", &local_time);
    temp_buf.resize(written_char_count);

    return temp_buf;
}

std::string GetLogTypeEnumString(LogType type) {
    switch (type) {
        case LogType::Info:
            return std::string("INFO");
        case LogType::Error:
            return std::string("ERROR");
        default: return std::string("UNKNOWN");
    }
}

void Logger::AddLine(const LogType type, const std::string& message) {
    std::time_t unixTimeScanStarted = std::time(nullptr);
    const auto formatted_time_str = CreateIso8061LocalTimeStringNoTSeparator(unixTimeScanStarted);
    log_file_ << "[" << formatted_time_str << "]" << " ";

    log_file_ << "[" << GetLogTypeEnumString(type) << "]" << " ";
    
    log_file_ << message << std::endl;
}