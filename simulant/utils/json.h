#pragma once

#include <iosfwd>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "../generic/optional.h"
#include "../path.h"
#include "formatter.h"

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

class JSONIterator;

template<typename K, typename V>
class InsertionOrderedDict {
private:
public:
    typedef typename std::pair<K, V> Entry;

    void insert(const std::pair<K, V>& p) {
        // FIXME: blow on duplicate?
        entries_.push_back(p);
    }

    void clear() {
        entries_.clear();
    }

    typename std::vector<Entry>::const_iterator begin() const {
        return entries_.begin();
    }

    typename std::vector<Entry>::const_iterator end() const {
        return entries_.end();
    }

    typename std::vector<Entry>::iterator begin() {
        return entries_.begin();
    }

    typename std::vector<Entry>::iterator end() {
        return entries_.end();
    }

    V& operator[](const K& key) {
        for(auto& e: entries_) {
            if(e.first == key) {
                return e.second;
            }
        }

        entries_.push_back(std::make_pair(key, V()));
        return entries_.back().second;
    }

    std::size_t count(const K& key) const {
        for(auto& e: entries_) {
            if(e.first == key) {
                return 1;
            }
        }

        return 0;
    }

    V& at(const K& key) {
        for(auto& e: entries_) {
            if(e.first == key) {
                return e.second;
            }
        }

        throw std::out_of_range("Key not found");
    }

    bool has_key(const K& key) const {
        for(auto& e: entries_) {
            if(e.first == key) {
                return true;
            }
        }

        return false;
    }

    std::size_t size() const {
        return entries_.size();
    }

private:
    std::vector<Entry> entries_;
};

class JSONNode {
public:
    JSONNode() = default;

    JSONNode(JSONNodeType type, JSONNode* parent) :
        type_(type), parent_(parent) {

        switch(type) {
            case JSON_OBJECT:
                value_ = ObjectType();
                break;
            case JSON_ARRAY:
                value_ = ArrayType();
                break;
            default:
                value_ = ValueType();
        }
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

    bool is_null() const;

    /* Convert the value to a string, this is well-defined for all value types
     * and will return the following:
     *
     *  - STRING - returns the verbatim value
     *  - NUMBER - returns the stringified floating point value
     *  - TRUE - returns "true"
     *  - FALSE - returns "false"
     *  - NULL - returns "null"
     */
    optional<std::string> to_str() const {
        if(type_ == JSON_STRING) {
            return std::get<std::string>(value_);
        }

        return no_value;
    }

    std::string repr() const {
        switch(type_) {
            case JSON_OBJECT:
                return _F("{{0}...}").format(this->size());
            case JSON_ARRAY:
                return _F("[{0}...]").format(this->size());
            case JSON_STRING:
            case JSON_NUMBER:
                return std::get<std::string>(value_);
            case JSON_TRUE:
                return "true";
            case JSON_FALSE:
                return "false";
            case JSON_NULL:
                return "null";
        }
    }

    /* Returns the value as an integer if the type is NUMBER */
    optional<int64_t> to_int() const;

    /* Returns the value as a float if the type is NUMBER */
    optional<float> to_float() const;

    /* Returns the value as a bool if the type is TRUE, FALSE, or NULL.
     * NULL always returns false. */
    optional<bool> to_bool() const;

    /* Returns an iterator pointing to this node */
    JSONIterator to_iterator() const;

    bool is_float() const;

    typedef InsertionOrderedDict<std::string, std::shared_ptr<JSONNode>>
        ObjectType;
    typedef std::vector<std::shared_ptr<JSONNode>> ArrayType;
    typedef std::string ValueType;

private:
    friend class JSONIterator;
    JSONNodeType type_ = JSON_NULL;
    JSONNode* parent_ = nullptr;

    friend std::shared_ptr<JSONNode> parse_array(_json_impl::IStreamPtr stream,
                                                 JSONNode* parent);
    friend std::shared_ptr<JSONNode> parse_object(_json_impl::IStreamPtr stream,
                                                  JSONNode* parent);
    friend std::shared_ptr<JSONNode> parse_node(_json_impl::IStreamPtr stream,
                                                JSONNode* parent);

    std::variant<ObjectType, ArrayType, ValueType> value_ = ValueType();
};

class JSONIterator {
    friend JSONIterator json_parse(const std::string&);
    friend JSONIterator json_load(const Path&);
    friend JSONIterator json_read(std::shared_ptr<std::istream>);

    friend class JSONNode;

private:
    JSONIterator() {
        /* We set the current node to the static invalid node
           so we can do comparisons, and accessors always return
           a NULL node and not a null ptr
        */
        current_node_ = invalid_node;
    }

    JSONIterator(std::shared_ptr<JSONNode> node) :
        is_array_iterator_(node->parent_ &&
                           node->parent_->type_ == JSON_ARRAY) {

        current_node_ = node;
    }

    bool is_array_iterator_ = false;

    std::shared_ptr<JSONNode> current_node_;

    void set_invalid(const std::string& message);

public:
    static std::shared_ptr<JSONNode> invalid_node;

    typedef JSONNode value_type;
    typedef JSONNode* pointer;
    typedef JSONNode reference;
    typedef std::input_iterator_tag iterator_category;

    bool is_valid() const {
        return current_node_ != invalid_node;
    }

    JSONNode* operator->() const {
        return current_node_.get();
    }

    JSONNode& operator*() const {
        return *current_node_.get();
    }

    JSONIterator operator[](const std::string& key) const;
    JSONIterator operator[](const std::size_t i) const;

    /* Range-loop iteration, only for array items */
    explicit operator bool() const {
        return is_valid();
    }

    JSONIterator begin() const;

    JSONIterator end() const {
        return JSONIterator();
    }

    JSONIterator& operator++();
    bool operator==(const JSONIterator& rhs) const {
        if(!is_valid() && !rhs.is_valid()) {
            return true;
        }

        return current_node_.get() == rhs.current_node_.get();
    }

    bool operator!=(const JSONIterator& rhs) const {
        return !((*this) == rhs);
    }

    bool is_array_iterator() const {
        return is_array_iterator_;
    }
};

template<typename T>
optional<T> json_auto_cast(JSONIterator it);

template<>
inline optional<int> json_auto_cast<int>(JSONIterator it) {
    return optional<int>(it->to_int().value());
}

template<>
inline optional<long> json_auto_cast<long>(JSONIterator it) {
    return optional<long>(it->to_int().value());
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
JSONIterator json_read(std::shared_ptr<std::istream> stream);

} // namespace smlt
