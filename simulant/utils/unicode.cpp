//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cstring>
#include <algorithm>
#include <string>
#include <iomanip>
#include <cassert>
#include <climits>
#include <cstdlib>

#include "utf8.h"
#include "unicode.h"

#ifndef __clang__
#if (__GNUC__ == 4 && __GNUC_MINOR__ <= 7)

/* GCC 4.7 doesn't define std::to_string... for some reason, so we just hack around it here */


namespace std {

static std::string to_string(uint32_t value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

}

#endif
#endif

std::ostream& operator<< (std::ostream& os, const unicode& str) {
    os << str.encode();
    return os;
}

bool operator==(const char* c_str, const unicode& uni_str) {
    return uni_str.encode() == c_str;
}

bool operator!=(const char* c_str, const unicode& uni_str) {
    return !(c_str == uni_str);
}

unicode operator+(const char* c_str, const unicode& uni_str) {
    return _u(c_str) + uni_str;
}

unicode& unicode::operator=(const unicode& rhs) {
    if(this == &rhs) {
        return *this;
    }
    this->string_ = rhs.string_;
    return *this;
}

unicode::unicode(int32_t n, char16_t c) {
    string_ = std::basic_string<char16_t>(n, c);
}

unicode::unicode(int32_t n, char c) {
    std::string s(n, c);

    *this = unicode(s.c_str());
}

unicode::unicode(const char* encoded_string, const std::string &encoding) {
    if(encoding == "ascii") {
        size_t len = strlen(encoded_string);
        string_.resize(len);
        for(size_t i = 0; i < len; ++i) {
            string_[i] = char16_t(encoded_string[i]);
        }
    } else if(encoding == "utf8" || encoding == "utf-8") {
        std::string tmp(encoded_string);
        utf8::utf8to16(tmp.begin(), tmp.end(), std::back_inserter(string_));
    } else if(encoding == "iso-8859-1" || encoding == "latin1" || encoding == "latin-1") {
        std::string tmp(encoded_string);
        std::string final;

        for(unsigned char ch: tmp) {
            if(ch < 0x80) {
                final.push_back(ch);
            } else {
                final.push_back((char) (0xc0 | (unsigned) (ch >> 6)));
                final.push_back((char) (0x80 | (ch & 0x3f)));
            }
        }

        utf8::utf8to16(final.begin(), final.end(), std::back_inserter(string_));

    } else {
        throw InvalidEncodingError("Unsupported encoding: " + encoding);
    }
}

unicode::unicode(const std::string& encoded_string, const std::string& encoding):
    unicode(encoded_string.c_str(), encoding) {
}

unicode::unicode(const char16_t *utf16_string) {
    string_ = ustring(utf16_string);
}

bool unicode::contains(const unicode& thing) const {
    return contains(thing.encode());
}

bool unicode::contains(const wchar_t ch) const {
    return string_.find(ch) != ustring::npos;
}

bool unicode::contains(const std::string& thing) const {
    bool result = string_.find(unicode(thing).string_) != ustring::npos;
    return result;
}

bool unicode::contains(const char *thing) const {
    return contains(std::string(thing));
}

std::string unicode::encode() const {
    std::string result;
    if(string_.empty()) {
        return result;
    }
    utf8::utf16to8(string_.begin(), string_.end(), std::back_inserter(result));
    return result;
}

void unicode::push_back(const wchar_t c) {
    string_.push_back(c);
}

unicode unicode::replace(const unicode& to_find, const unicode& to_replace) const {
    ustring subject = this->string_;
    ustring search = to_find.string_;
    ustring replace = to_replace.string_;

    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != ustring::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }

    return unicode(subject.begin(), subject.end());
}

unicode unicode::upper() const {
    std::vector<char16_t> result;

    for(char16_t c: *this) {
        result.push_back(std::towupper(c));
    }

    return unicode(result.begin(), result.end());
}

