#pragma once

#include <functional>
#include <memory>
#include <algorithm>

#include "../logging.h"
#include "../threads/atomic.h"

#define DEFINE_SIGNAL(prototype, name) \
    public: \
        prototype& name() { return name##_; } \
        prototype& name() const { return name##_; } \
    private: \
        mutable prototype name##_

namespace smlt {
namespace sig {

class ConnectionImpl;

class Disconnector {
public:
    virtual ~Disconnector() {}

    virtual bool disconnect(const ConnectionImpl& conn) = 0;
    virtual bool connection_exists(const ConnectionImpl& conn) const = 0;
};

class Connection;

class ConnectionImpl {
public:
    ConnectionImpl(Disconnector* parent, size_t id, std::weak_ptr<int> marker):
        id_(id),
        parent_(parent),
        marker_(marker) {}

    ConnectionImpl(const ConnectionImpl& rhs):
        id_(rhs.id_),
        parent_(rhs.parent_),
        marker_(rhs.marker_) {}

    bool operator!=(const ConnectionImpl& rhs) const { return !(*this == rhs); }
    bool operator==(const ConnectionImpl& rhs) const { return this->id_ == rhs.id_; }

    ConnectionImpl& operator=(const ConnectionImpl& rhs) {
        if(*this == rhs) {
            return *this;
        }

        this->id_ = rhs.id_;
        this->parent_ = rhs.parent_;

        return *this;
    }

private:
    size_t id_;
    Disconnector* parent_;
    std::weak_ptr<int> marker_;
    friend class Connection;
};

class Connection {
public:
    Connection():
        impl_({}) {

    }

    Connection(std::shared_ptr<ConnectionImpl> impl):
        impl_(impl) {}

    bool disconnect() {
        auto p = impl_.lock();
        return p && p->marker_.lock() && p->parent_->disconnect(*p);
    }

    bool is_connected() const {
        auto p = impl_.lock();
        return p && p->marker_.lock() && p->parent_->connection_exists(*p);
    }

    operator bool() const {
        return is_connected();
    }

private:
    std::weak_ptr<ConnectionImpl> impl_;
};

class ScopedConnection {
public:
    ScopedConnection(Connection conn):
        counter_(new int(1)),
        conn_(conn) {

    }

    ScopedConnection(const ScopedConnection& rhs):
        counter_(rhs.counter_),
        conn_(Connection()) {

        if(counter_) {
            ++*counter_;
        }
    }

    ScopedConnection& operator=(const ScopedConnection& rhs) {
        if(&rhs == this) {
            return *this;
        }

        clear();
        conn_ = rhs.conn_;
        counter_ = rhs.counter_;

        if(counter_) {
            ++*counter_;
        }

        return *this;
    }

    ~ScopedConnection() {
        clear();
    }

    bool is_connected() const {
        return conn_.is_connected();
    }

    void disconnect() {
        conn_.disconnect();
    }
private:
    int* counter_ = nullptr;
    Connection conn_;

    void clear() {
        if(counter_) {
            if(*counter_ == 1) {
                if(conn_.is_connected()) {
                    conn_.disconnect();
                }
            }

            if(!--*counter_) {
                delete counter_;
                counter_ = nullptr;
            }
        }
    }
};

template<typename> class ProtoSignal;

template<typename R, typename... Args>
class ProtoSignal<R (Args...)> : public Disconnector {
public:
    typedef R result;
    typedef std::function<R (Args...)> callback;

    ~ProtoSignal() {
        Link* it = head_;
        while(it) {
            auto next = it->next;
            delete it;
            it = next;
        }
    }

    Connection connect(const callback& func) {
        static size_t id_counter = 0;
        size_t id = ++id_counter;
        Link new_link;
        new_link.func = func;
        new_link.conn_impl = std::make_shared<ConnectionImpl>(this, id, marker_);

        if(push_link(new_link)) {
            return Connection(new_link.conn_impl);
        } else {
            L_WARN("Error adding connection to signal!");
            return Connection();
        }
    }

    void operator()(Args... args) {
        Link* it = head_;

        ++iterating_;
        while(it) {
            if(!it->is_dead) {
                assert(it->func);
                it->func(args...);
            }

            it = it->next;
        }
        --iterating_;

        shrink_to_fit();
    }

    bool connection_exists(const ConnectionImpl& conn) const  {
        Link* it = head_;
        ++iterating_;
        while(it) {
            if((!it->is_dead) && it->conn_impl.get() == &conn) {
                return true;
            }

            it = it->next;
        }
        --iterating_;
        return false;
    }

    void shrink_to_fit() {
        if(iterating_ > 0) {
            return;
        }

        Link* it = head_;
        Link* prev = nullptr;
        while(it) {
            auto next = it->next;
            if(it->is_dead) {
                if(prev) {
                    prev->next = next;
                } else {
                    assert(!prev);
                    head_ = next;
                }

                if(it == tail_) {
                    tail_ = prev;
                }

                delete it;
            } else {
                prev = it;
            }

            it = next;
        }
    }

    bool disconnect(const ConnectionImpl& conn_impl) {
        bool removed = false;

        Link* it = head_;

        ++iterating_;
        while(it) {
            auto next = it->next;
            if((!it->is_dead) && it->conn_impl.get() == &conn_impl) {
                it->is_dead = true;

                connection_count_--;
                removed = true;
            }

            it = next;
        }
        --iterating_;

        shrink_to_fit();

        return removed;
    }

    std::size_t connection_count() const {
        return connection_count_;
    }

private:
    struct Link {
        callback func;
        std::shared_ptr<ConnectionImpl> conn_impl;
        Link* next = nullptr;
        bool is_dead = false;
    };

    Link* head_ = nullptr;
    Link* tail_ = nullptr;

    /* Keeps track of whether or not we're currently iterating
     * the links. This protects us deleting a thing during an iteration */
    mutable uint8_t iterating_ = 0;

    uint32_t connection_count_ = 0;

    /* This marker is passed as a weak_ptr to all connections. It's
     * used to track whether this signal has been destroyed. If so
     * then any connections pointing to it will fail to get a lock */
    std::shared_ptr<int> marker_ = std::make_shared<int>(1);

    bool push_link(const Link& link) {
        if(!head_) {
            assert(!tail_);
            head_ = tail_ = new Link();
            *head_ = link;
        } else {
            assert(tail_);
            tail_->next = new Link();
            tail_ = tail_->next;
            *tail_ = link;
        }

        connection_count_++;

        return true;
    }
};

template<typename Signature>
class signal : public ProtoSignal<Signature> {

};

typedef Connection connection;
typedef ScopedConnection scoped_connection;

}
}
