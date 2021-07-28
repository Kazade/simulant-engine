#pragma once

#include <memory>
#include <iosfwd>
#include <string>
#include <vector>

#include "../generic/optional.h"
#include "../path.h"

namespace smlt {

enum JSONNodeType {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL
};

namespace _json_impl {
    typedef std::shared_ptr<std::istream> IStreamPtr;
}

class JSONNode {
private:
    std::string read_value_from_stream() const;
public:
    JSONNode() = default;
    JSONNode(_json_impl::IStreamPtr stream, std::streampos start, std::streampos end, std::size_t size=0):
        stream_(stream), start_(start), end_(end), size_(size) {}

    std::streampos start() const {
        return start_;
    }

    std::streampos end() const {
        return end_;
    }

    JSONNodeType type() const;

    /* For non-value-types, returns the number
     * of child items. Value types return 0 */
    std::size_t size() const;

    /* Returns true if this is an object, that contains the
     * provided key */
    bool has_key(const std::string& key) const;

    /* Returns the keys found in the object, if this
     * node is an object */
    std::vector<std::string> keys() const;

    /* Returns true if the type is a NUMBER, STRING, TRUE, FALSE, or NULL */
    bool is_value_type() const;

    bool is_bool() const {
        return type_ == JSON_FALSE || type_ == JSON_TRUE;
    }

    bool is_str() const {
        return type_ == JSON_STRING;
    }

    bool is_number() const {
        return type_ == JSON_NUMBER;
    }

    bool is_array() const {
        return type_ == JSON_ARRAY;
    }

    bool is_object() const {
        return type_ == JSON_OBJECT;
    }
    /* Convert the value to a string, this is well-defined for all value types
     * and will return the following:
     *
     *  - STRING - returns the verbatim value
     *  - NUMBER - returns the stringified floating point value
     *  - TRUE - returns "true"
     *  - FALSE - returns "false"
     *  - NULL - returns "null"
     */
    std::string to_str() const {
        switch(type_) {
            case JSON_OBJECT: return "{}";
            case JSON_ARRAY: return "[]";
            case JSON_STRING: return read_value_from_stream();
            case JSON_NUMBER: return read_value_from_stream();
            case JSON_TRUE: return "true";
            case JSON_FALSE: return "false";
            case JSON_NULL: return "null";
            default:
                return "";
        }
    }

    /* Returns the value as an integer if the type is NUMBER */
    optional<int64_t> to_int() const;

    /* Returns the value as a float if the type is NUMBER */
    optional<float> to_float() const;

    /* Returns the value as a bool if the type is TRUE, FALSE, or NULL.
     * NULL always returns false. */
    optional<bool> to_bool() const;

    bool is_null() const;

private:
    friend class JSONIterator;
    mutable _json_impl::IStreamPtr stream_;
    std::streampos start_ = 0;
    std::streampos end_ = 0;
    std::size_t size_ = 0;
    JSONNodeType type_ = JSON_OBJECT;

    /* Internal function. If this is an object type,
     * will read from start to end and call cb() with
     * each key found */
    template<typename Func>
    void read_keys(Func&& cb) const;
};

class JSONIterator {
    friend JSONIterator json_parse(const std::string&);
    friend JSONIterator json_load(const Path&);
    friend JSONIterator json_read(std::shared_ptr<std::istream>&);

private:
    JSONIterator() = default;  /* Invalid or end */

    JSONIterator(_json_impl::IStreamPtr stream, std::streampos pos):
        stream_(stream) {
        current_node_ = std::make_shared<JSONNode>();
        parse_node(*current_node_, stream, pos);
    }


    _json_impl::IStreamPtr stream_;

    std::shared_ptr<JSONNode> current_node_;

    void parse_node(JSONNode& node, _json_impl::IStreamPtr stream, std::streampos pos);
    void set_invalid(const std::string& message);

public:
    typedef JSONNode value_type;
    typedef JSONNode* pointer;
    typedef JSONNode& reference;
    typedef std::input_iterator_tag iterator_category;

    bool is_valid() const {
        return bool(current_node_);
    }

    JSONNode* operator->() const {
        return current_node_.get();
    }

    JSONIterator operator[](const std::string& key) const;
    JSONIterator operator[](const std::size_t i) const;
};

template<typename T>
optional<T> json_auto_cast(JSONIterator it);

template<>
inline optional<int64_t> json_auto_cast<int64_t>(JSONIterator it) {
    return it->to_int();
}

template<>
inline optional<int> json_auto_cast<int>(JSONIterator it) {
    return optional<int>(it->to_int().value());
}

template<>
inline optional<float> json_auto_cast<float>(JSONIterator it) {
    return it->to_float();
}

template<>
inline optional<bool> json_auto_cast<bool>(JSONIterator it) {
    return it->to_bool();
}

template<>
inline optional<std::string> json_auto_cast<std::string>(JSONIterator it) {
    return optional<std::string>(it->to_str());
}

JSONIterator json_load(const Path& path);
JSONIterator json_parse(const std::string& data);
JSONIterator json_read(std::shared_ptr<std::istream>& stream);

}
