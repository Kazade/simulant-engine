#ifndef KGLT_MATRIX_STACK_H
#define KGLT_MATRIX_STACK_H

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
