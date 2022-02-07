#pragma once

#include <simulant/test.h>
#include <simulant/utils/json.h>

namespace {

using namespace smlt;

class JSONTests : public test::TestCase {
public:

    void test_basic_usage() {
        std::string data = R"(
{
    "array": [1, 2, 3, 4],
    "object": {
        "one": 1,
        "two": 2.0,
        "three": true,
        "four": false,
        "five": null
    }
}
)";

        auto it = json_parse(data);

        assert_equal(it->type(), JSON_OBJECT);
        assert_equal(it->size(), 2u);

        auto array = it["array"];

        assert_equal(array->type(), JSON_ARRAY);
        assert_equal(array->size(), 4u);

        auto value = array[0];
        assert_equal(value->type(), JSON_NUMBER);
        assert_equal(value->to_int().value_or(0), 1);

        value = array[1];
        assert_equal(value->type(), JSON_NUMBER);
        assert_equal(value->to_int().value_or(0), 2);

        assert_false(array[5].is_valid());

        auto obj = it["object"];

        assert_equal(obj->type(), JSON_OBJECT);
        assert_equal(obj->size(), 5u);

        auto one = obj["one"];

        assert_equal(one->type(), JSON_NUMBER);
        assert_true(one->is_value_type());
        assert_equal(one->to_int().value_or(0), 1);
        assert_equal(one->to_float().value_or(0.0f), 1.0f);
        assert_equal(one->to_str().value(), "1");

        assert_equal(obj["five"]->type(), JSON_NULL);
    }

    void test_nested_accesses() {
        std::string data = R"(
{
    "object": {
        "array": [1, 2, 3]
    }
}
)";

        auto json = json_parse(data);
        assert_equal(
            json["object"]["array"][0]->to_int().value_or(0),
            1
        );
    }

    void test_nested_arrays() {
        std::string data = R"(
{
    "array": [[1, 2, 3], "cheese"]
}
)";

        auto json = json_parse(data);

        assert_equal(
            json["array"][0][0]->to_int().value_or(0),
            1
        );

        assert_equal(
            json["array"][0][1]->to_int().value_or(0),
            2
        );

        assert_equal(
            json["array"][1]->to_str().value(),
            "cheese"
        );
    }

    void test_nested_objects() {
        std::string data = R"(
{
    "object": {
        "a": {
            "b": 1,
            "c": "AAA",
            "d": {
                "e": 2
            }
        }
    }
}
)";

        auto json = json_parse(data);

        assert_equal(json["object"]["a"]["b"]->to_int().value(), 1);
        assert_equal(json["object"]["a"]["d"]["e"]->to_int().value(), 2);
    }

    void test_mixed_arrays_objects() {
        std::string data = R"(
{
    "object": [{
        "b": 1,
        "c": ["AAA", {"f": 3}],
        "d": {
            "e": 2
        }
    }]
}
)";

        auto json = json_parse(data);

        assert_equal(json["object"][0]["c"][1]["f"]->to_int().value(), 3);
    }

    void test_stuff_in_strings() {
        std::string data = R"(
{
    "{my_thing}": {
        ",[]": 1
    }
}
)";

        auto json = json_parse(data);
        assert_equal(json["{my_thing}"][",[]"]->to_int().value(), 1);
    }

    void test_object_keys() {
        std::string data = R"(
{
    "object": [{
        "b": 1,
        "c": ["AAA", {"f": 3}],
        "d": {
            "e": 2
        }
    }]
}
)";

        auto json = json_parse(data);

        auto root = json->keys();

        assert_true(root == std::vector<std::string>{"object"});
        auto keys = json["object"][0]->keys();
        assert_true(keys == std::vector<std::string>({"b", "c", "d"}));
    }

    void test_simple_material() {
        const std::string data = R"(
     {
         "property_values": {
             "s_lighting_enabled": true,
             "s_textures_enabled": 1,
             "s_blend_func": "alpha"
         },
         "passes": [{}]
     }
)";

        auto json = json_parse(data);
        assert_true(json->has_key("passes"));
        assert_true(json->has_key("property_values"));
        assert_true(json["property_values"]->is_object());
        assert_equal(json["property_values"]->keys().size(), 3u);
        assert_equal(json["passes"]->size(), 1u);
        assert_equal(json["passes"][0]->size(), 0u);
    }
    void test_complex_material() {
        const std::string data = R"(
{
    "custom_properties": [
        {
            "name": "texture_map",
            "type": "texture",
            "default": null
        },
        {
            "name": "enable_texturing",
            "type": "bool",
            "default": true
        },
    ],

    "property_values": {
        "texture_map": "simulant-icon.png",
        "s_material_diffuse": "1 0 1 0",
        "s_material_ambient": "1 1 1 1"
    },

    "passes": [
        {
            "iteration": "once"
        }
    ]
}
        )";

        auto json = json_parse(data);
        assert_equal(json["custom_properties"]->size(), 2u);
        assert_equal(json["passes"]->size(), 1u);
    }

    void test_texture_only_material() {
        const std::string data = R"(
{
    "passes": [{
        "property_values": {
            "s_lighting_enabled": false,
            "s_textures_enabled": 1
        }
    }]
}

)";

        auto json = json_parse(data);

        assert_true(json.is_valid());

        assert_true(json->has_key("passes"));
        assert_equal(json["passes"]->size(), 1u);

        assert_true(json["passes"][0]->has_key("property_values"));
        assert_equal(json["passes"][0]["property_values"]->size(), 2u);
    }

    void test_array_range_iteration() {
        const std::string data = R"(
{
    "array": [1, 2, {"three":3}]
}

)";
        auto json = json_parse(data);
        auto it = json["array"][0];

        assert_true(it.is_array_iterator());

        ++it;

        assert_equal(it->to_int().value_or(0), 2);

        ++it;

        assert_true(it->is_object());

        ++it;

        assert_false(it);

        std::size_t counter = 0;
        for(auto _: json["array"]) {
            counter++;
        }

        assert_equal(counter, json["array"]->size());
    }

    void test_empty_array_value() {
        const std::string data = R"({"sessions": []})";
        auto json = json_parse(data);

        assert_equal(json["sessions"]->size(), 0);
    }
};

}
