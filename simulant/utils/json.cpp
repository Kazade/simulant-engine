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
};

static std::streampos seek_closing(
        _json_impl::IStreamPtr stream,
        const char opening,
        const char closing,
        int* comma_count=NULL) {

    std::size_t counter = 1;
    int ccount = 0;
    while(stream->good()) {
        auto c = stream->get();

        if(c == ',') {
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
};

static std::streampos seek_next_of(_json_impl::IStreamPtr stream, const std::string& chars) {
    while(stream->good()) {
        auto c = stream->get();
        if(chars.find(c) != std::string::npos) {
            return stream->tellg();
        }
    }

    // FIXME: optional<>
    return stream->tellg();
};

static std::streampos seek_next_not_of(_json_impl::IStreamPtr stream, const std::string& chars) {
    while(stream->good()) {
        auto c = stream->get();
        if(chars.find(c) == std::string::npos) {
            return stream->tellg();
        }
    }

    // FIXME optional<>
    return stream->tellg();
};

std::string JSONNode::read_value_from_stream() const {
    auto len = end_ - start_;
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

void JSONIterator::parse_node(JSONNode& node, _json_impl::IStreamPtr stream, std::streampos pos) {
    stream->seekg(pos);
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
            end = seek_closing(stream, '[', ']');
        break;
        case '"':
            node.type_ = JSON_STRING;
            end = seek_closing(stream, '"', '"');
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
            end = seek_next_of(stream, ",\n\t ");
            end = int(end) - 1;
    }
}

void JSONIterator::set_invalid(const std::string& message) {
    current_node_.reset();
    S_ERROR(message);
}

JSONIterator JSONIterator::operator[](const std::size_t i) {
    if(!is_valid() || current_node_->type() != JSON_ARRAY) {
        return JSONIterator();
    }

    stream_->seekg(current_node_->start());

    std::size_t entry = 0;
    while(stream_->tellg() != current_node_->end()) {
        auto start = seek_next_not_of(stream_, " \t\n\r");

        /* Look for commas, or the end of the array */
        auto end = seek_next_of(stream_, ",]");

        if(entry == i) {
            return JSONIterator(stream_, start);
        } else {
            ++entry;
        }
    }

    return JSONIterator();
}

JSONIterator JSONIterator::operator[](const std::string& key) {
    if(!is_valid() || current_node_->type() != JSON_OBJECT) {
        return JSONIterator();
    }

    /* Search for the key */
    stream_->seekg(current_node_->start());

    while(stream_->tellg() != current_node_->end()) {
        /* Find the opening quote */
        auto start = seek_next_not_of(stream_, "\t\n ");
        auto c = stream_->get();

        if(c != '"') {
            auto it = JSONIterator();
            it.set_invalid("Unexpected character: " + std::string(c, 1));
            return it;
        }

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
            auto new_start = seek_next_not_of(stream_, "\t\n \r");
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



}
