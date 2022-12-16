#include<iostream>
#include "KeInterface.h"

int main(int argc, const char* argv[]) {
	SetConsoleTitle(L"KernelCom");
	KeInterface Driver("\\\\.\\Protector");

	Driver.protectThis();

	DWORD procId = Driver.GetTargetPid();
	ULONG clientAddress = Driver.GetClientModule();

	std::cout << "procId : " << procId << std::endl;
	std::cout << "clientAddress : " << clientAddress << std::endl;
	std::cin.get();

	return 0;
}



/*
	DWORD pid = atoi(argv[1]);
	HANDLE device = CreateFile(L"\\\\.\\Protector", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	if (device == INVALID_HANDLE_VALUE) {
		std::cout << "Failed to open device" << std::endl;
		return 1;
	}

	// Send Pid to be protected
	bool success = DeviceIoControl(device, IOCTL_PROTECT_PID, &pid, sizeof(pid), nullptr, 0, &bytes, nullptr);
	CloseHandle(device);

	if (!success) {
		std::cout << "Failed in DeviceIoControl: " << GetLastError() << std::endl;
		return 1;
	}

	std::cout << "Protected process with pid: " << pid << std::endl;
*/