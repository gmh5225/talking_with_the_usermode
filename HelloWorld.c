// Credits: 1.) https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver
//			2.) https://idov31.github.io/2022/07/14/lord-of-the-ring0-p1.html
#include <ntddk.h>
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KmdfHelloWorldEvtDeviceAdd;
void MyUnload(PDRIVER_OBJECT);

NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	// MSDN Tutorial;
	// Record success or failure
	// NTSTATUS status = STATUS_SUCCESS;

	// Driver configuration object;
	// WDF_DRIVER_CONFIG config;

	// Print Hello world for Driver Entry
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "talking_to_the_user_mode: DriverEntry\n"));
	// Initialize driver config object to register entry point 
	// for evtdeviceadd callback, KmdfHelloWorldEvtDeviceAdd
	// WDF_DRIVER_CONFIG_INIT(&config, KmdfHelloWorldEvtDeviceAdd);
	
	// Create driver object
	// status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
	
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = MyUnload;
	DbgPrint("talking_to_the_user_mode: DriverEntry\n");
	return STATUS_SUCCESS;
}

NTSTATUS KmdfHelloWorldEvtDeviceAdd(
	_In_ WDFDRIVER Driver,
	_Inout_ PWDFDEVICE_INIT DeviceInit
)
{
	// Not using Driver object so mark as unreferenced
	UNREFERENCED_PARAMETER(Driver);

	NTSTATUS status;

	//Allocate device object
	WDFDEVICE hDevice;

	// Print Hello World
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "talking_to_the_user_mode: KmdfHelloWorldEvtDeviceAdd\n"));
	DbgPrint("talking_to_the_user_mode: KmdfHelloWorldEvtDeviceAdd\n");

	status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);
	return status;
}

void MyUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "talking_to_the_user_mode: Driver Unload\n"));
	DbgPrint("talking_to_the_user_mode: Driver Unload\n");
}