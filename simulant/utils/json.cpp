#include <sstream>
#include <istream>
#include "json.h"
#include "../logging.h"

namespace smlt {

const std::string WHITESPACE = "\t\n\r ";

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

static bool check_remainder(_json_impl::IStreamPtr& stream, const std::string& rest) {
    for(auto& l: rest) {
        if(stream->get() != l) {
            return false;
        }
    }

    return true;
}

static std::streampos seek_next_not_of(_json_impl::IStreamPtr& stream, const std::string& chars) {
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

static std::streampos seek_next_of(_json_impl::IStreamPtr& stream, const std::string& chars) {
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

static std::streampos find_comma_or_closing_brace(_json_impl::IStreamPtr& stream) {
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

static std::streampos find_comma_or_closing_bracket(_json_impl::IStreamPtr& stream) {
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

template<typename Func>
void JSONNode::read_keys(Func&& cb) const {
    if(type_ != JSON_OBJECT) {
        S_WARN("read_keys called on non-object node!");
        return;
    }


    assert(!stream_->bad());
    assert(!stream_->fail());
    assert(!stream_->eof());

    stream_->seekg(start_);

    auto c = stream_->get();

    assert(c == '{');
    if(c != '{') {
        S_WARN("Invalid character {0} expected '{'", c);
        return;
    }

    while(!stream_->eof()) {
        if(skip_whitespace(stream_) == -1) {
            break;
        }

        assert(stream_->good());

        char c = stream_->get();
        assert(stream_->good());

        if(c == '"') {
            std::string key = read_string(stream_);
            if(cb(key)) {
                return;
            }
            skip_whitespace(stream_);
            assert(stream_->good());

            c = stream_->get();
            assert(stream_->good());

            if(c != ':') {
                S_WARN("Expected ':' found '{0}'", c);
                return;
            }
            skip_whitespace(stream_);
            assert(stream_->good());

            auto pos = find_comma_or_closing_brace(stream_);
            if(pos == -1) {
                S_ERROR("Unable to find command or closing }");
                return;
            }

            assert(stream_->good());
            c = stream_->get(); /* Get the comma or } */
            assert(stream_->good());

            if(c == '}') {
                return;
            }
        } else if(c == '}') {
            /* Empty case */
            return;
        } else {
            S_WARN("Unexpected character: {0}", c);
            return;
        }
    }

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

bool JSONNode::has_key(const std::string &key) const {
    bool found = false;
    auto cb = [&key, &found](const std::string& item) -> bool {
        if(key == item) {
            found = true;
            return true;
        }

        return false;
    };

    read_keys(cb);

    return found;
}

std::vector<std::string> JSONNode::keys() const {
    std::vector<std::string> ret;

    auto cb = [&ret](const std::string& item) -> bool {
        ret.push_back(item);
        return false;
    };

    read_keys(cb);

    return ret;
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

JSONIterator JSONNode::to_iterator() const {
    auto it = JSONIterator();
    it.current_node_ = std::make_shared<JSONNode>(*this);
    it.stream_ = stream_;
    return it;
}

bool JSONNode::is_null() const {
    return type_ == JSON_NULL;
}

static optional<std::size_t> parse_array(_json_impl::IStreamPtr stream) {
    std::size_t count = 0;

    skip_whitespace(stream);
    auto p = stream->tellg();

    while(!stream->eof()) {
        skip_whitespace(stream);
        auto p2 = find_comma_or_closing_bracket(stream);
        auto c = stream->get();

        if(c == ',') {
            ++count;
        } else {
            assert(c == ']');

            /* If we found the closing ] just after the opening one, then
             * this is an empty array */
            bool empty = (p2 - p) == 0;

            /* We just do this final check to make sure we didn't double
             * count things... */
            auto end = stream->tellg();
            do {
                unget(stream);
                unget(stream);
                c = stream->get(); // Moves forward one, so we move back twice each loop
            } while(WHITESPACE.find(c) != std::string::npos);

            stream->seekg(end, std::ios::beg);

            if(c == ',') {
                S_WARN("Found trailing comma in JSON array");
                return count;
            } else {
                if(!empty) {
                    ++count;
                }

                return count;
            }
        }
    }

    return optional<std::size_t>();
}

/* Parse an object node and return its size if the parse is successful */
static optional<std::size_t> parse_object(_json_impl::IStreamPtr stream) {
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
                return optional<std::size_t>();
            }

            skip_whitespace(stream);

            auto pos = find_comma_or_closing_brace(stream);

            if(pos == -1) {
                S_ERROR("Couldn't find comma or closing brace");
                return optional<std::size_t>();
            } else {
                c = stream->get();
                assert(c == ',' || c == '}');

                if(c == '}') {
                    return optional<std::size_t>(count);
                } else {
                    stream->ignore();
                }
            }
        } else if(c == '}') {
            stream->unget();
            return optional<std::size_t>(count);
        } else {
            S_ERROR("Unexpected character: {0} when parsing object", c);
            return optional<std::size_t>();
        }
    }

    return optional<std::size_t>(count);
}

void JSONIterator::parse_node(JSONNode& node, _json_impl::IStreamPtr stream, std::streampos pos) {
    stream->seekg(pos);

    skip_whitespace(stream);

    auto start = stream_->tellg();
    char c = stream->get();

    std::streampos end;

    switch(c) {
        case '{': {
            node.type_ = JSON_OBJECT;
            auto v = parse_object(stream);
            assert(v);
            if(v) {
                node.size_ = v.value_or(0);
                end = stream_->tellg();
            }
        } break;
        case '[': {
            node.type_ = JSON_ARRAY;
            auto v = parse_array(stream);
            assert(v);
            if(v) {
                node.size_ = v.value_or(0);
                end = stream_->tellg();
            }
        } break;
        case '"':
            node.type_ = JSON_STRING;
            read_string(stream);
            end = int(stream->tellg()) - 2;
            start = int(start) + 1;
        break;
        case 't':
            if(check_remainder(stream, "rue")) {
                node.type_ = JSON_TRUE;
            } else {
                set_invalid("Error at pos: " + smlt::to_string((int) pos));
            }
        break;
        case 'f':
            if(check_remainder(stream, "alse")) {
                node.type_ = JSON_FALSE;
            } else {
                set_invalid("Error at pos: " + smlt::to_string((int) pos));
            }
        break;
        case 'n':
            if(check_remainder(stream, "ull")) {
                node.type_ = JSON_NULL;
            } else {
                set_invalid("Error at pos: " + smlt::to_string((int) pos));
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

    char c = stream_->get(); // Skip the opening '['
    assert(c == '[');
    assert(stream_->good());

    if(c != '[') {
        return JSONIterator();
    }

    std::size_t entry = 0;
    int nest_counter = 0;
    while(stream_->tellg() != current_node_->end()) {
        assert(stream_->good());
        skip_whitespace(stream_);
        assert(stream_->good());

        auto start = stream_->tellg();

        /* Find the end of this element, skipping commas
         * inside arrays or objects */

        nest_counter = 0;
        bool done = false;
        while(stream_->tellg() != current_node_->end()) {
            assert(stream_->good());
            c = stream_->get();
            assert(stream_->good());

            if(c == '[' || c == '{') {
                nest_counter++;
            } else if(c == ']' || c == '}') {
                nest_counter--;
            }

            /* We hit the closing ']' so break */
            if(nest_counter < 0) {
                done = true;
                unget(stream_);
                break;
            }

            if(!nest_counter) {
                if(c == ',') {
                    unget(stream_);
                    break;
                }
            }
        }

        if(entry == i) {
            return JSONIterator(stream_, start, /*is_array_item=*/ true);
        } else {
            if(done) {
                /* End of the array */
                break;
            }
            char skip = stream_->get();
            assert(skip == ',');
            _S_UNUSED(skip);

            ++entry;
        }
    }

    return JSONIterator();
}

JSONIterator JSONIterator::begin() const {
    if(!current_node_) {
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

JSONIterator &JSONIterator::operator++() {
    if(!current_node_ || !is_array_iterator()) {
        return *this;
    }

    /* Move to the end of the node */
    stream_->seekg(current_node_->end());

    /* Find the comma following the item end */
    auto pos = find_comma_or_closing_bracket(stream_);
    if(pos == -1) {
        S_ERROR("Unable to find comma or closing bracket");
        return *this;
    }
    auto c = stream_->get();
    assert(c == ',' || c == ']');

    if(c == ']') {
        /* Hit the end of the array, this iterator is
         * done with */
        current_node_.reset();
        return *this;
    }

    skip_whitespace(stream_);

    /* Now set this to the next element in the array */
    (*this) = JSONIterator(stream_, int(stream_->tellg()));
    is_array_iterator_ = true;
    return *this;
}

JSONIterator JSONIterator::operator[](const std::string& key) const {
    if(!is_valid() || current_node_->type() != JSON_OBJECT) {
        return JSONIterator();
    }

    bool found = false;
    auto cb = [&found, &key](const std::string& iterkey) -> bool {
        if(iterkey == key) {
            found = true;
            return true;
        }

        return false;
    };

    current_node_->read_keys(cb);

    assert(found);

    if(found) {
        /* If we found the key, then the stream will be positioned
         * just after the string, before the colon. So we just need to skip
         * past that */
        skip_whitespace(stream_);
        char colon = stream_->get();
        if(colon != ':') {
            S_WARN("Unexpected character '{0}'. Expected ':'", colon);
            return JSONIterator();
        }

        return JSONIterator(stream_, skip_whitespace(stream_));
    }

    return JSONIterator();
}

JSONIterator json_load(const Path& path) {
    std::shared_ptr<std::istream> filein = std::make_shared<std::ifstream>(path.str());
    if(!*filein) {
        return JSONIterator();
    }

    return json_read(filein);
}

JSONIterator json_parse(const std::string& data) {
    auto ss = std::make_shared<std::stringstream>();
    (*ss) << data;
    return JSONIterator(ss, 0);
}

JSONIterator json_read(std::shared_ptr<std::istream> stream) {
    return JSONIterator(stream, 0);
}



}
