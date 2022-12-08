#include <ntddk.h>

//Definitions
#define IOCTL_PROTECT_PID	CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define PROCESS_TERMINATE 1

//Prototypes
DRIVER_UNLOAD ProtectorUnload;
NTSTATUS ProtectorCreateClose(PDEVICE_OBJECT pDevice, PIRP Irp);
NTSTATUS ProtectorDeviceControl(PDEVICE_OBJECT pDevice, PIRP Irp);

OB_PREOP_CALLBACK_STATUS PreOpenProcessOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info);

//Globals
PVOID regHandle;
ULONG protectedPid;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING puString) {

	UNREFERENCED_PARAMETER(puString);

	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\Protector");
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(L"\\??\\Protector");
	PDEVICE_OBJECT DeviceObject = nullptr;

	status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status)) {
		DbgPrint("failed to create device object (status=%08X)\n", status);
		return status;
	}

	status = IoCreateSymbolicLink(&symLinkName, &deviceName);

	if (!NT_SUCCESS(status)) {
		DbgPrint("failed to create symbolic link (status=%08X)", status);
		IoDeleteDevice(DeviceObject);
		return status;
	}

	OB_OPERATION_REGISTRATION operations[] = {
		{
		PsProcessType,
		OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
		PreOpenProcessOperation, nullptr
		}
	};

	OB_CALLBACK_REGISTRATION reg = {
		OB_FLT_REGISTRATION_VERSION,
		1,
		RTL_CONSTANT_STRING(L"12345.6879"),
		nullptr,
		operations
	};

	status = ObRegisterCallbacks(&reg, &regHandle);

	if (!NT_SUCCESS(status)) {
		DbgPrint("failed to register the callback (status=%08X)\n", status);
		IoDeleteSymbolicLink(&symLinkName);
		IoDeleteDevice(DeviceObject);
		return status;
	}

	(*DriverObject).DriverUnload = ProtectorUnload;
	(*DriverObject).MajorFunction[IRP_MJ_CREATE] = (*DriverObject).MajorFunction[IRP_MJ_CLOSE] = ProtectorCreateClose;
	(*DriverObject).MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProtectorDeviceControl;

	DbgPrint("DriverEntry completed successfully\n");
	return status;
}

void ProtectorUnload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("Protector Unload called...\n");
	if (regHandle) {
		ObUnRegisterCallbacks(regHandle);
		regHandle = NULL;
	}

	UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	IoDeleteSymbolicLink(&symbolicLink);
	IoDeleteDevice((*DriverObject).DeviceObject);
}

NTSTATUS ProtectorCreateClose(PDEVICE_OBJECT pDevice, PIRP Irp) {

	UNREFERENCED_PARAMETER(pDevice);

	(*Irp).IoStatus.Status = STATUS_SUCCESS;
	(*Irp).IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS ProtectorDeviceControl(PDEVICE_OBJECT pDevice, PIRP Irp) {

	UNREFERENCED_PARAMETER(pDevice);

	NTSTATUS status = STATUS_SUCCESS;
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	

	switch ((*stack).Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_PROTECT_PID: {
		auto size = (*stack).Parameters.DeviceIoControl.InputBufferLength;
		if (size % sizeof(ULONG) != 0) {
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		auto data = (ULONG*)(*Irp).AssociatedIrp.SystemBuffer;
		protectedPid = *data;
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	(*Irp).IoStatus.Status = status;
	(*Irp).IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

OB_PREOP_CALLBACK_STATUS PreOpenProcessOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info) {
	
	UNREFERENCED_PARAMETER(RegistrationContext);
	
	if ((*Info).KernelHandle)
		return OB_PREOP_SUCCESS;

	auto process = (PEPROCESS)(*Info).Object;
	auto pid = HandleToULong(PsGetProcessId(process));

	if (pid == protectedPid) {
		(*(*Info).Parameters).CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
	}

	return OB_PREOP_SUCCESS;
}