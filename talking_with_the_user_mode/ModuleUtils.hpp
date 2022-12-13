// Creedits: https://github.com/Idov31/Nidhogg/blob/f87960f55e63a426b5239b24a8f51ae330e5213a/Nidhogg/ModuleUtils.hpp
#pragma once
#include "WindowsTypes.h"

PVOID GetModuleBase(PEPROCESS Process, WCHAR* moduleName);
NTSTATUS KeWriteProcessMemory(PVOID sourceDataAddress, PEPROCESS TargetProcess, PVOID targetAddress, SIZE_T dataSize, MODE mode);
NTSTATUS KeReadProcessMemory(PEPROCESS Process, PVOID sourceAddress, PVOID targetAddress, SIZE_T dataSize, MODE mode);

typedef NTSTATUS(NTAPI* tZwProtectVirtualMemory)(
	HANDLE ProcessHandle,
	PVOID* BaseAddress,
	SIZE_T* NumberOfBytesToProtect,
	ULONG NewAccessProtection,
	PULONG OldAccessProtection);

typedef NTSTATUS(NTAPI* tMmCopyVirtualMemory)(
	PEPROCESS SourceProcess,
	PVOID SourceAddress,
	PEPROCESS TargetProcess,
	PVOID TargetAddress,
	SIZE_T BufferSize,
	KPROCESSOR_MODE PreviousMode,
	PSIZE_T ReturnSize);

typedef PPEB(NTAPI* tPsGetProcessPeb)(
	PEPROCESS Process);

struct DynamicImportedModulesGlobal {
	tZwProtectVirtualMemory ZwProtectVirtualMemory;
	tMmCopyVirtualMemory	MmCopyVirtualMemory;
	tPsGetProcessPeb		PsGetProcessPeb;

	void Init() {
		UNICODE_STRING routineName;
		RtlInitUnicodeString(&routineName, L"ZwProtectVirtualMemory");
		ZwProtectVirtualMemory = (tZwProtectVirtualMemory)MmGetSystemRoutineAddress(&routineName);
		RtlInitUnicodeString(&routineName, L"MmCopyVirtualMemory");
		MmCopyVirtualMemory = (tMmCopyVirtualMemory)MmGetSystemRoutineAddress(&routineName);
		RtlInitUnicodeString(&routineName, L"PsGetProcessPeb");
		PsGetProcessPeb = (tPsGetProcessPeb)MmGetSystemRoutineAddress(&routineName);
	}
}; DynamicImportedModulesGlobal dimGlobals;

PVOID GetModuleBase(PEPROCESS Process, WCHAR* moduleName) {
	PVOID moduleBase = NULL;
	LARGE_INTEGER time = { 0 };
	time.QuadPart = -10011 * 10 * 1000; // ? 

	PREALPEB targetPeb = (PREALPEB)dimGlobals.PsGetProcessPeb(Process);

	if (!targetPeb) {
		DbgPrint("Failed to get PEB.\n");
		return moduleBase;
	}

	for (int i = 0; !(*targetPeb).LoaderData && i < 10; i++) {
		KeDelayExecutionThread(KernelMode, TRUE, &time); // ?
	}

	if (!(*targetPeb).LoaderData) {
		DbgPrint("Failed to get LDR.\n");
		return moduleBase;
	}

	// Getting module image base
	// Interesting for loop, iterating over list of modules of process ? Just a guess not sure yet.
	for (PLIST_ENTRY pListEntry = (*(*targetPeb).LoaderData).InLoadOrderModuleList.Flink;
		pListEntry != &(*(*targetPeb).LoaderData).InLoadOrderModuleList;
		pListEntry = (*pListEntry).Flink) {

		PLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		// TODO Check how this comparsion is done between Module Name and Table Entry
		if (_wcsnicmp((*pEntry).FullDllName.Buffer, moduleName, (*pEntry).FullDllName.Length / sizeof(wchar_t) - 4) == 0) {
			moduleBase = (*pEntry).DllBase;
			break;
		}
	}
	return moduleBase;
}

NTSTATUS KeReadProcessMemory(PEPROCESS Process, PVOID sourceAddress, PVOID targetAddress, SIZE_T dataSize, MODE mode){
	SIZE_T bytesRead;

	if (mode != KernelMode && mode != UserMode) {
		DbgPrint("Invalid mode.\n");
		return STATUS_UNSUCCESSFUL;
	}

	// Kernel mode address valid ?
	if (mode == KernelMode && !MmIsAddressValid(targetAddress)) {
		DbgPrint("Buffer Address invalid.\n");
		return STATUS_UNSUCCESSFUL;
	}

	return dimGlobals.MmCopyVirtualMemory(Process, sourceAddress, PsGetCurrentProcess(), targetAddress, dataSize, KernelMode, &bytesRead);
}

NTSTATUS KeWriteProcessMemory(PVOID sourceDataAddress, PEPROCESS TargetProcess, PVOID targetAddress, SIZE_T dataSize, MODE mode) {
	HANDLE hTargetProcess;
	ULONG oldProtection;
	SIZE_T patchLen;
	SIZE_T bytesWritten;
	NTSTATUS status = STATUS_SUCCESS;

	if (mode != KernelMode && mode != UserMode) {
		DbgPrint("Invalid mode.\n");
		return STATUS_UNSUCCESSFUL;
	}

	// Kernel address valid ?
	if (mode == KernelMode && !MmIsAddressValid(sourceDataAddress)) {
		status = STATUS_UNSUCCESSFUL;
		DbgPrint("Source address invalid.\n");
		return status;
	}

	// Write Permission
	status = ObOpenObjectByPointer(TargetProcess, OBJ_KERNEL_HANDLE, NULL, PROCESS_ALL_ACCESS, *PsProcessType, UserMode, &hTargetProcess);

	if (!NT_SUCCESS(status)) {
		DbgPrint("Failes to get process handle.\n");
		return status;
	}

	patchLen = dataSize;
	PVOID addressToProtect = targetAddress;
	status = dimGlobals.ZwProtectVirtualMemory(hTargetProcess, &addressToProtect, &patchLen, PAGE_READWRITE, &oldProtection);

	if (!NT_SUCCESS(status)) {
		DbgPrint("Failed to change protection, (0x%08X).\n", status);
		ZwClose(hTargetProcess);
		return status;
	}

	// Writing
	status = dimGlobals.MmCopyVirtualMemory(PsGetCurrentProcess(), sourceDataAddress, TargetProcess, targetAddress, dataSize, KernelMode, &bytesWritten);

	if(!NT_SUCCESS(status))
		DbgPrint("MmCopyVirtualMemeory failed status, (0x%08X).\n", status);

	// Restoring permissions and cleaning up
	if (ObOpenObjectByPointer(TargetProcess, OBJ_KERNEL_HANDLE, NULL, PROCESS_ALL_ACCESS, *PsProcessType, UserMode, &hTargetProcess) == STATUS_SUCCESS) {
		patchLen = dataSize;
		dimGlobals.ZwProtectVirtualMemory(hTargetProcess, &addressToProtect, &patchLen, oldProtection, &oldProtection);

	}

	ZwClose(hTargetProcess);
	return status;
}