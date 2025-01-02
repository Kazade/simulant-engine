#include "json.h"
#include "../logging.h"
#include <cstddef>
#include <istream>
#include <sstream>

namespace smlt {

const std::string WHITESPACE = "\t\n\r ";
std::shared_ptr<JSONNode> JSONIterator::invalid_node =
    std::make_shared<JSONNode>();

static void unget(_json_impl::IStreamPtr& stream) {
    /* Frustratingly, if you hit the end of the stream
     * and then "unget()", it only clears the eof bit, and
     * not the fail bit. So then everything fails...
     *
     * This calls unget then clears it all */

    stream->unget();
    assert(!stream->eof());

    stream->clear();
}

static std::string read_value(_json_impl::IStreamPtr& stream) {
    std::string buffer;
    while(!stream->eof()) {
        char c = stream->peek();

        if(c == ',' || c == '}' || c == ']') {
            return buffer;
        } else {
            buffer += stream->get();
        }
    }

    return "";
}

static std::string read_string(_json_impl::IStreamPtr& stream) {
    std::string buffer;
    while(!stream->eof()) {
        char c = stream->get();

        if(c == '"') {
            return buffer;
        } else {
            buffer += c;
        }
    }

    return "";
}

static bool check_remainder(_json_impl::IStreamPtr& stream,
                            const std::string& rest) {
    for(auto& l: rest) {
        if(stream->get() != l) {
            return false;
        }
    }

    return true;
}

static std::streampos seek_next_not_of(_json_impl::IStreamPtr& stream,
                                       const std::string& chars) {
    while(!stream->eof()) {
        char c = stream->get();
        if(stream->eof()) {
            break;
        }

        if(chars.find(c) == std::string::npos) {
            unget(stream);
            return stream->tellg();
        }
    }

    return std::streampos(-1);
}

static std::streampos skip_whitespace(_json_impl::IStreamPtr& stream) {
    return seek_next_not_of(stream, WHITESPACE);
}

static std::streampos seek_next_of(_json_impl::IStreamPtr& stream,
                                   const std::string& chars) {
    while(!stream->eof()) {
        auto c = stream->get();
        if(chars.find(c) != std::string::npos) {
            unget(stream);
            assert(stream->good());
            return stream->tellg();
        }
    }

    // FIXME: optional<>
    return stream->tellg();
}

static std::streampos
    find_comma_or_closing_brace(_json_impl::IStreamPtr& stream) {
    bool in_quotes = false;

    int nested_counter = 0;
    while(!stream->eof()) {
        char c = stream->get();
        if(c == '"') {
            in_quotes = !in_quotes;
        } else if(c == '[' || c == '{') {
            if(!in_quotes) {
                nested_counter++;
            }
        } else if(c == ']') {
            if(!in_quotes) {
                nested_counter--;
            }
        } else if(c == ',') {
            if(!in_quotes && nested_counter == 0) {
                unget(stream);
                return stream->tellg();
            }
        } else if(c == '}') {
            if(!in_quotes && nested_counter == 0) {
                unget(stream);
                return stream->tellg();
            } else {
                nested_counter--;
            }
        }
    }

    return std::streampos(-1);
}

static std::streampos
    find_comma_or_closing_bracket(_json_impl::IStreamPtr& stream) {
    bool in_quotes = false;

    int nested_counter = 0;
    while(!stream->eof()) {
        char c = stream->get();
        if(c == '"') {
            in_quotes = !in_quotes;
        } else if(c == '[' || c == '{') {
            if(!in_quotes) {
                nested_counter++;
            }
        } else if(c == '}') {
            if(!in_quotes) {
                nested_counter--;
            }
        } else if(c == ',') {
            if(!in_quotes && nested_counter == 0) {
                unget(stream);
                return stream->tellg();
            }
        } else if(c == ']') {
            if(!in_quotes && nested_counter == 0) {
                unget(stream);
                return stream->tellg();
            } else {
                nested_counter--;
            }
        }
    }

    return std::streampos(-1);
}

JSONNodeType JSONNode::type() const {
    return type_;
}

std::size_t JSONNode::size() const {
    if(type_ == JSON_ARRAY) {
        return std::get<ArrayType>(value_).size();
    } else if(type_ == JSON_OBJECT) {
        return std::get<ObjectType>(value_).size();
    } else {
        return 1;
    }
}

bool JSONNode::has_key(const std::string& key) const {
    if(type_ != JSON_OBJECT) {
        return false;
    } else {
        return std::get<ObjectType>(value_).count(key);
    }
}

std::vector<std::string> JSONNode::keys() const {
    if(type_ != JSON_OBJECT) {
        return {};
    } else {
        std::vector<std::string> keys;
        for(auto& p: std::get<ObjectType>(value_)) {
            keys.push_back(p.first);
        }
        return keys;
    }
}

bool JSONNode::is_value_type() const {
    return type_ != JSON_ARRAY && type_ != JSON_OBJECT;
}

optional<int64_t> JSONNode::to_int() const {
    if(type_ != JSON_NUMBER) {
        return optional<int64_t>();
    }

    return int64_t(std::atof(std::get<std::string>(value_).c_str()));
}

bool JSONNode::is_float() const {
    if(type_ == JSON_NUMBER) {
        std::string value = std::get<std::string>(value_);
        return value.find('.') != std::string::npos;
    }

    return false;
}

optional<float> JSONNode::to_float() const {
    if(type_ != JSON_NUMBER) {
        return optional<float>();
    }

    return std::atof(std::get<std::string>(value_).c_str());
}

optional<bool> JSONNode::to_bool() const {
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

JSONIterator JSONNode::to_iterator() const {
    auto it = JSONIterator();
    if(this == JSONIterator::invalid_node.get()) {
        it.current_node_ = JSONIterator::invalid_node;
    } else {
        it.current_node_ = std::make_shared<JSONNode>(*this);
    }

    return it;
}

bool JSONNode::is_null() const {
    return type_ == JSON_NULL;
}

std::shared_ptr<JSONNode> parse_node(_json_impl::IStreamPtr stream);

std::shared_ptr<JSONNode> parse_array(_json_impl::IStreamPtr stream) {
    auto node = std::make_shared<JSONNode>(JSON_ARRAY);

    while(!stream->eof()) {
        auto subnode = parse_node(stream);
        if(subnode) {
            std::get<JSONNode::ArrayType>(node->value_).push_back(subnode);
        }
        auto c = stream->get();
        if(c == ']') {
            break;
        }
    }

    return node;
}

/* Parse an object node and return its size if the parse is successful */
std::shared_ptr<JSONNode> parse_object(_json_impl::IStreamPtr stream) {
    auto node = std::make_shared<JSONNode>(JSON_OBJECT);

    std::size_t count = 0;
    while(!stream->eof()) {
        if(skip_whitespace(stream) == -1) {
            break;
        }

        char c = stream->get();
        if(c == '"') {
            std::string key = read_string(stream);
            ++count;
            skip_whitespace(stream);
            c = stream->get();
            if(c != ':') {
                S_WARN("Expected ':' found '{0}'", c);
                return JSONIterator::invalid_node;
            }

            skip_whitespace(stream);

            auto subnode = parse_node(stream);
            std::get<JSONNode::ObjectType>(node->value_)[key] =
                (subnode) ? subnode : JSONIterator::invalid_node;
        } else if(c == ',') {
            continue;
        } else if(c == '}') {
            return node;
        } else {
            S_ERROR("Unexpected character: {0} when parsing object", c);
            return JSONIterator::invalid_node;
        }
    }

    return std::shared_ptr<JSONNode>();
}

std::shared_ptr<JSONNode> parse_node(_json_impl::IStreamPtr stream) {
    skip_whitespace(stream);

    char c = stream->peek();

    switch(c) {
        case '{': {
            stream->get(); // Consume
            return parse_object(stream);
        } break;
        case '[': {
            stream->get(); // Consume
            return parse_array(stream);
        } break;
        case '}':
        case ']': {
            return std::shared_ptr<JSONNode>();
        } break;
        case '"': {
            stream->get(); // Consume '"'
            auto node = std::make_shared<JSONNode>(JSON_STRING);
            node->value_ = read_string(stream);
            return node;
        }
        case 't':
            stream->get();
            if(check_remainder(stream, "rue")) {
                return std::make_shared<JSONNode>(JSON_TRUE);
            } else {
                return JSONIterator::invalid_node;
            }
        case 'f':
            stream->get();
            if(check_remainder(stream, "alse")) {
                return std::make_shared<JSONNode>(JSON_FALSE);
            } else {
                return JSONIterator::invalid_node;
            }
        case 'n':
            stream->get();
            if(check_remainder(stream, "ull")) {
                return std::make_shared<JSONNode>(JSON_NULL);
            } else {
                return JSONIterator::invalid_node;
            }
        default: {
            // Number
            auto node = std::make_shared<JSONNode>(JSON_NUMBER);
            node->value_ = read_value(stream);
            return node;
        }
    }
}

void JSONIterator::set_invalid(const std::string& message) {
    current_node_ = invalid_node;
    S_ERROR(message);
}

JSONIterator JSONIterator::operator[](const std::size_t i) const {
    if(!is_valid() || current_node_->type() != JSON_ARRAY ||
       i >= current_node_->size()) {
        return JSONIterator();
    }

    return JSONIterator(
        std::get<JSONNode::ArrayType>(current_node_->value_)[i]);
}

JSONIterator JSONIterator::begin() const {
    if(!is_valid()) {
        return JSONIterator();
    }

    if(!current_node_->is_array()) {
        return JSONIterator();
    }

    if(current_node_->size() == 0) {
        return JSONIterator();
    }

    return (*this)[0];
}

JSONIterator& JSONIterator::operator++() {
    if(!is_valid() || !is_array_iterator()) {
        return *this;
    }

    is_array_iterator_ = true;
    return *this;
}

JSONIterator JSONIterator::operator[](const std::string& key) const {
    if(!is_valid() || current_node_->type() != JSON_OBJECT) {
        return JSONIterator();
    }

    return JSONIterator(
        std::get<JSONNode::ObjectType>(current_node_->value_).at(key));
}

JSONIterator json_load(const Path& path) {
    std::shared_ptr<std::istream> filein =
        std::make_shared<std::ifstream>(path.str());
    if(!*filein) {
        return JSONIterator();
    }

    return json_read(filein);
}

JSONIterator json_parse(const std::string& data) {
    auto ss = std::make_shared<std::stringstream>();
    (*ss) << data;

    return parse_node(ss);
}

JSONIterator json_read(std::shared_ptr<std::istream> stream) {
    std::string content{std::istreambuf_iterator<char>(*stream),
                        std::istreambuf_iterator<char>()};

    return json_parse(content);
}

} // namespace smlt
