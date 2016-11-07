#ifndef SIMULANT_LOADABLE_H
#define SIMULANT_LOADABLE_H

#include <string>

namespace smlt {

class Loadable {
public:
	virtual ~Loadable() {}
	
	const std::string& name() { return name_; }
	void set_name(const std::string& name) { name_ = name; }
	
private:
	std::string name_;
};

}

#endif
