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
    ConnectionImpl(Disconnector* parent, size_t id):
        id_(id),
        parent_(parent) {}

    ConnectionImpl(const ConnectionImpl& rhs):
        id_(rhs.id_),
        parent_(rhs.parent_) {}

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
        return p && p->parent_->disconnect(*p);
    }

    bool is_connected() const {
        auto p = impl_.lock();
        return p && p->parent_->connection_exists(*p);
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

    ProtoSignal() {
        connection_counter_ = 0;
    }

    Connection connect(const callback& func) {
        size_t id = increment_counter();
        auto conn_impl = std::make_shared<ConnectionImpl>(this, id);

        Link new_link{func, conn_impl};
        if(push_link(std::move(new_link))) {
            return Connection(conn_impl);
        } else {
            L_WARN("Error adding connection to signal!");
            return Connection();
        }
    }

    void operator()(Args... args) {
        for(auto& link: links_) {
            if(link.func) {
                link.func(args...);
            }
        }
    }

    bool connection_exists(const ConnectionImpl& conn) const  {
        for(auto link: links_) {
            if(link.func && link.conn_impl.get() == &conn) {
                return true;
            }
        }
        return false;
    }

    bool disconnect(const ConnectionImpl& conn_impl) {
        bool removed = false;

        if(!links_) {
            return false;
        }

        Link* it = links_;
        while(it->next) {
            if(it->next->conn_impl.get() == &conn_impl) {
                auto to_del = it->next;
                it->next = it->next->next;
                delete to_del;
                removed = true;
            }

            it = it->next;
        }

        /* Check the root */
        if(links_->conn_impl.get() == &conn_impl) {

        }


        if(links_->conn_impl.get() == &conn_impl) {
            auto t
            links_ = links_->next;

        }

        for(auto& link: links_) {
            if(link.func && link.conn_impl.get() == &conn_impl) {
                link.func = callback();
                link.conn_impl.reset();
                removed = true;
                decrement_counter();
            }
        }

        return removed;
    }

    std::size_t connection_count() const {
        return connection_counter_;
    }

private:
    struct Link {
        callback func;
        std::shared_ptr<ConnectionImpl> conn_impl;

        Link* next = nullptr;
    };

    Link* links_ = nullptr;
    thread::Atomic<size_t> connection_counter_ = {0};

    bool push_link(Link&& link) {
        Link* dest = nullptr;
        if(!links_) {
            links_ = new Link();
            dest = links_;
        } else {
            dest = links_;
            while(dest->next) {
                dest = dest->next;
            }
        }

        *dest = std::move(link);
        increment_counter();

        return true;
    }

    inline size_t increment_counter() {
        connection_counter_++;
        return connection_counter_;
    }

    inline size_t decrement_counter() {
        connection_counter_--;
        return connection_counter_;
    }
};

template<typename Signature>
class signal : public ProtoSignal<Signature> {

};

typedef Connection connection;
typedef ScopedConnection scoped_connection;

}
}
