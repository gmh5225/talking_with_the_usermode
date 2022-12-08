#include<iostream>
#include<Windows.h>

int main() {
	int counter = 0;
	std::cout << GetCurrentProcessId() << std::endl;
	for (;;) {
		if (counter % 5 == 0) {
			std::cout << "Tick" << std::endl;
		}

		counter++;
		Sleep(1000);
	}
}