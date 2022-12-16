#pragma once
#include<Windows.h>

#define IOCTL_PROTECT_PID	CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_REQUEST	CTL_CODE(0x8000, 0x601, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_WRITE_REQUEST	CTL_CODE(0x8000, 0x602, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_MODULE_REQUEST	CTL_CODE(0x8000, 0x603, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_ID_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x604, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

typedef struct _KERNEL_READ_REQUEST
{
	ULONG ProcessId;

	ULONG Address;
	ULONG Response;
	ULONG Size;

} KERNEL_READ_REQUEST, * PKERNEL_READ_REQUEST;

typedef struct _KERNEL_WRITE_REQUEST
{
	ULONG ProcessId;

	ULONG Address;
	ULONG Value;
	ULONG Size;

} KERNEL_WRITE_REQUEST, * PKERNEL_WRITE_REQUEST;


class KeInterface
{
public:
	HANDLE hDriver; // Handle to driver

	// Initializer
	KeInterface(LPCSTR RegistryPath)
	{
		hDriver = CreateFileA(RegistryPath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, nullptr);
	}
	~KeInterface() {
		CloseHandle(hDriver);
	}

	DWORD GetTargetPid()
	{
		if (hDriver == INVALID_HANDLE_VALUE)
			return false;

		ULONG id = 0;
		DWORD bytes = 0;

		if (DeviceIoControl(hDriver, IOCTL_GET_ID_REQUEST, &id, sizeof(id),
			&id, sizeof(id), &bytes, NULL))
			return id;
		else
			return false;
	}

	ULONG GetClientModule()
	{
		if (hDriver == INVALID_HANDLE_VALUE)
			return false;

		ULONG address = 0;
		DWORD bytes = 0;

		if (DeviceIoControl(hDriver, IOCTL_GET_MODULE_REQUEST, &address, sizeof(address),
			&address, sizeof(address), &bytes, NULL))
			return address;
		else
			return false;
	}

	DWORD protectThis() {
		if (hDriver == INVALID_HANDLE_VALUE)
			return false;

		DWORD pid = GetCurrentProcessId();
		std::cout << "pid : " << pid << std::endl;
		DWORD bytes = 0;

		if(DeviceIoControl(hDriver, IOCTL_PROTECT_PID, &pid, sizeof(pid), nullptr, 0, &bytes, nullptr))
			return pid;
		else
			return false;
	}
};