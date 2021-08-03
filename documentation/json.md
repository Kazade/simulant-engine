# JSON Parsing

Simulant comes with a specialized JSON parser built-in. Simulant's JSON parser
is designed to minimize memory usage over performance, and does so by working
with the input file stream directly. This allows you to read and navigate
large JSON files on memory constrained platforms like the Dreamcast.

To use the JSON functionality you should `#include <simulant/utils/json.h>`

## JSON Functions

The following parsing functions are available, each designed for a different input:

 - `JSONIterator json_load(const Path& path);`
 - `JSONIterator json_parse(const std::string& data);`
 - `JSONIterator json_read(std::shared_ptr<std::istream>& stream);`
 
It is recommended that you use `json_read` whereever possible, and pass in the
stream returned by calling `vfs->open_file(...)` which will allow you to maximise
memory efficiency and portability. 

## Navigating a JSON Document

Once you have a `JSONIterator` instance returned from one of the parsing functions, 
you can use it to navigate the file. Assuming the following JSON document:

```
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
```

You can access the various values using the `[]` operator which returns
another `JSONIterator` instance.

```
auto json = json_read(...);

auto array = json["array"];
array->size();  // 4
array->is_array();  // true
array->type();  // JSON_ARRAY

auto value0 = array[0];
value0->is_number();   // true
value0->to_int().value();  // (int64_t) 1
value0->to_float().value();  // (float) 1.0f
value0->to_str();  // "1"

auto obj = json["object"];

obj["five"]->is_null();  // true
auto i = obj["five"]->to_int();
i.has_value();  // false, null is not convertible to int
```

>>> *Caution! Each JSONIterator instance will retain an reference-count on the underlying stream! Make sure you don't persist JSONIterators longer than necessary.


