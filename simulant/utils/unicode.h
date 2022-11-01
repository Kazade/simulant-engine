/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UNICODE_H
#define UNICODE_H

#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>

typedef std::u16string ustring;

class InvalidEncodingError : public std::runtime_error {
public:
    InvalidEncodingError(const std::string& what):
        std::runtime_error(what) {}
};

class unicode {
public:
    static const size_t npos = -1;
    typedef ustring::size_type size_type;
    typedef ustring::value_type value_type;

    unicode():
        string_(u"") {}

    unicode(const unicode& rhs):
        string_(rhs.string_){

    }

    unicode& operator=(const unicode& rhs);
    unicode(const char* encoded_string, const std::string& encoding="ascii");

    unicode(int32_t n, char16_t c);
    unicode(int32_t n, char c);

    unicode(const std::string& utf8_string, const std::string &encoding="ascii");
    unicode(const char16_t *utf16_string);

    template<class InputIterator>
    unicode(InputIterator begin, InputIterator end):
        string_(begin, end) {

    }

    std::wstring::size_type length() const {
        return string_.size();
    }

    std::string encode() const;

    unicode capitalize() const;
    unicode title() const;

    unicode lower() const;
    unicode upper() const;
    unicode strip() const;
    unicode lstrip() const;
    unicode rstrip() const;

    unicode strip(const unicode& things) const;
    unicode lstrip(const unicode& things) const;
    unicode rstrip(const unicode& things) const;
    unicode swap_case() const;
    unicode replace(const unicode& thing, const unicode& replacement) const;

    unicode lpad(int32_t indent);
    unicode rpad(int32_t count);

    bool contains(const unicode& thing) const;
    bool contains(const std::string& thing) const;
    bool contains(const char* thing) const;
    bool contains(const wchar_t ch) const;

    void push_back(const wchar_t c);
    void pop_back();

    unicode slice(int32_t beg, int32_t end) const;
    unicode slice(int32_t beg, void* null) const;
    unicode slice(void* null, int32_t end) const;

    bool empty() const { return string_.empty(); }
    bool starts_with(const unicode& thing) const;
    bool ends_with(const unicode& thing) const;

    std::vector<unicode> split(const unicode& on, int32_t count=-1, bool keep_empty=true) const;

    unicode join(const std::vector<unicode>& parts) const;
    unicode join(const std::vector<std::string>& parts) const;

    bool operator==(const unicode& rhs) const {
        return string_ == rhs.string_;
    }

    bool operator!=(const unicode& rhs) const {
        return !(*this == rhs);
    }

    unicode& operator=(const std::string& rhs) {
        //Automatically convert UTF-8 strings to unicode
        *this = unicode(rhs);
        return *this;
    }

    unicode& operator=(const char* rhs) {
        //Automatically convert UTF-8 strings to unicode
        *this = unicode(rhs);
        return *this;
    }

    std::size_t find(const char16_t c) const {
        return string_.find(c);
    }

    std::size_t find(const unicode& what) const {
        return string_.find(what.string_);
    }

    std::size_t rfind(const unicode& what) const {
        return string_.rfind(what.string_);
    }

    char16_t& operator[](ustring::size_type pos) {
        return string_[pos];
    }

    const char16_t& operator[](ustring::size_type pos) const {
        return string_[pos];
    }

    unicode& operator+=(const unicode& rhs) {
        string_.append(rhs.string_);
        return *this;
    }

    unicode operator+(const unicode& rhs) const {
        unicode result;
        result += *this;
        result += rhs;
        return result;
    }

    unicode operator*(const uint32_t rhs) const {
        unicode result;
        for(uint32_t i = 0; i < rhs; ++i) {
            result += *this;
        }
        return result;
    }

    bool operator<(const unicode& rhs) const {
        //FIXME: need to do a proper lexigraphical compare - probably
        return encode() < rhs.encode();
    }

    ustring::iterator begin() { return string_.begin(); }
    ustring::iterator end() { return string_.end(); }
    ustring::const_iterator begin() const { return string_.begin(); }
    ustring::const_iterator end() const { return string_.end(); }

    ustring::reverse_iterator rbegin() { return string_.rbegin(); }
    ustring::reverse_iterator rend() { return string_.rend(); }
    ustring::const_reverse_iterator rbegin() const { return string_.rbegin(); }
    ustring::const_reverse_iterator rend() const { return string_.rend(); }

    uint32_t count(const unicode& str) const;

    //Conversion functions
    int32_t to_int() const;
    float to_float() const;
    bool to_boolean() const;
    ustring to_ustring() const { return string_; }
private:
    ustring string_;
};

std::ostream& operator<< (std::ostream& os, const unicode& str);
bool operator==(const char* c_str, const unicode& uni_str);
bool operator!=(const char* c_str, const unicode& uni_str);
unicode operator+(const char* c_str, const unicode& uni_str);

namespace std {
    template<>
    struct hash<unicode> {
        size_t operator()(const unicode& str) const {
            hash<ustring> make_hash;
            return make_hash(str.to_ustring());
        }
    };
}

typedef unicode _u;

unicode humanize(int i);

#endif // UNICODE_H
