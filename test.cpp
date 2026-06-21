/**
 * This test file needs to be compiled with -ltbb and -std=c++23
 * the same is not true for the progress printer itself
 */

#include "progress-printer.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <execution>
#include <iostream>
#include <thread>
#include <ranges>



int main()
{
	{
		std::cout << "Should run for about 10 seconds:\n";
		size_t n = 10000;
		ProgressPrinter progress(n);

		for (size_t i = 0; i < n; i++)
		{
			progress.completed_tasks += 1;
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	{
		std::cout << "Long, faster updating progress bar:\n";
		size_t n = 1UL << 30;
		auto I = std::ranges::iota_view(0UL, n);
		ProgressPrinter progress(n, 150, 10);

		std::for_each(std::execution::par_unseq, I.begin(), I.end(),
		[&progress](size_t i){
			progress.completed_tasks += 1;
		}
		);
	}

	{
		std::cout << "Won't complete:\n";
		size_t n = 1UL << 30;
		auto I = std::ranges::iota_view(0UL, n);
		ProgressPrinter progress(n * 3, 73, 234);

		std::for_each(std::execution::par_unseq, I.begin(), I.end(),
		[&progress](size_t i){
			progress.completed_tasks += 1;
		}
		);
	}
}

