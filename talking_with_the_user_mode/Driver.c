#include"utils.h"

//Definitions
#define IOCTL_PROTECT_PID	CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_REQUEST	CTL_CODE(0x8000, 0x601, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_WRITE_REQUEST	CTL_CODE(0x8000, 0x602, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_MODULE_REQUEST	CTL_CODE(0x8000, 0x603, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IOCTL_GET_ID_REQUEST CTL_CODE(FILE_DEVICE_UNKNOWN, 0x604, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#define PROCESS_TERMINATE 1
#define PROCESS_VM_READ 0x10
#define PROCESS_CREATE_THREAD 0x2
#define PROCESS_VM_OPERATION 8

//Prototypes
DRIVER_UNLOAD ProtectorUnload;
NTSTATUS ProtectorCreateClose(PDEVICE_OBJECT pDevice, PIRP Irp);
NTSTATUS ProtectorDeviceControl(PDEVICE_OBJECT pDevice, PIRP Irp);

OB_PREOP_CALLBACK_STATUS PreOpenProcessOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info);
void ImageLoadCallback(PUNICODE_STRING FullImageName,
	HANDLE ProcessId, PIMAGE_INFO ImageInfo);

//Globals
PVOID regHandle;
ULONG protectedPid = NULL;
PVOID clientAddress;
ULONG csgoId;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING puString) {

	UNREFERENCED_PARAMETER(puString);

	DbgPrintEx(0, 0, "85693.1267 Driver Loading...\n");

	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\Protector");
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(L"\\??\\Protector");
	PDEVICE_OBJECT DeviceObject = NULL;

	PsSetLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)ImageLoadCallback);

	status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "failed to create device object (status=%08X)\n", status);
		return status;
	}

	status = IoCreateSymbolicLink(&symLinkName, &deviceName);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "failed to create symbolic link (status=%08X)", status);
		IoDeleteDevice(DeviceObject);
		return status;
	}

	OB_OPERATION_REGISTRATION operations[] = {
		{
		PsProcessType,
		OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE,
		PreOpenProcessOperation, NULL
		}
	};

	OB_CALLBACK_REGISTRATION reg = {
		OB_FLT_REGISTRATION_VERSION,
		1,
		RTL_CONSTANT_STRING(L"85693.1267"),
		NULL,
		operations
	};

	status = ObRegisterCallbacks(&reg, &regHandle);

	if (!NT_SUCCESS(status)) {
		DbgPrintEx(0, 0, "failed to register the callback (status=%08X)\n", status);
		IoDeleteSymbolicLink(&symLinkName);
		IoDeleteDevice(DeviceObject);
		return status;
	}

	(*DriverObject).DriverUnload = ProtectorUnload;
	(*DriverObject).MajorFunction[IRP_MJ_CREATE] = (*DriverObject).MajorFunction[IRP_MJ_CLOSE] = ProtectorCreateClose;
	(*DriverObject).MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProtectorDeviceControl;

	DbgPrintEx(0, 0, "DriverEntry completed successfully\n");
	return status;
}

