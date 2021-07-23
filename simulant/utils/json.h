#pragma once

#include <memory>
#include <iosfwd>
#include <string>

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

    /* Returns true if the type is a NUMBER, STRING, TRUE, FALSE, or NULL */
    bool is_value_type() const;

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
    optional<int64_t> to_int() const {
        if(type_ != JSON_NUMBER) {
            return optional<int64_t>();
        }

        return optional<int64_t>(std::stoi(read_value_from_stream()));
    }

    /* Returns the value as a float if the type is NUMBER */
    optional<float> to_float() const {
        if(type_ != JSON_NUMBER) {
            return optional<float>();
        }

        return optional<float>(std::stof(read_value_from_stream()));
    }

    /* Returns the value as a bool if the type is TRUE, FALSE, or NULL.
     * NULL always returns false. */
    optional<bool> to_bool() const {
        switch(type_) {
            case JSON_FALSE:
            case JSON_NULL:
                return optional<bool>(false);
            case JSON_TRUE:
                return optional<bool>(true);
            default:
                return optional<bool>();
        }
    }

private:
    friend class JSONIterator;
    mutable _json_impl::IStreamPtr stream_;
    std::streampos start_ = 0;
    std::streampos end_ = 0;
    std::size_t size_ = 0;
    JSONNodeType type_ = JSON_OBJECT;
};

class JSONIterator {
    friend JSONIterator json_parse(const std::string&);
    friend JSONIterator json_load(const Path&);

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

    JSONIterator operator[](const std::string& key);
    JSONIterator operator[](const std::size_t i);
};

JSONIterator json_load(const Path& path);
JSONIterator json_parse(const std::string& data);

}