unicode unicode::lower() const {
    std::vector<char16_t> result;

    for(char16_t c: *this) {
        result.push_back(std::towlower(c));
    }

    return unicode(result.begin(), result.end());
}

unicode unicode::lpad(int32_t indent) {
    std::string prefix(indent, ' ');

    return _u("{0}{1}").format(prefix, *this);
}

unicode unicode::rpad(int32_t count) {
    std::string suffix(count, ' ');

    return _u("{0}{1}").format(*this, suffix);
}


std::vector<unicode> unicode::split(const unicode &delimiter, int32_t count, bool keep_empty) const {
    std::vector<unicode> result;

    unicode to_split = *this;
    unicode final_delim = delimiter;

    if (delimiter.empty()) {
        //Split on whitespace
        to_split.replace("\t", " ").replace("\n", " ").replace("\r", " ");
        final_delim = " ";
    }

    ustring::iterator substart = to_split.begin(), subend;

    while (true) {
        subend = std::search(substart, to_split.end(), final_delim.begin(), final_delim.end());
        unicode temp(substart, subend);
        if (keep_empty || !temp.empty()) {
            result.push_back(temp);
            if(result.size() == (std::size_t) count) {
                // Add the remainder to the array before returning
                substart = subend + final_delim.length();
                temp = unicode(substart, to_split.end());
                result.push_back(temp);
                return result;
            }
        }
        if (subend == to_split.end()) {
            break;
        }
        substart = subend + final_delim.length();
    }
    return result;
}

unicode unicode::join(const std::vector<std::string>& parts) const {
    std::vector<unicode> tmp_parts;
    for(std::string p: parts) {
        tmp_parts.push_back(unicode(p));
    }

    return join(tmp_parts);
}

unicode unicode::join(const std::vector<unicode>& parts) const {
    if(parts.empty()) {
        return unicode("");
    }

    if(parts.size() == 1) {
        return parts.front();
    }

    unicode final_string;
    for(unicode p: parts) {
        if(p.empty()) {
            continue;
        }
        final_string += p;
        final_string += *this;
    }

    if(final_string.empty()) {
        return unicode("");
    }

    return unicode(final_string.begin(), final_string.begin() + (final_string.length() - string_.length()));
}

unicode unicode::slice(int32_t beg, int32_t end) const {
    //Handle negative indexing
    if(beg < 0) {
        beg = length() - beg;
    }

    if(end < 0) {
        end = length() - end;
    }

    //Keep within range
    if(beg > (int) length()) {
        beg = length() - 1;
    }

    if(end > (int) length()) {
        end = length() - 1;
    }

    return unicode(begin() + beg, begin() + end);
}

unicode unicode::slice(int32_t beg, void* null) const {
    assert(!null);
    return slice(beg, length());
}

unicode unicode::slice(void* null, int32_t end) const {
    assert(!null);
    return slice((int32_t)0, end);
}


/*
 ANDROID SUCKS... can't use std::stoi and friends
*/
int32_t _stoi(const std::string& str) {
    const char* inp = str.c_str();
    char* p = nullptr;

    auto test = strtol(inp, &p, 10);

    if(p == inp || test == LONG_MIN || test == LONG_MAX) {
        throw std::invalid_argument("Couldn't convert from a string to an integer");
    }

    return (int32_t) test;
}

float _stof(const std::string& str) {
    const char* inp = str.c_str();
    char* p = nullptr;

#ifdef _arch_dreamcast
    // Dreamcast (GCC 4.7.3) doesn't define strtof but does define strtod
    float test = (float) strtod(inp, &p);
#else
    auto test = strtof(inp, &p);
#endif
    if(p == inp) {
        throw std::invalid_argument("Couldn't convert from a string to a float");
    }

    return (float) test;
}

double _stod(const std::string& str) {
    const char* inp = str.c_str();
    char* p = nullptr;

    auto test = strtod(inp, &p);

    if(p == inp) {
        throw std::invalid_argument("Couldn't convert from a string to a double");
    }

    return (double) test;
}

