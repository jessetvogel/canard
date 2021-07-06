//
// Created by Jesse Vogel on 01/07/2021.
//

#include <sstream>
#include <iomanip>
#include "message.h"

std::string Message::escape(const std::string &raw) {
    std::ostringstream ss;
    for (char c : raw) {
        if (c == '\"') ss << "\\\"";
        else if (c == '\\') ss << "\\\\";
        else if (c == '\b') ss << "\\b";
        else if (c == '\f') ss << "\\f";
        else if (c == '\n') ss << "\\n";
        else if (c == '\r') ss << "\\r";
        else if (c == '\t') ss << "\\t";
        else if (c >= 0x20 && c <= 0x7E) ss << c;
        else ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int) c;
    }
    return ss.str();
}

const std::string status_strings[] = {"success", "fail", "error"};

std::string Message::create(MessageStatus status, const std::string &data) {
    std::ostringstream ss;
    ss << "{\"status\":\"" << status_strings[status] << "\",\"data\":\"" << escape(data) << "\"}";
    return ss.str();
}

std::string Message::create(MessageStatus status, const std::vector<std::string> &data) {
    std::ostringstream ss;
    ss << '[';
    bool first = true;
    for (const std::string &s : data) {
        if (!first) ss << ',';
        first = false;
        ss << '"' << escape(s) << '"';
    }
    ss << ']';
    return create(status, ss.str());
}

std::string
Message::create(MessageStatus status, const std::vector<std::string> &keys, const std::vector<std::string> &values) {
    std::ostringstream ss;
    size_t n = keys.size();
    ss << '{';
    bool first = true;
    for (int i = 0; i < n; ++i) {
        if (!first) ss << ',';
        first = false;
        ss << '"' << escape(keys[i]) << "\":\"" << escape(values[i]) << '"';
    }
    ss << '}';
    return create(status, ss.str());
}
