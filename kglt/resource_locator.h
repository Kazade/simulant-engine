#ifndef RESOURCE_LOCATOR_H
#define RESOURCE_LOCATOR_H

#include <list>
#include <vector>
#include <string>

#include "generic/managed.h"

namespace kglt {

class ResourceLocator :
    public Managed<ResourceLocator> {

public:
    ResourceLocator();

    std::list<std::string>& resource_path() { return resource_path_; }

    std::string locate_file(const std::string& filename);
    std::istringstream read_file(const std::string& filename);
    std::vector<std::string> read_file_lines(const std::string& filename);

private:
    std::string find_executable_directory();
    std::string find_working_directory();

    std::list<std::string> resource_path_;
};

}

#endif // RESOURCE_LOCATOR_H
