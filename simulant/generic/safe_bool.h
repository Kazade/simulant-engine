/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SAFE_BOOL_H
#define SAFE_BOOL_H

//http://www.artima.com/cppsource/safebool3.html

class safe_bool_base {
protected:
    typedef void (safe_bool_base::*bool_type)() const;
    void this_type_does_not_support_comparisons() const {}

    safe_bool_base() {}
    safe_bool_base(const safe_bool_base&) {}
    safe_bool_base& operator=(const safe_bool_base&) {return *this;}
    virtual ~safe_bool_base() {}
};

template <typename T=void>
class safe_bool :
    public safe_bool_base {

public:
    operator bool_type() const {
    return (static_cast<const T*>(this))->boolean_test()
      ? &safe_bool<T>::this_type_does_not_support_comparisons : 0;
    }

protected:
    ~safe_bool() {}
};

template<>
class safe_bool<void> :
    public safe_bool_base {
public:
    operator bool_type() const {
        return boolean_test()==true ?
            &safe_bool<void>::this_type_does_not_support_comparisons : 0;
    }
protected:
    virtual bool boolean_test() const=0;
    virtual ~safe_bool() {}
};

template <typename T, typename U>
void operator==(const safe_bool<T>& lhs,const safe_bool<U>& rhs) {
    lhs.this_type_does_not_support_comparisons();
}

template <typename T,typename U>
void operator!=(const safe_bool<T>& lhs,const safe_bool<U>& rhs) {
    lhs.this_type_does_not_support_comparisons();
}

#endif // SAFE_BOOL_H
