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

#include <cassert>
#include <mutex>
#include <string>
#include <chrono>
#include <memory>

#include "generic/property.h"
#include "generic/data_carrier.h"
#include "generic/object_manager.h"

namespace smlt {

class AssetManager;

/*
 * An AssetTransaction is the only way to manipulate
 * an asset once it's been created. There are several
 * reasons for this, but ultimately it comes down to
 * thread safety.
 *
 * If an asset is being manipulated once a renderable
 * is in the queue, then what is rendered will be incorrect
 * (e.g. the order could be wrong) at best, or cause
 * crashes at worst.
 *
 * AssetTransaction leverages atomic swaps and the pimpl
 * pattern to make sure that things are thread-safe
 */

template<typename T>
class AssetTransaction {
public:
    typedef typename T::impl_type impl_type;

    AssetTransaction(std::shared_ptr<T> source):
        source_(source) {

        assert(source);
        target_ = source->pimpl_;
    }

    virtual ~AssetTransaction() {
        rollback();
    }

    void commit() {
        if(is_dirty_) {
            on_commit();
            std::swap(source_->pimpl_, target_);
            committed_ = true;
        }
    }

    void rollback() {
        if(committed_) return;

        on_rollback();

        L_WARN("Rolling back uncommitted asset transaction");
    }

protected:
    /* If a change is made, the transaction needs to be marked "dirty"
     * otherwise commit() will be a no-op */

    void mark_dirty() {
        is_dirty_ = true;
    }

    std::shared_ptr<T> source_;
    std::shared_ptr<impl_type> target_;

private:
    virtual void on_commit() {}
    virtual void on_rollback() {}

    bool is_dirty_ = false;
    bool committed_ = false;
};

/*
 * All assets that implement AtomicAsset store their
 * main implementation in the provided pimpl_ member
 */
template<typename T, typename AssetImplType, typename AssetTransactionType>
class AtomicAsset:
    public virtual std::enable_shared_from_this<T> {

    friend class AssetTransaction<T>;

public:
    typedef AssetTransactionType transaction_type;
    typedef AssetImplType impl_type;

    typedef std::shared_ptr<AssetTransactionType> transaction_pointer_type;

    AtomicAsset(std::shared_ptr<AssetImplType> impl):
        pimpl_(impl) {}

    virtual ~AtomicAsset() {}


    transaction_pointer_type begin_transaction() {
        return std::make_shared<transaction_type>(this->shared_from_this());
    }

protected:
    std::shared_ptr<AssetImplType> pimpl_;
};


class Asset {
public:
    friend class AssetManager;

    Asset(AssetManager* manager);

    virtual ~Asset() {}

    AssetManager& asset_manager() { assert(manager_); return *manager_; }
    const AssetManager& asset_manager() const { assert(manager_); return *manager_; }

    int age() const;

    void set_garbage_collection_method(GarbageCollectMethod method);

    Property<Asset, generic::DataCarrier> data = {this, &Asset::data_};

protected:
    Asset(const Asset& rhs);
    Asset& operator=(const Asset& rhs);
private:
    AssetManager* manager_;

    std::chrono::time_point<std::chrono::system_clock> created_;

    generic::DataCarrier data_;
};

}

