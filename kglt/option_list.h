#ifndef OPTION_LIST_H
#define OPTION_LIST_H

#include <stdexcept>
#include <initializer_list>
#include <boost/lexical_cast.hpp>

namespace kglt {
namespace option_list {
	
class OptionDoesNotExist : public std::logic_error {
public:
	OptionDoesNotExist(const std::string& which):
		std::logic_error(which + std::string(" does not exist as an option")) {}
};
	
typedef std::initializer_list<std::string> OptionList;

template<typename T>
T get_as(const OptionList& options, const std::string& key) {
	bool next_is_value = false;
	
	for(std::string entry: options) {
		if(entry == key) {
			next_is_value = true;
		} else if(next_is_value) {
			//If the last value was the key, this one is the value
			return boost::lexical_cast<T>(entry);
		}		
	}
	
	throw OptionDoesNotExist(key);
}

}
}
#endif
