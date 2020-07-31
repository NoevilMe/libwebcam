#ifndef __STRING_UTIL_HPP_
#define __STRING_UTIL_HPP_

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace string_util {

void basic_split(const std::string &str, std::vector<std::string> &strs) {
    std::istringstream iss(str);
    copy(std::istream_iterator<std::string>(iss),
         std::istream_iterator<std::string>(), back_inserter(strs));
}

void basic_split(const std::string &str, std::vector<std::string> &strs,
                 const char sep) {
    int begin_idx = 0;
    int end_idx = str.find(sep);
    if (end_idx == -1)
        return;

    while (end_idx != -1) {
        strs.push_back(str.substr(begin_idx, end_idx - begin_idx));
        begin_idx = end_idx + 1;
        end_idx = str.find(sep, begin_idx);
    }
    strs.push_back(str.substr(begin_idx));
}

void split(const std::string &str, std::vector<std::string> &strs) {
    strs.clear();
    basic_split(str, strs);
}

void split(const std::string &str, std::vector<std::string> &strs,
           const char sep = ' ') {
    strs.clear();
    basic_split(str, strs, sep);
}

std::vector<std::string> split(const std::string &str) {
    std::vector<std::string> strs;
    basic_split(str, strs);
    return strs;
}

std::vector<std::string> split(const std::string &str, const char sep = ' ') {
    std::vector<std::string> strs;
    basic_split(str, strs, sep);
    return strs;
}

void basic_join(const std::vector<std::string> &strs, std::string &str,
                const char sep) {
    for_each(strs.begin(), strs.end() - 1,
             [&](const std::string &s) { str += s + sep; });
    str += strs[strs.size() - 1];
}

void basic_join(const std::vector<std::string> &strs, std::string &str,
                const std::string &sep) {
    for_each(strs.begin(), strs.end() - 1,
             [&](const std::string &s) { str += s + sep; });
    str += strs[strs.size() - 1];
}

void join(const std::vector<std::string> &strs, std::string &str,
          const char sep = ' ') {
    str = "";
    basic_join(strs, str, sep);
}

std::string join(const std::vector<std::string> &strs, const char sep) {
    std::string str;
    basic_join(strs, str, sep);
    return str;
}

std::string join(const std::vector<std::string> &strs, const std::string &sep) {
    std::string str;
    basic_join(strs, str, sep);
    return str;
}

void left_trim(std::string &str) { str.erase(0, str.find_first_not_of(' ')); }

void right_trim(std::string &str) { str.erase(str.find_last_not_of(' ') + 1); }

void trim(std::string &str) {
    left_trim(str);
    right_trim(str);
}

void lower(std::string &str) {
    for_each(str.begin(), str.end(), [&](char &ch) { ch = tolower(ch); });
}

void upper(std::string &str) {
    for_each(str.begin(), str.end(), [&](char &ch) { ch = toupper(ch); });
}

void captain(std::string &str) {
    lower(str);
    str[0] = toupper(str[0]);
}

template <typename T> std::string to_string(T value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T> T string_to(std::string str, T value) {
    std::istringstream iss(str);
    iss >> value;
    return value;
}

std::string trim_new(std::string &str) {
    std::string new_str = str;
    trim(new_str);
    return new_str;
}

std::string left_trim_new(std::string &str) {
    std::string new_str = str;
    left_trim(new_str);
    return new_str;
}

std::string right_trim_new(std::string &str) {
    std::string new_str = str;
    right_trim(new_str);
    return new_str;
}

std::string lower_new(std::string &str) {
    std::string new_str = str;
    lower(new_str);
    return new_str;
}

std::string upper_new(std::string &str) {
    std::string new_str = str;
    upper(new_str);
    return new_str;
}

std::string captain_new(std::string &str) {
    std::string new_str = str;
    captain(new_str);
    return new_str;
}

} // namespace string_util

#endif /* __STRING_UTIL_HPP_ */
