#include <unittest++/UnitTest++.h>
#include <unittest++/TestReporterStdout.h>
#include <iostream>

#include "kazbase/logging/logging.h"

int main(int argc, char* argv[]) {
    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

	//Disable cout for tests
	std::cout.rdbuf(0); 

    if(argc > 1) {
        //if first arg is "suite", we search for suite names instead of test names
        const bool bSuite = strcmp( "suite", argv[ 1 ] ) == 0;

        //walk list of all tests, add those with a name that
        //matches one of the arguments  to a new TestList
        const UnitTest::TestList& allTests( UnitTest::Test::GetTestList() );
        UnitTest::TestList selectedTests;
        UnitTest::Test* p = allTests.GetHead();
        while(p) {
            for(int i = 1 ; i < argc ; ++i) {
                if( strcmp( bSuite ? p->m_details.suiteName
                                 : p->m_details.testName, argv[ i ] ) == 0 ) {
                    selectedTests.Add( p );
                }
            }
            UnitTest::Test* q = p;
            p = p->next;
            q->next = NULL;
        }

        //run selected test(s) only
        UnitTest::TestReporterStdout reporter;
        UnitTest::TestRunner runner( reporter );
        return runner.RunTestsIf(selectedTests, 0, UnitTest::True(), 0);
    } else {
        return UnitTest::RunAllTests();
    }
}
