#pragma once

#include <cassert>
#include <iterator>

namespace threadsafe {

template<typename T> class ring_buffer;

namespace impl {

template<typename T>
struct Node {
    Node(bool is_root=false):
        next(this),
        prev(this),
        is_root_(is_root) {
    }

    T val;

    Node* next;
    Node* prev;

    bool is_root() const { return is_root_; }

    void inc_ref() const {
        if(is_root()) return;

        assert(ref_count > -1);
        ++ref_count;
    }

    void dec_ref() const {
        if(is_root()) return;

        --ref_count;
        assert(ref_count > -1);
        if(ref_count == 0) {
            delete this;
        }
    }

private:
    bool is_root_ = false;
    mutable int ref_count = 0;

    Node(const Node& rhs) = delete;
    void operator=(const Node& rhs) = delete;
};

template<typename T, bool is_const=false>
class Iterator : public std::iterator<std::bidirectional_iterator_tag, T> {
public:
    typedef typename std::conditional<is_const, const Node<T>*, Node<T>*>::type node_pointer_type;
    typedef typename std::conditional<is_const, const T&, T&>::type value_reference_type;

private:
    friend class ring_buffer<T>;

    node_pointer_type node_ = nullptr;

public:
    Iterator(const Iterator& rhs) {
        rhs.node_->inc_ref();
        node_ = rhs.node_;
    }

    Iterator& operator=(const Iterator& rhs) {
        if(this == &rhs) {
            return *this;
        }

        if(node_) {
            node_->dec_ref();
        }

        rhs.node_->inc_ref();
        node_ = rhs.node_;
        return *this;
    }

    Iterator(node_pointer_type node) {
        node->inc_ref();
        node_ = node;
    }

    ~Iterator() {
        if(node_) {
            node_->dec_ref();
        }
    }

    value_reference_type operator*() const {
        return node_->val;
    }

    T* operator->() const {
        return &node_->val;
    }

    Iterator& operator++() {
        node_pointer_type old = node_;
        node_ = old->next;
        node_->inc_ref();
        old->dec_ref();

        return *this;
    }

    Iterator operator++(int i) {
        node_pointer_type old = node_;
        node_ = old->next;
        node_->inc_ref();
        old->dec_ref();

        return *this;
    }

    bool operator==(const Iterator& rhs) const {
        return node_ == rhs.node_;
    }
    bool operator!=(const Iterator& rhs) const {
        return !(*this == rhs);
    }
};

} //End impl namespace

template<typename T>
class ring_buffer {
public:
    typedef T value_type;
    typedef impl::Iterator<T> iterator;
    typedef impl::Iterator<T, true> const_iterator;

    ring_buffer():
        root_(true) {}

    ~ring_buffer() {
        erase(begin(), end());
    }

    //  [ R, X1, X2 ]

    void push_back(const value_type& val) {
        impl::Node<T>* new_node = new impl::Node<T>();
        new_node->val = val;
        new_node->prev = root_.prev;
        new_node->next = &root_;
        new_node->inc_ref(); //Always hold a reference for the buffer

        root_.prev->next = new_node; //Forward iterators now find this node
        root_.prev = new_node; //Reverse iterators now find this node
    }

    void push_front(const value_type& val) {
        impl::Node<T>* new_node = new impl::Node<T>();
        new_node->val = val;
        new_node->next = root_.next;
        new_node->prev = &root_;
        new_node->inc_ref();

        root_.next->prev = new_node; //Reverse iterators now find the new node
        root_.next = new_node; //Forward iterators now find the new node
    }

    void pop_back() {
        assert(!empty());
        impl::Node<T>* to_delete = root_.prev;
        to_delete->prev->next = &root_; //Forward iterators now skip this node
        root_.prev = to_delete->prev; //Reverse iterators now skip this node
        to_delete->dec_ref(); //Remove the hold we have on it
    }

    void pop_front() {
        assert(!empty());
        impl::Node<T>* to_delete = root_.next;
        root_.next = to_delete->next; //Forward iterators now skip this node
        to_delete->next->prev = &root_; //Reverse iterators now skip this node
        to_delete->dec_ref();  //Remove the hold we have on it
    }

    T front() const { return root_.next->val; }
    T back() const { return root_.prev->val; }

    iterator begin() { return iterator(root_.next); }
    iterator end() { return iterator(&root_); }

    const_iterator begin() const { return const_iterator(root_.next); }
    const_iterator end() const { return const_iterator(&root_); }

    bool empty() const { return root_.next == &root_; }
    size_t size() const {
        size_t i = 0;
        for(auto it = begin(); it != end(); ++it) {
            ++i;
        }
        return i;
    }

    iterator erase(iterator it) {
        assert(!empty());
        impl::Node<T>* to_delete = it.node_;
        to_delete->prev->next = to_delete->next;
        to_delete->next->prev = to_delete->prev;

        iterator ret(to_delete->next);
        to_delete->dec_ref();
        return ret;
    }

    iterator erase(iterator start, iterator end) {
        for(; start != end;) {
            start = erase(start);
        }
        return start;
    }

private:
    /*
     *  Root stays constant through the lifetime of the ring_buffer. It acts as a dummy node that
     *  represents the "end" iterator. Iterators start at root->next or root->prev and end when they hit
     *  root.
     */
    impl::Node<T> root_;
};

}

