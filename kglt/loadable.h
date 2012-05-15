#ifndef KGLT_LOADABLE_H
#define KGLT_LOADABLE_H

namespace kglt {

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
