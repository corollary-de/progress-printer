#!/bin/bash


fail()
{
	echo "Test $1 failed"
	exit
}


clang++ -std=c++11 test-c++11.cpp -o test-c++11 || fail "c++11 compilation"
./test-c++11 || fail "c++11 run"
rm test-c++11

clang++ -std=c++23 -ltbb test-c++23.cpp -o test-c++23 || fail "c++23 compilation"
./test-c++23 || fail "c++23 run"
rm test-c++23

echo "All tests passed."

