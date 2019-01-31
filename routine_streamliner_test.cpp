#include <iostream>
#include "routine_streamliner.hpp"

int main() {
	std::cout << "Starting" << std::endl;
	RoutineStreamliner<std::string> repeater(std::chrono::milliseconds(300), [] (const std::vector<std::string*>& lines) {
			for (std::string* line : lines)
			std::cout << *line << " ";
			std::cout << std::endl;
		});
	repeater.add("1.1s", std::chrono::milliseconds(1100));
	repeater.add("0.4s", std::chrono::milliseconds(400));
	repeater.add("0.5s", std::chrono::milliseconds(500));
	repeater.add("0.7s", std::chrono::milliseconds(700));
	repeater.add("1.3s", std::chrono::milliseconds(1300));
	unsigned int remove_key = repeater.add("2.8s", std::chrono::milliseconds(2800));
	repeater.remove(remove_key);

	std::this_thread::sleep_for(std::chrono::seconds(10));
	return 0;
}
