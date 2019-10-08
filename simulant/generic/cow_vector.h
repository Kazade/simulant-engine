#pragma once

#include <vector>
#include <memory>

namespace smlt {

/* This has nothing to do with Cows.
 *
 * This is a copy-on-write vector. This allows
 * sharing of a vectors data across multiple instance
 * until the data is manipulated, at which point the
 * vector is copied.
 *
 * This is used for texture data when using transactions
 * to avoid large, unnecessary copies
 */

template<typename T>
class cow_vector {
public:
    cow_vector():
        vector_(std::make_shared<std::vector<T>>()),
        unique_(true) {

    }

    cow_vector(const std::vector<T>& rhs):
        vector_(std::make_shared<std::vector<T>>(rhs.begin(), rhs.end())),
        unique_(true) {

    }

    cow_vector(const cow_vector& rhs):
        vector_(rhs.vector_),
        unique_(false) {

        rhs.unique_ = false;
    }

    cow_vector& operator=(const cow_vector& rhs) {
        vector_ = rhs.vector_;
        unique_ = false;
        rhs.unique_ = false;

        return *this;
    }

    const T& operator[](std::size_t idx) const {
        return (*vector_)[idx];
    }

    T& operator[](std::size_t idx) {
        copy();
        return (*vector_)[idx];
    }

    const T& at(std::size_t idx) const {
        return vector_->at(idx);
    }

    T& at(std::size_t idx) {
        copy();
        return vector_->at(idx);
    }

    std::size_t size() const {
        return vector_->size();
    }

    void clear() {
        vector_  = std::make_shared<std::vector<T>>();
        unique_ = true;
    }

    void push_back(const T& val) {
        copy();
        vector_->push_back(val);
    }

    T* data() {
        copy();
        return vector_->data();
    }

    const T& data() const {
        return vector_->data();
    }

    void shrink_to_fit() {
        copy();
        vector_->shrink_to_fit();
    }

    bool empty() const {
        return vector_->empty();
    }

    void reserve(std::size_t size) {
        copy();
        vector_->reserve(size);
    }

    void resize(std::size_t size) {
        copy();
        vector_->resize(size);
    }

    bool unique() const {
        return unique_;
    }

private:
    void copy() {
        if(unique_) return;

        vector_ = std::make_shared<std::vector<T>>(*vector_);
        unique_ = true;
    }

    std::shared_ptr<std::vector<T>> vector_;
    mutable bool unique_ = true;
};

}
