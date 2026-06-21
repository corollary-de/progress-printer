/**
 * 	Tests for c++11 compliance. Compile with -std=c++11.
 */

#include "progress-printer.hpp"

#include <cstddef>
#include <iostream>
#include <chrono>
#include <thread>



int main()
{
	{
		std::cout << "Should run for about 3 seconds:\n";
		size_t n = 3000;
		ProgressPrinter progress(n);

		for (size_t i = 0; i < n; i++)
		{
			progress.completed_tasks += 1;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}

