#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <iterator>

template<typename T, typename Predicate>
std::vector<T> filter(const std::vector<T>& input, Predicate predicate) {
    std::vector<T> result;
    std::copy_if(input.begin(), input.end(), std::back_inserter(result), predicate);
    return result;
}

template<typename Input, typename Output>
Output map(const Input& input, std::function<typename Output::value_type (const typename Input::value_type&)> func) {
    Output ret;
    for(auto in: input) {
        ret.push_back(func(in));
    }
    return ret;
}
