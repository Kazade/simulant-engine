#include <unittest++/UnitTest++.h>
#include <iostream>

int main(int argc, char* argv[]) {
	//Disable cout for tests
	std::cout.rdbuf(0); 
	return UnitTest::RunAllTests();
}
