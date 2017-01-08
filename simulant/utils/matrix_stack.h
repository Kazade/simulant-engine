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

#ifndef SIMULANT_MATRIX_STACK_H
#define SIMULANT_MATRIX_STACK_H

#include <cassert>
#include <stack>
#include "kazmath/mat4.h"

class MatrixStack {
public:
    MatrixStack() {
        push(); //Push the idactor matrix
    }
    
    void push() {
        kmMat4 new_matrix;
        //Copy the state of the top of the stack to the new matrix
        if(!stack_.empty()) {
            kmMat4Assign(&new_matrix, &top());
        } else {
            kmMat4Idactor(&new_matrix);
        }
        stack_.push(new_matrix);
    }
    
    void pop() {
        stack_.pop();
    }
    
    kmMat4& top() {
        assert(!stack_.empty());
        return stack_.top();
    }

private:
    std::stack<kmMat4> stack_;
};

#endif
