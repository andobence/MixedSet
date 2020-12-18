#include "MixedSet.h"
#include <iostream>
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int argc, char* argv[]) {
	//return Catch::Session().run(argc, argv);

	std::cout << "Select test:\n"
		<< "\t1. Correctness test\n"
		<< "\t2. Performance test\n\n"
		<< ">";

	char ch;
	std::cin >> ch;

	if (ch == '1')
	{
		Catch::Session session; // There must be exactly one instance

		// writing to session.configData() here sets defaults
		// this is the preferred way to set them
		session.configData().showDurations = Catch::ShowDurations::Always;

		int returnCode = session.applyCommandLine(argc, argv);
		if (returnCode != 0) // Indicates a command line error
			return returnCode;

		// writing to session.configData() or session.Config() here 
		// overrides command line args
		// only do this if you know you need to

		session.run();
	}
	else if (ch == '2')
	{
		extern void PerformanceTest();
		PerformanceTest();
	}
}
