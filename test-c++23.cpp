/**
 * Tests for functionality. Compile with -ltbb -std=c++23.
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
		std::cout << "Long, faster updating progress bar:\n";
		size_t n = 1UL << 28;
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
		size_t n = 1UL << 28;
		auto I = std::ranges::iota_view(0UL, n);
		ProgressPrinter progress(n * 3, 73, 234);

		std::for_each(std::execution::par_unseq, I.begin(), I.end(),
		[&progress](size_t i){
			progress.completed_tasks += 1;
		}
		);
	}
}

