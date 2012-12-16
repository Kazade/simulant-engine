#include "kazbase/os/path.h"
#include "resource_locator.h"

namespace kglt {

ResourceLocator::ResourceLocator() {
    resource_path_.push_back(find_executable_directory()); //Make sure the directory the executable lives is on the resource path
    resource_path_.push_back(find_working_directory()); //Add the working directory (might be different)
    resource_path_.push_back("/usr/share"); //Look in /usr/share (kglt files might be installed to /usr/share/kglt)
}

std::string ResourceLocator::locate_file(const std::string& filename) {
    for(std::string path: resource_path_) {
        std::string full_path = os::path::join(path, filename);
        if(os::path::exists(full_path)) {
            return full_path;
        }
    }

    throw IOError("Unable to find file: " + filename);
}

std::istringstream ResourceLocator::read_file(const std::string& filename) {
    std::string path = locate_file(filename);

    std::ifstream file_in(path.c_str());
    if(!file_in) {
        throw IOError("Unable to load file: " + filename);
    }

    std::istringstream result;
    result << file_in.rdbuf();
    return result;
}

std::vector<std::string> ResourceLocator::read_file_lines(const std::string& filename) {
    std::string path = locate_file(filename);

    std::ifstream file_in(path.c_str());
    if(!file_in) {
        throw IOError("Unable to load file: " + filename);
    }

    std::vector<std::string> results;
    std::string line;
    while(std::getline(file_in, line)) {
        results.push_back(line);
    }
    return results;
}

std::string ResourceLocator::find_executable_directory() {
    return os::path::exe_path();
}

std::string ResourceLocator::find_working_directory() {
    return os::path::cwd();
}

}
