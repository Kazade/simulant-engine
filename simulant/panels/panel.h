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

#pragma once

namespace smlt {

class Panel {
public:
    virtual ~Panel() {}

    const bool is_active() const { return is_active_; }

    void activate() {
        if(is_active_) return;

        do_activate();
        is_active_ = true;
    }

    void deactivate() {
        if(!is_active_) return;

        do_deactivate();
        is_active_ = false;
    }

private:
    bool is_active_ = false;

    virtual void do_activate() = 0;
    virtual void do_deactivate() = 0;
};


}
