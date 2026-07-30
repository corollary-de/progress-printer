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
#include <cmath>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <iostream>




namespace _progress_printer_internal {

/**
 * @brief Represents a sample of performance, that is a progress count value
 * and the time at which it was sampled
 */
struct perf_sample
{
	std::chrono::time_point<std::chrono::high_resolution_clock> tp;
	size_t count;
};


/**
 * @brief Represents a ring buffer of performance samples with a method
 * to estimate a rate `time / progress count` over this buffer.
 * 
 */
struct perf_tracker
{
	// Size of the buffer
	const static size_t N = 25;
	// Samples
	std::array<perf_sample, N> points;

	// Index of first element
	size_t i_first = 0;
	// Index of last element
	size_t i_last = 0;

	perf_tracker()
	{
		points[0] = {
			std::chrono::high_resolution_clock::now(),
			0
		};
	}


	/**
	 * @brief Add another point of progress to the ring buffer
	 * 
	 * @param with progress count value to add
	 */
	void update(size_t with) noexcept
	{
		i_last = ++i_last % N;

		if (i_last == i_first)
			i_first = ++i_first % N;

		points[i_last] = {
			std::chrono::high_resolution_clock::now(),
			with
		};
	}


	/**
	 * @brief Return an estimate of `time / progress count` calculated
	 * via linear extrapolation on the internal ring buffer.
	 * 
	 * @return Rate estimate or NaN if change in count is 0
	 */
	double estimate_rate() noexcept
	{
		size_t delta_count = points[i_last].count - points[i_first].count;
		if (delta_count == 0) return NAN;

		double delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
			points[i_last].tp - points[i_first].tp
		).count();

		return delta_t / delta_count;
	}
};


/**
 * @brief Clamp a value between low and high, since std::clamp is c++17
 * 
 * @param value Value to return clamped
 * @param low Lower bound (inclusive)
 * @param high Upper bound (inclusive)
 * @return Value clamped between low and high
 */
inline int clamp(int value, int low, int high) {
	value = value > high ? high : value;
	value = value < low ? low : value;
	return value;
}


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
	static std::string fmt_time(size_t milliseconds) noexcept
	{
		size_t seconds = milliseconds / 1000;
		size_t minutes = seconds / 60;
		size_t hours = minutes / 60;
		size_t days = hours / 24;
		milliseconds = (milliseconds % 1000) / 100;
		seconds %= 60;
		minutes %= 60;
		hours %= 24;

		std::ostringstream oss;

		// > 1 Year is not a reasonable time and probably 
		// an ETA resulting from a progress rate of 0
		if (days > 365) {
			oss << "Unknown";
		} else if (days > 0) {
			oss << days << "d "
				<< std::setfill('0') << std::setw(2) << hours << ":"
				<< std::setfill('0') << std::setw(2) << minutes;
		} else if (hours > 0) {
			oss << std::setfill('0') << std::setw(2) << hours << ":"
				<< std::setfill('0') << std::setw(2) << minutes << ":"
				<< std::setfill('0') << std::setw(2) << seconds;
		} else {
			oss << std::setfill('0') << std::setw(2) << minutes << ":"
				<< std::setfill('0') << std::setw(2) << seconds << "."
				<< std::setfill('0') << std::setw(1) << milliseconds;
			}

		return oss.str();
	}


	/**
	 * @brief Prints a progress bar at a given width
	 * 
	 * @param progress - progress ranging from 0 to 1
	 * @param width - width of the progress bar in chars
	 */
	inline static void print_progressbar(double progress, size_t width) noexcept
	{
		const std::string BAR_COLOR_COMPLETE = "\x1b[32m";		// Green
		const std::string BAR_COLOR_INCOMPLETE = "\x1b[33m";	// Yellow
		const std::string BAR_COLOR_BACKGROUND = "\x1b[40m";	// Gray
		const std::string ANSI_RESET = "\x1b[0m";

		const std::string PROGRESS_BAR_LUT[9] = {
				" ", "▏", "▎",
				"▍", "▌", "▋",
				"▊", "▉", "█"
		};

		if (progress < 1){
			std::cout << BAR_COLOR_INCOMPLETE + "[ ";

			std::cout << std::left << std::setprecision(3) << std::setw(5)
					  << progress * 100 << std::right;

			std::cout << "% ] " + BAR_COLOR_BACKGROUND;

			const size_t subdiv = 8; // 8 subdivisions per char of the progress bar

			size_t bar_position = progress * width * subdiv;

			for (int i = 0; i < width * subdiv; i += subdiv) {
				if (i + 8 < bar_position)
					// Bar position is to the right, so this cell is filled in
					std::cout << PROGRESS_BAR_LUT[8];
				else if (i > bar_position)
					// Bar position is to the left, so this cell is not filled in
					std::cout << PROGRESS_BAR_LUT[0];
				else
					// Bar position is in this cell, so choose according subdivision
					std::cout << PROGRESS_BAR_LUT[bar_position - i];
			}

			std::cout << ANSI_RESET;
		} else {
			std::cout << BAR_COLOR_COMPLETE + "[  Done  ] ";

			for (int i = 0; i < width; i++)
				std::cout << PROGRESS_BAR_LUT[8];

			std::cout << ANSI_RESET;
		}

	}


private:
	// Whether this is just a dummy object that doesn't do anything.
	bool is_dummy;

	// Width of the progress bar.
	size_t progress_bar_width;

	// Number of milliseconds to sleep between progress reports.
	size_t polling_interval;

	// The thread responsible for printing the progress to the terminal
	std::thread printer_thread;
	// Offswitch
	std::atomic<bool> running;

	// Time at start of progress printing
	std::chrono::time_point<std::chrono::high_resolution_clock> t_start;


	_progress_printer_internal::perf_tracker performance;


	/**
	 * @brief Print an individual line of progress
	 */
	void print_progress_line() noexcept
	{
		const std::string ANSI_CLEAR_LINE = "\x1b[2K\r";

		// Lock in values so they can't be changed while print_progress_line
		// is running and guard against 0
		size_t goal = task_completion_goal;
		size_t complete = completed_tasks;

		auto now = std::chrono::high_resolution_clock::now();

		size_t elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - t_start
		).count();

		double rate = performance.estimate_rate();
		size_t eta_ms;
		if (rate < 0 || std::isnan(rate))
			eta_ms = ~(0UL);
		else
			eta_ms = rate * (goal - complete);

		std::cout << ANSI_CLEAR_LINE; // Clear line of terminal
		std::cout << fmt_time(elapsed_ms) << " ";

		if (goal > 0) {
			double progress = static_cast<double>(complete) / goal;
			print_progressbar(progress, progress_bar_width);
		}

		if (complete < goal)
			std::cout << " ETA: " << fmt_time(eta_ms);

		std::cout << std::flush;
	}


	/**
	 * @brief Function executed by printer_thread
	 */
	void printer_thread_fn() noexcept
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
	std::atomic<size_t> completed_tasks;

	/**
	 * @brief Goal of tasks to complete. If this is less than or equal to
	 * completed_tasks, the progress is considered complete
	 */
	std::atomic<size_t> task_completion_goal;

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
		if (polling_interval == 0) throw std::invalid_argument("Polling interval may not be 0");
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

	ProgressPrinter(const ProgressPrinter &) = delete;
	ProgressPrinter & operator=(const ProgressPrinter &) = delete;
};


#endif // PROGRESS_PRINTER_HPP