void ProtectorUnload(PDRIVER_OBJECT DriverObject) {
	DbgPrintEx(0, 0, "Protector Unload called...\n");
	if (regHandle) {
		ObUnRegisterCallbacks(regHandle);
		regHandle = NULL;
	}

	PsRemoveLoadImageNotifyRoutine((PLOAD_IMAGE_NOTIFY_ROUTINE)ImageLoadCallback);
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

	static int counter = 0;

	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION  stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG len = 0;
	
	switch ((*stack).Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_PROTECT_PID: {
		ULONG size = (*stack).Parameters.DeviceIoControl.InputBufferLength;
		if (size % sizeof(ULONG) != 0) {
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		ULONG* buffer = (ULONG*)(*Irp).AssociatedIrp.SystemBuffer;
		protectedPid = *buffer;
		//DbgPrintEx(0, 0, "Protect Pid:  %d", *buffer);
		break;
	}
	case IOCTL_READ_REQUEST: {
		// Get the input buffer & format it to our struct
		PKERNEL_READ_REQUEST ReadInput = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;
		PKERNEL_READ_REQUEST ReadOutput = (PKERNEL_READ_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		PEPROCESS Process;
		// Get our process
		if (NT_SUCCESS(PsLookupProcessByProcessId(ReadInput->ProcessId, &Process)))
			KeReadVirtualMemory(Process, ReadInput->Address,
				&ReadInput->Response, ReadInput->Size);

		/*
		if (counter % 20 == 0)
			DbgPrintEx(0, 0, "Read response / address:  %lu, %lu \n", ReadInput->Response, ReadInput->Address);
		*/
		status = STATUS_SUCCESS;
		len = sizeof(KERNEL_READ_REQUEST);
		break;
	}
	case IOCTL_WRITE_REQUEST:
	{
		PKERNEL_WRITE_REQUEST WriteInput = (PKERNEL_WRITE_REQUEST)Irp->AssociatedIrp.SystemBuffer;

		PEPROCESS Process;
		// Get our process
		if (NT_SUCCESS(PsLookupProcessByProcessId(WriteInput->ProcessId, &Process)))
			KeWriteVirtualMemory(Process, &WriteInput->Value,
				WriteInput->Address, WriteInput->Size);

		/*
		if(counter % 20 == 0)
			DbgPrintEx(0, 0, "Write value / address:  %lu, %#010x \n", WriteInput->Value, WriteInput->Address);
		*/

		status = STATUS_SUCCESS;
		len = sizeof(KERNEL_WRITE_REQUEST);
		break;
	}
	case IOCTL_GET_ID_REQUEST: {
		// Thats how  a argument is passed to the usermode.
		PULONG outPut = (PULONG)(*Irp).AssociatedIrp.SystemBuffer;
		*outPut = csgoId;

		//DbgPrintEx(0, 0, "id get %#010x", csgoId);
		status = STATUS_SUCCESS;
		len = sizeof(*outPut);
		break;
	}
	case IOCTL_GET_MODULE_REQUEST: {
		PULONG outPut = (PULONG)(*Irp).AssociatedIrp.SystemBuffer;
		*outPut = (ULONG)clientAddress;

		//DbgPrintEx(0, 0, "Module get %p", clientAddress);
		status = STATUS_SUCCESS;
		len = sizeof(*outPut);
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	(*Irp).IoStatus.Status = status;
	(*Irp).IoStatus.Information = len;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	counter++;
	return status;
}

OB_PREOP_CALLBACK_STATUS PreOpenProcessOperation(PVOID RegistrationContext, POB_PRE_OPERATION_INFORMATION Info) {
	
	UNREFERENCED_PARAMETER(RegistrationContext);
	
	if ((*Info).KernelHandle)
		return OB_PREOP_SUCCESS;

	PEPROCESS process = (PEPROCESS)(*Info).Object;
	ULONG pid = PsGetProcessId(process);

	if (pid == protectedPid) {
		(*(*Info).Parameters).CreateHandleInformation.DesiredAccess &= ~PROCESS_DUP_HANDLE;
		(*(*Info).Parameters).CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_READ;
		(*(*Info).Parameters).CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_OPERATION;
	}

	return OB_PREOP_SUCCESS;
}

// set a callback for every PE image loaded to user memory
// then find the client.dll & csgo.exe using the callback
// Such a nice way to do this.
void ImageLoadCallback(PUNICODE_STRING FullImageName,
	HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	// Compare our string to input
	if (wcsstr((*FullImageName).Buffer, L"\\path\\to\\process.dll")) {
		// if it matches
		/*
		DbgPrintEx(0, 0, "Loaded Name: %ls \n", (*FullImageName).Buffer);
		DbgPrintEx(0, 0, "Pid: %d \n", (ULONG)ProcessId);
		DbgPrintEx(0, 0, "Client.dll: %p \n", (*ImageInfo).ImageBase);
		DbgPrintEx(0, 0, "Client.dll: %d \n", (ULONG)(*ImageInfo).ImageBase);
		*/
		clientAddress = (*ImageInfo).ImageBase;
		csgoId = (ULONG)ProcessId;
	}
}
