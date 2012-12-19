#include <unittest++/UnitTest++.h>
#include <iostream>

#include "kazbase/logging/logging.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

	//Disable cout for tests
	std::cout.rdbuf(0); 
	return UnitTest::RunAllTests();
}
