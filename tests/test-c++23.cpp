/**
 * Tests for functionality. Compile with -ltbb -std=c++23.
 */

#include "progress-printer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <execution>
#include <iostream>
#include <ratio>
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
			progress.completed_tasks++;
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
			progress.completed_tasks++;
		}
		);
	}

	{
		std::cout << "Moving Goalpost:\n";
		size_t n = 1UL << 26;
		ProgressPrinter progress(n, 100, 17);

		const size_t factor[] = { 1, 3, 9, 27 };
		const size_t divisor[] = { 1, 2, 4, 8 };

		for (int step = 0; step < 4; step++)
		{
			auto I = std::ranges::iota_view(0UL, n * factor[step] / divisor[step]);

			std::for_each(std::execution::par_unseq, I.begin(), I.end(),
			[&progress](size_t i){
				progress.completed_tasks++;
			}
			);

			if (step < 3)
				progress.task_completion_goal += n * factor[step + 1] / divisor[step + 1];
		}
	}

	{
		std::cout << "Variable speed:\n";
		size_t n = 1UL << 22;
		ProgressPrinter progress(n, 50, 10);

		for (int step = 0; step < 4; step++)
		{
			auto I = std::ranges::iota_view(0UL, n / 4);

			std::for_each(std::execution::par_unseq, I.begin(), I.end(),
			[&progress, step](size_t i){
				progress.completed_tasks++;
				std::this_thread::sleep_for(std::chrono::microseconds(5 << (step * 2)));
			}
			);
		}
	}

	{
		std::cout << "Sin Wave:\n";
		ProgressPrinter progress(400);

		for (float x = 0; x < 10; x += 0.01)
		{
			progress.completed_tasks = (std::sin(x) + 1) * 200;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

