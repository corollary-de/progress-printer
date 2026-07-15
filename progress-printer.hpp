/**
 * @file progress-printer.hpp
 * @author Chloé Franke (git@corollary.de)
 * @brief C++11 header-only async progress printer
 * @version 1.1.0
 * @date 2026-06-22
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef PROGRESS_PRINTER_HPP
#define PROGRESS_PRINTER_HPP

#include <array>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <iostream>




namespace _progress_printer_internal {

struct perf_sample
{
	std::chrono::time_point<std::chrono::high_resolution_clock> tp;
	size_t count;
};


struct perf_tracker
{
	const static size_t N = 25;
	std::array<perf_sample, N> points;

	size_t i_first = 0;
	size_t i_last = 0;

	perf_tracker()
	{
		points[0] = {
			std::chrono::high_resolution_clock::now(),
			0
		};
	}


	void update(size_t with)
	{
		i_last = ++i_last % N;

		if (i_last == i_first)
			i_first = ++i_first % N;

		points[i_last] = {
			std::chrono::high_resolution_clock::now(),
			with
		};
	}


	double estimate_rate()
	{
		return static_cast<double>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				points[i_last].tp - points[i_first].tp
			).count()
		) / (
			points[i_last].count - points[i_first].count
		);
	}
};


}




/**
 * @brief Simple async progress printer.
 * Printer thread is started by the constructor and stopped by the deconstructor.
 */
class ProgressPrinter
{
private:
	/**
	 * @brief Format a time in milliseconds into a human-readable string
	 */
	static std::string fmt_time(size_t milliseconds)
	{
		size_t seconds = milliseconds / 1000;
		size_t minutes = seconds / 60;
		size_t hours = minutes / 60;

		char *tmp;
		asprintf(&tmp, "%02ld:%02ld:%02ld.%01ld",
			hours, minutes % 60, seconds % 60, (milliseconds % 1000) / 100
		);
		if (!tmp)
			return "";

		std::string result = tmp;
		free(tmp);
		return result;
	}


	/**
	 * @brief Prints a progress bar at a given width
	 * 
	 * @param progress - progress ranging from 0 to 1
	 * @param width - width of the progress bar in chars
	 */
	inline static void print_progressbar(double progress, size_t width)
	{
		const std::string PROGESS_BAR_LUT[9] = {
			" ", "▏", "▎",
			"▍", "▌", "▋",
			"▊", "▉", "█"
		};

		const std::string ANSI_GREEN = "\x1b[32m";
		const std::string ANSI_YELLOW = "\x1b[33m";
		const std::string ANSI_GREY_BG = "\x1b[40m";
		const std::string ANSI_RESET = "\x1b[0m";

		if (progress < 1){
			std::cout << ANSI_YELLOW + "[ " 
					  << std::setprecision(3) << std::setw(5);

			std::left(std::cout);
			std::cout << progress * 100;
			std::right(std::cout);

			std::cout << "% ] " + ANSI_GREY_BG;

			for (int i = 0; i < width; i++) {
				int bar_i = static_cast<int>(progress * 8 * width) - 8 * i;
				bar_i = bar_i > 8 ? 8 : bar_i;	// std::clamp is c++17
				bar_i = bar_i < 0 ? 0 : bar_i;
				std::cout << PROGESS_BAR_LUT[bar_i];
			}

			std::cout << ANSI_RESET;
		} else {
			std::cout << ANSI_GREEN + "[  Done  ] ";

			for (int i = 0; i < width; i++)
				std::cout << PROGESS_BAR_LUT[8];

			std::cout << ANSI_RESET;
		}

	}


private:
	// Whether this is just a dummy object that doesn't do anything.
	bool is_dummy;

	// The thread responsible for printing the progress to the terminal
	std::thread printer_thread;
	// Offswitch
	std::atomic_bool running;

	// Time at start of progress printing
	std::chrono::time_point<std::chrono::high_resolution_clock> t_start;


	_progress_printer_internal::perf_tracker performance;


	/**
	 * @brief Print an individual line of progress
	 */
	void print_progress_line()
	{
		double progress = static_cast<double>(completed_tasks) / task_completion_goal;

		auto now = std::chrono::high_resolution_clock::now();

		size_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - t_start
		).count();

		size_t eta_ms = performance.estimate_rate() * (task_completion_goal - completed_tasks);

		std::cout << "\x1b[2K\r"; // Clear line of terminal
		std::cout << fmt_time(elapsed_ms) << " ";

		print_progressbar(progress, progress_bar_width);

		if (completed_tasks < task_completion_goal)
			std::cout << " ETA: " << fmt_time(eta_ms);

		std::flush(std::cout);
	}


	/**
	 * @brief Function executed by printer_thread
	 */
	void printer_thread_fn()
	{
		t_start = std::chrono::high_resolution_clock::now();

		while (running) {
			performance.update(completed_tasks);
			print_progress_line();

			std::this_thread::sleep_for(
				std::chrono::milliseconds(polling_interval)
			);
		}
		
		print_progress_line();
	}


public:
	// Current number of completed tasks. Can be updated at any time.
	std::atomic_size_t completed_tasks;

	/**
	 * @brief Goal of tasks to complete. If this is less than or equal to
	 * completed_tasks, the progress is considered complete
	 */
	std::atomic_size_t task_completion_goal;

	/**
	 * @brief Width of the progress bar. I don't see a reason why you would
	 * want to change this at runtime, but it also wouldn't break anything.
	 */
	std::atomic_size_t progress_bar_width;

	/**
	 * @brief Number of milliseconds to sleep between progress reports.
	 */
	std::atomic_size_t polling_interval;


	/**
	 * @brief Construct a new Progress Printer object;
	 * This starts the printer thread. stdout should not be written to
	 * while this printer is in scope.
	 * 
	 * @param goal - Number of tasks for completion
	 * @param width - Width of the progress bar
	 * @param polling_interval - Number of milliseconds to sleep between progress prints
	 * @param dummy - Don't print anything, don't start a thread, just exist.
	 * Useful for if your program has a '--quiet' parameter
	 */
	ProgressPrinter(
		size_t goal,
		size_t width = 50,
		size_t polling_interval = 100,
		bool dummy = false
	) :
		is_dummy(dummy),
		task_completion_goal(goal),
		progress_bar_width(width),
		polling_interval(polling_interval),
		running(true),
		completed_tasks(0)
	{
		if (is_dummy) return;
		printer_thread = std::thread([this](){ printer_thread_fn(); });
	}


	/**
	 * @brief Destroy the Progress Printer object;
	 * This cancels the printer thread.
	 */
	~ProgressPrinter()
	{
		if (is_dummy) return;

		// Kill printer thread
		running = false;
		printer_thread.join();

		std::cout << "\n";
	}
};


#endif // PROGRESS_PRINTER_HPP
