#ifndef MD5_H_INCLUDED
#define MD5_H_INCLUDED

#include <memory>
#include <string>

#include "md5_public_domain.h"

namespace hashlib {

class MD5 {
public:
    MD5();
    MD5(const std::string& data);

    void update(const std::string& data);
    std::string hex_digest();

private:
    std::shared_ptr<MD5_CTX> ctx_;
};

}
#endif // MD5_H_INCLUDED