int32_t unicode::to_int() const {
    return _stoi(encode());
}

float unicode::to_float() const {
    return _stof(encode());
}

double unicode::to_double() const {
    return _stod(encode());
}

bool unicode::to_boolean() const {
    return lower() == "yes" || lower() == "true";
}

uint32_t unicode::count(const unicode& str) const {
    if(length() == 0) return 0;

    uint32_t count = 0;
    //FIXME Use this->find when it's implemented
    for(size_t offset = string_.find(str.string_);
        offset != std::string::npos;
        offset = string_.find(str.string_, offset + str.string_.length())) {
        ++count;
    }

    return count;
}

bool unicode::starts_with(const unicode& thing) const {
    return std::mismatch(thing.begin(), thing.end(), begin()).first == thing.end();
}

bool unicode::ends_with(const unicode& thing) const {
    return std::mismatch(thing.rbegin(), thing.rend(), rbegin()).first == thing.rend();
}

unicode unicode::rstrip(const unicode& things) const {
    unicode result = *this;
    result.string_.erase(result.string_.find_last_not_of(things.string_) + 1);
    return result;
}

unicode unicode::rstrip() const {
    unicode result = *this;

    auto& s = result.string_;
    auto it = std::find_if(s.rbegin(), s.rend(), [](const char16_t& c) -> bool { return !(iswspace(c) || iswcntrl(c)); }).base();
    s.erase(it, s.end());

    return result;
}

unicode unicode::lstrip(const unicode& things) const {
    unicode result = *this;

    result.string_.erase(result.begin(), result.begin() + result.string_.find_first_not_of(things.string_));

    return result;
}

unicode unicode::lstrip() const {
    unicode result = *this;

    auto& s = result.string_;
    auto it = std::find_if(s.begin(), s.end(), [](const char16_t& c) -> bool { return !(iswspace(c) || iswcntrl(c)); });
    s.erase(s.begin(), it);

    return result;
}


unicode unicode::strip(const unicode& things) const {
    unicode result = *this;

    return result.lstrip(things).rstrip(things);
}

unicode unicode::strip() const {
    unicode result = *this;
    return result.lstrip().rstrip();
}

unicode unicode::_do_format(uint32_t counter, const std::string& value) {
    unicode search = "{" + std::to_string(counter);

    ustring replacement(value.begin(), value.end());
    unicode result = *this;

    auto start_pos = 0;
    while(true) {
        auto pos = result.string_.find(search.string_, start_pos);
        if(pos == ustring::npos) {
            break;
        }

        auto end = result.string_.find(u"}", pos);
        if(end == ustring::npos) {
            break;
        }

        ustring contents = ustring(
            result.string_.begin() + pos + search.length(),
            result.string_.begin() + end
        );

        auto colon_idx = contents.find(':');
        if(colon_idx != ustring::npos) {
            // OK, so we have some kind of formatting parameter here... let's deal with floats only
            auto formatter = ustring(contents.begin() + colon_idx + 1, contents.end());
            if(formatter[formatter.size() - 1] == 'f') {
                if(formatter[0] == '.') {
                    auto precision_string = ustring(formatter.begin() + 1, formatter.end() - 1);
                    auto precision = _stoi(unicode(precision_string.c_str()).encode());
                    auto float_val = _stof(value);

                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(precision) << float_val;
                    replacement = unicode(ss.str()).string_;
                }
            }
        }

        result.string_.replace(
            result.string_.begin() + pos,
            result.string_.begin() + end + 1,
            replacement
        );

        start_pos = pos + replacement.length();
    }
    return result;
}


unicode humanize(int i) {
    switch(i) {
        case 0: return "zero";
        case 1: return "one";
        case 2: return "two";
        case 3: return "three";
        case 4: return "four";
        case 5: return "five";
        case 6: return "six";
        case 7: return "seven";
        case 8: return "eight";
        case 9: return "nine";
    default:
        return unicode("{0}").format(i);
    }
}
