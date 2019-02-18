//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <cstdio>
#include <cassert>
#include "md5.h"

namespace hashlib {

MD5::MD5():
    ctx_(std::shared_ptr<MD5_CTX>(new MD5_CTX())) {
    MD5_Init(ctx_.get());
}

MD5::MD5(const std::string& data):
    ctx_(std::shared_ptr<MD5_CTX>(new MD5_CTX())) {

    MD5_Init(ctx_.get());
    update(data);
}

void MD5::update(const std::string& data) {
    assert(!data.empty());
    MD5_Update(ctx_.get(), (void*) data.c_str(), data.length());
}

std::string MD5::hex_digest() {
    unsigned char result[16] = {0};

    MD5_Final(result, ctx_.get());

    std::string final;
    char szTemp[16] = {0};
    for(int i = 0; i < 16; ++i) {
        sprintf(szTemp, "%02X", result[i]);
        final += std::string(szTemp);
    }

    return final;
}

}

