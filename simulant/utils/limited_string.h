#pragma once

#include <cstddef>
#include <string>
#include <cstring>

template<std::size_t N, typename T = char>
class LimitedString {
public:
    LimitedString() = default;
    LimitedString(const LimitedString&) = default;
    LimitedString(const char* s) {
        std::strncpy(string_, s, N);
        len_ = std::strlen(string_);
        assert(string_[N] == '\0');
    }

    bool operator==(const LimitedString& rhs) const {
        return std::strncmp(string_, rhs.string_, N) == 0;
    }

    bool operator!=(const LimitedString& rhs) const {
        return !(*this == rhs);
    }

    bool operator==(const std::string& rhs) const {
        return rhs == str();
    }

    bool operator!=(const std::string& rhs) const {
        return rhs != str();
    }

    bool operator==(const char* rhs) const {
        return std::strncmp(string_, rhs, N) == 0;
    }

    bool operator!=(const char* rhs) const {
        return std::strncmp(string_, rhs, N) != 0;
    }

    LimitedString& operator=(const LimitedString&) = default;
    LimitedString& operator=(const std::string& rhs) {
        std::strncpy(string_, rhs.c_str(), N);
        len_ = std::min((std::size_t) rhs.length(), N);
        assert(string_[N] == '\0');
        return *this;
    }

    LimitedString& operator=(const char* rhs) {
        std::strncpy(string_, rhs, N);
        len_ = std::strlen(string_);
        assert(string_[N] == '\0');
        return *this;
    }

    std::string str() const {
        return std::string(string_, len_);
    }

    const T* c_str() const {
        assert(string_[N] == '\0');
        return string_;
    }

    std::size_t length() const {
        return len_;
    }

    bool empty() const {
        return len_ == 0;
    }

    void clear() {
        len_ = 0;
        string_[0] = '\0';
    }

    std::size_t capacity() const {
        return N;
    }

private:
    T string_[N + 1] = {'\0'};
    std::size_t len_ = 0;
};
