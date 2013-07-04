#include "kglt/kazbase/os/path.h"
#include "kglt/kazbase/exceptions.h"
#include "resource_locator.h"

namespace kglt {

ResourceLocator::ResourceLocator() {
    resource_path_.push_back(find_executable_directory()); //Make sure the directory the executable lives is on the resource path
    resource_path_.push_back(find_working_directory()); //Add the working directory (might be different)
    resource_path_.push_back("/usr/local/share"); //Look in /usr/share (kglt files might be installed to /usr/share/kglt)
    resource_path_.push_back("/usr/share"); //Look in /usr/share (kglt files might be installed to /usr/share/kglt)
}

unicode ResourceLocator::locate_file(const unicode &filename) {
    /**
      Locates a file on one of the resource paths, throws an IOError if the file
      cannot be found
    */
    if(os::path::exists(filename)) { //Absolute path
        return os::path::abs_path(filename);
    }

    for(unicode path: resource_path_) {
        unicode full_path = os::path::join(path, filename);
        if(os::path::exists(full_path)) {
            return os::path::abs_path(full_path);
        }
    }

    throw IOError(_u("Unable to find file: ") + filename);
}

std::shared_ptr<std::stringstream> ResourceLocator::read_file(const unicode& filename) {
    unicode path = locate_file(filename);

    std::ifstream file_in(path.encode());
    if(!file_in) {
        throw IOError(_u("Unable to load file: ") + filename);
    }

    std::shared_ptr<std::stringstream> result(new std::stringstream);
    (*result) << file_in.rdbuf();
    return result;
}

std::vector<std::string> ResourceLocator::read_file_lines(const unicode &filename) {
    unicode path = locate_file(filename);

    std::ifstream file_in(path.encode().c_str());
    if(!file_in) {
        throw IOError(_u("Unable to load file: ") + filename);
    }

    std::vector<std::string> results;
    std::string line;
    while(std::getline(file_in, line)) {
        results.push_back(line);
    }
    return results;
}

unicode ResourceLocator::find_executable_directory() {
    return os::path::exe_dirname();
}

unicode ResourceLocator::find_working_directory() {
    return os::path::get_cwd();
}

}
