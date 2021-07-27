#include <sstream>
#include <istream>
#include "json.h"
#include "../logging.h"

namespace smlt {

bool check_remainder(_json_impl::IStreamPtr stream, const std::string& rest) {
    for(auto& l: rest) {
        if(stream->get() != l) {
            return false;
        }
    }

    return true;
}

static std::streampos seek_closing(
        _json_impl::IStreamPtr stream,
        const char opening,
        const char closing,
        int* comma_count=NULL) {

    std::size_t counter = 1;
    int ccount = 0;

    /* Increments each time we hit a [ or {, decrements
     * each time we hit a ] or }. If > 0, then we're in a nested
     * block */
    int nest_counter = 0;
    bool in_quotes = false;

    while(stream->good()) {
        char c = stream->get();

        if(c == '"' && closing != '"') {
            in_quotes = !in_quotes;
        }

        /* We don't match stuff inside quotemarks */
        if(in_quotes) {
            continue;
        }

        /* Track nested blocks */
        if(c == '{' || c == '[') {
            ++nest_counter;
        } else if(c == '}' || c == ']') {
            --nest_counter;
        }

        /* Count commas if we're not nested */
        if(!nest_counter && c == ',') {
            ccount++;
        }

        if(c == closing) {
            --counter;
            if(counter == 0) {
                break;
            }
        } else if(c == opening) {
            ++counter;
        }
    }

    if(comma_count) {
        *comma_count = ccount;
    }

    // FIXME: optional<>
    return stream->tellg();
}

static std::streampos seek_next_of(_json_impl::IStreamPtr stream, const std::string& chars) {
    while(stream->good()) {
        auto c = stream->get();
        if(chars.find(c) != std::string::npos) {
            stream->putback(c);
            return stream->tellg();
        }
    }

    // FIXME: optional<>
    return stream->tellg();
}

static std::streampos seek_next_not_of(_json_impl::IStreamPtr stream, const std::string& chars) {
    while(stream->good()) {
        char c = stream->get();
        if(chars.find(c) == std::string::npos) {
            stream->putback(c);
            return stream->tellg();
        }
    }

    // FIXME optional<>
    return stream->tellg();
}

static std::streampos skip_whitespace(_json_impl::IStreamPtr stream) {
    const std::string& WHITESPACE = "\t\n\r ";
    return seek_next_not_of(stream, WHITESPACE);
}

std::string JSONNode::read_value_from_stream() const {
    /* Start and end are inclusive. So if the value was a
     * single character then end-start would == 0.
     * So we need to add 1 to the length. */
    auto len = (end_ - start_) + 1;
    stream_->seekg(start_);
    char buffer[len + 1];
    stream_->read(buffer, len);
    buffer[len] = '\0';
    return std::string(buffer);
}

JSONNodeType JSONNode::type() const {
    return type_;
}

std::size_t JSONNode::size() const {
    return size_;
}

bool JSONNode::is_value_type() const {
    return type_ != JSON_ARRAY && type_ != JSON_OBJECT;
}

optional<int64_t> JSONNode::to_int() const {
    if(type_ != JSON_NUMBER) {
        return optional<int64_t>();
    }

    std::string value = read_value_from_stream();
    int64_t ret;
    std::stringstream ss;

    ss << value;
    ss >> ret;

    return optional<int64_t>(ret);
}

optional<float> JSONNode::to_float() const {
    if(type_ != JSON_NUMBER) {
        return optional<float>();
    }

    std::string value = read_value_from_stream();
    float ret;
    std::stringstream ss;

    ss << value;
    ss >> ret;

    return optional<float>(ret);
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

bool JSONNode::is_null() const {
    return type_ == JSON_NULL;
}

void JSONIterator::parse_node(JSONNode& node, _json_impl::IStreamPtr stream, std::streampos pos) {
    stream->seekg(pos);

    skip_whitespace(stream);

    auto start = stream_->tellg();
    char c = stream->get();

    std::streampos end;
    int size = 0;

    switch(c) {
    case '{': {
        node.type_ = JSON_OBJECT;
            end = seek_closing(stream, '{', '}', &size);
            ++size;
            node.size_ = size;
        } break;
        case '[':
            node.type_ = JSON_ARRAY;
            end = seek_closing(stream, '[', ']', &size);
            ++size;
            node.size_ = size;
        break;
        case '"':
            node.type_ = JSON_STRING;
            end = seek_closing(stream, '"', '"');
            start = int(start) + 1;
            end = int(end) - 2;
        break;
        case 't':
            if(check_remainder(stream, "rue")) {
                node.type_ = JSON_TRUE;
            } else {
                set_invalid("Error at pos: " + std::to_string(pos));
            }
        break;
        case 'f':
            if(check_remainder(stream, "alse")) {
                node.type_ = JSON_FALSE;
            } else {
                set_invalid("Error at pos: " + std::to_string(pos));
            }
        break;
        case 'n':
            if(check_remainder(stream, "ull")) {
                node.type_ = JSON_NULL;
            } else {
                set_invalid("Error at pos: " + std::to_string(pos));
            }
        break;
        default:
            // Number
            node.type_ = JSON_NUMBER;
            end = seek_next_of(stream, ",\r\n\t ]}");
            end = int(end) - 1;
    }

    node.start_ = start;
    node.end_ = end;
    node.stream_ = stream;
}

void JSONIterator::set_invalid(const std::string& message) {
    current_node_.reset();
    S_ERROR(message);
}

JSONIterator JSONIterator::operator[](const std::size_t i) const {
    if(!is_valid() || current_node_->type() != JSON_ARRAY) {
        return JSONIterator();
    }

    stream_->seekg(current_node_->start());
    stream_->get(); // Skip the opening '['

    std::size_t entry = 0;
    int nest_counter = 0;
    while(stream_->tellg() != current_node_->end()) {
        skip_whitespace(stream_);

        auto start = stream_->tellg();

        /* Find the end of this element, skipping commas
         * inside arrays or objects */

        nest_counter = 0;
        bool done = false;
        while(stream_->tellg() != current_node_->end()) {
            char c = stream_->get();
            if(c == '[' || c == '{') {
                nest_counter++;
            } else if(c == ']' || c == '}') {
                nest_counter--;
            }

            /* We hit the closing ']' so break */
            if(nest_counter < 0) {
                done = true;
                stream_->putback(c);
                break;
            }

            if(!nest_counter) {
                if(c == ',') {
                    stream_->putback(c);
                    break;
                }
            }
        }

        if(entry == i) {
            return JSONIterator(stream_, start);
        } else {
            if(done) {
                /* End of the array */
                break;
            }
            char skip = stream_->get();
            assert(skip == ',');
            ++entry;
        }
    }

    return JSONIterator();
}

bool JSONIterator::has_key(const std::string &key) const {
    return (*this)[key].is_valid();
}

JSONIterator JSONIterator::operator[](const std::string& key) const {
    if(!is_valid() || current_node_->type() != JSON_OBJECT) {
        return JSONIterator();
    }

    /* Search for the key */
    stream_->seekg(current_node_->start(), std::ios::beg);
    stream_->get(); // Skip the opening '{'

    while(stream_->tellg() != current_node_->end()) {
        /* Find the opening quote */
        skip_whitespace(stream_);
        char c = stream_->get();

        if(c != '"') {
            auto it = JSONIterator();
            it.set_invalid("Unexpected character: " + std::string(c, 1));
            return it;
        }

        /* Letter after the quote */
        auto start = stream_->tellg();

        /* Find the closing quote */
        auto end = seek_next_of(stream_, "\"");
        auto len = end - start;
        char buffer[len + 1];

        stream_->seekg(start);
        stream_->read(buffer, len);
        buffer[len] = '\0';

        if(key == buffer) {
            /* Point the node at the value */
            seek_next_of(stream_, ":");
            stream_->ignore(); /* Discard the colon */

            skip_whitespace(stream_);
            auto new_start = stream_->tellg();
            auto it = JSONIterator(stream_, new_start);
            return it;
        }
    }

    return JSONIterator();
}

JSONIterator json_parse(const std::string& data) {
    auto ss = std::make_shared<std::stringstream>();
    (*ss) << data;
    return JSONIterator(ss, 0);
}

JSONIterator json_read(std::shared_ptr<std::istream> &stream) {
    return JSONIterator(stream, 0);
}



}
