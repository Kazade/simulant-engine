// #pragma once

/* This is a consistent result type for functions that
 * may error. It's inspired by std::optional<T> and Rust's
 * error handling.
 *
 * Usage:
 *
 * class MyError : public Error {};
 *
 * Result<int> my_func_that_might_error() {
 *  if(bad_thing) {
 *      return make_error<MyError>("Something bad happened");
 *  } else {
 *      return 1;
 *  }
 * }
 *
 * ...
 *
 * auto result = my_func_that_might_error();
 * if(!result) {
 *    result.handle<MyError>([](MyError err) { ... code ... });
 *    result.handle<Error>([](Error err) { ... code ... });
 * }
 *
 * auto value = result.value_or(0);
 *
 */

// namespace smlt {

// class Error {};

// template<typename T>
// class Result {
// public:
//     bool ok() const;

//     T* value();
//     const T* value() const;

//     Error err() const;

//     operator bool() const {
//         return ok();
//     }

//     template<typename U>
//     void handle(const std::function<void (U)>&) {}

//     void handle(const std::function<void (T)>& handler) {
//         handler((T) err());
//     }

// };


// }
