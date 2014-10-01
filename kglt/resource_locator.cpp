#include <kazbase/os/path.h>
#include <kazbase/exceptions.h>
#include "resource_locator.h"

#ifdef __ANDROID__
#include <SDL_rwops.h>
#endif

namespace kglt {

ResourceLocator::ResourceLocator() {
    resource_path_.push_back(find_executable_directory()); //Make sure the directory the executable lives is on the resource path
    resource_path_.push_back(find_working_directory()); //Add the working directory (might be different)
    resource_path_.push_back("/usr/local/share"); //Look in /usr/share (kglt files might be installed to /usr/share/kglt)
    resource_path_.push_back("/usr/share"); //Look in /usr/share (kglt files might be installed to /usr/share/kglt)
}

void ResourceLocator::add_search_path(const unicode& path) {
    resource_path_.push_back(path);
}

unicode ResourceLocator::locate_file(const unicode &filename) const {
    /**
      Locates a file on one of the resource paths, throws an IOError if the file
      cannot be found
    */
#ifdef __ANDROID__
    //On Android we use SDL_RWops which reads from the APK
    SDL_RWops* ops = SDL_RWFromFile(filename.encode().c_str(), "rb");
    if(ops) {
        //If we could open the file, return the filename
        SDL_RWclose(ops);
        return filename;
    }

#else

    if(os::path::exists(filename)) { //Absolute path
        return os::path::abs_path(filename);
    }

    for(unicode path: resource_path_) {
        unicode full_path = os::path::join(path, filename);
        if(os::path::exists(full_path)) {
            return os::path::abs_path(full_path);
        }
    }
#endif
    throw IOError(_u("Unable to find file: ") + filename);
}

std::shared_ptr<std::stringstream> ResourceLocator::read_file(const unicode& filename) {
#ifdef __ANDROID__
    //If we're on Android, don't bother trying to locate the file, just try to load it from the APK
    std::shared_ptr<std::stringstream> result = std::make_shared<std::stringstream>();
    SDL_RWops* ops = SDL_RWFromFile(filename.encode().c_str(), "r");
    if(ops) {
        //If we could open the file, return the filename
        SDL_RWseek(ops, 0, SEEK_END);
        int length = SDL_RWtell(ops);
        SDL_RWseek(ops, 0, SEEK_SET);

        std::vector<unsigned char> data(length); //Make room for all the data
        SDL_RWread(ops, &data[0], sizeof(unsigned char), length);
        SDL_FreeRW(ops);

        std::string str(data.begin(), data.end());
        //Populate the stringstream
        (*result) << str;
        return result;
    } else {
        throw IOError(_u("Unable to load file: ") + filename);
    }
    SDL_FreeRW(ops);
#else
    unicode path = locate_file(filename);

    std::ifstream file_in(path.encode());
    if(!file_in) {
        throw IOError(_u("Unable to load file: ") + filename);
    }

    std::shared_ptr<std::stringstream> result(new std::stringstream);
    (*result) << file_in.rdbuf();
    return result;
#endif
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
