#ifndef RESOURCE_LOCATOR_H
#define RESOURCE_LOCATOR_H

#include <list>
#include <vector>
#include <string>

#include "generic/managed.h"
#include "utils/unicode.h"

namespace smlt {


class ResourceMissingError : public std::runtime_error {
public:
    ResourceMissingError(const std::string& what):
        std::runtime_error(what) {}
};


class ResourceLocator :
    public Managed<ResourceLocator> {

public:
    ResourceLocator();

    std::list<unicode>& resource_path() { return resource_path_; }

    unicode locate_file(const unicode& filename) const;
    std::shared_ptr<std::stringstream> read_file(const unicode& filename);
    std::vector<std::string> read_file_lines(const unicode& filename);

    void add_search_path(const unicode& path);
private:
    unicode find_executable_directory();
    unicode find_working_directory();

    std::list<unicode> resource_path_;
};

}

#endif // RESOURCE_LOCATOR_H
