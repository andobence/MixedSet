#include "MixedSet.h"
#include <iostream>
#include <optional>

#include "vec3.h"

void Test2()
{
	MixedSet<vec3, Vec3Linearizer<>> set;

	for (int i = 0; i < 1024*1024*2; i++)
	{
		set.insert({ i, 0, 0 });
	}

#define TEST(cmd) std::cout << #cmd << '\t' << (cmd) << std::endl;

	TEST(set.contains({ 10, 0, 0 }));
	TEST(set.contains({ 11, 0, 0 }));
	TEST(set.contains({ 1022, 0, 0 }));
	TEST(set.contains({ 1023, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
	TEST(set.erase({ 11, 0, 0 }));
	TEST(set.erase({ 1023, 0, 0 }));
	TEST(set.erase({ 11, 0, 0 }));
	TEST(set.contains({ 10, 0, 0 }));
	TEST(set.contains({ 11, 0, 0 }));
	TEST(set.contains({ 1022, 0, 0 }));
	TEST(set.contains({ 1023, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
	TEST(set.insert({ 1024, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
	TEST(set.erase({ 1024, 0, 0 }));
	TEST(set.contains({ 1024, 0, 0 }));
}
