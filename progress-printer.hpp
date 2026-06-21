#ifndef PROGRESS_PRINTER_HPP
#define PROGRESS_PRINTER_HPP

#include <string>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <iostream>



/**
 * @brief Sinple async progress printer.
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
	 * @brief Clear the line of the terminal
	 */
	static void clear_line()
	{
		std::cout << "\x1b[2K\r";
	}

	const static std::string PROGESS_BAR_LUT[9];


	/**
	 * @brief Prints a progress bar at a given width
	 * 
	 * @param progress progress ranging from 0 to 1
	 * @param width width in chars of the progress bar
	 */
	inline static void print_progressbar(double progress, size_t width)
	{
		if (progress < 1) {
			std::cout << "\x1b[33m"		// Yellow
					  << "[ " 
					  << std::setprecision(3) 
					  << std::setw(5);
			std::left(std::cout);
			std::cout << progress * 100;
			std::right(std::cout);
			std::cout << "% ] "
					  << "\x1b[40m";	// Grey BG

			for (int i = 0; i < width; i++) {
				int bar_i = static_cast<int>(progress * 8 * width) - 8 * i;
				bar_i = bar_i > 8 ? 8 : bar_i;
				bar_i = bar_i < 0 ? 0 : bar_i;
				std::cout << PROGESS_BAR_LUT[bar_i];
			}

			std::cout << "\x1b[0m"; // Clear
		} else {
			std::cout << "\x1b[32m" // Green
					  << "[  Done  ] ";

			for (int i = 0; i < width; i++)
				std::cout << PROGESS_BAR_LUT[8];

			std::cout << "\x1b[0m"; // Clear
		}

	}


private:
	// The thread responsible for printing the progress to the terminal
	std::thread printer_thread;
	// Killswitch
	std::atomic_bool running;

	std::chrono::time_point<std::chrono::high_resolution_clock> t_start;

	/**
	 * @brief Return the number of milliseconds since start of progess printing
	 */
	size_t get_elapsed_milliseconds()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now() - t_start
		).count();
	}


	/**
	 * @brief Print an individual line of progress
	 */
	void print_progress_line()
	{
		double progress = static_cast<double>(completed_tasks) / task_completion_goal;
		size_t elapsed_ms = get_elapsed_milliseconds();
		size_t eta_ms = elapsed_ms * (1. - progress) / progress;

		clear_line();

		std::cout << fmt_time(elapsed_ms) << " ";

		print_progressbar(progress, progress_bar_width);

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
			size_t eta_ms = get_elapsed_milliseconds();

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
	 */
	ProgressPrinter(size_t goal, size_t width = 50, size_t polling_interval = 100) :
		task_completion_goal(goal),
		progress_bar_width(width),
		polling_interval(polling_interval),
		running(true),
		completed_tasks(0)
	{
		printer_thread = std::thread([this](){ printer_thread_fn(); });
	}


	/**
	 * @brief Destroy the Progress Printer object;
	 * This cancels the printer thread.
	 */
	~ProgressPrinter()
	{
		// Kill printer thread
		running = false;
		printer_thread.join();

		std::cout << "\n";
	}
};

 
const std::string ProgressPrinter::PROGESS_BAR_LUT[9] = {
	" ", "▏", "▎", "▍",
	"▌", "▋", "▊", "▉", "█"
};


std::string ProgressPrinter::fmt_time(size_t milliseconds)
{
}



#endif // PROGRESS_PRINTER_HPP
