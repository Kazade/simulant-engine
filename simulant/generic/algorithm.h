/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

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
