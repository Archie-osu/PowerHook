#include "Definitions.hpp"

static FN_IdlePreselect g_OriginalRoutine = nullptr;
static _KPRCB* g_ModifiedPrcb = nullptr;

void SetPowerInterceptRoutine(
	IN _KPRCB* Prcb,
	IN FN_IdlePreselect PowerInterceptRoutine,
	OPTIONAL OUT FN_IdlePreselect* OriginalRoutine
)
{
	// If the caller wants to know the original routine, save it first.
	// Hooks may want to call the original routine inside their handler.
	// 
	// If the currently running code were to be switched from the
	// processor whose Prcb is being modified, and the processor
	// became idle, not setting this first could potentially crash the OS.

	if (OriginalRoutine)
		*OriginalRoutine = Prcb->PowerState.IdleStates->IdlePreselect;

	Prcb->PowerState.IdleStates->IdlePreselect = PowerInterceptRoutine;
}

NTSTATUS HookRoutine(
	IN PVOID Rcx,
	IN PVOID Rdx
)
{
	//DbgPrintEx(
	//	0, 0, 
	//	"[HookRoutine] TID: %lu, PID: %lu, IRQL: %lu\n", 
	//	HandleToULong(PsGetCurrentThreadId()),
	//	HandleToULong(PsGetCurrentProcessId()),
	//	KeGetCurrentIrql()
	//);

	auto current_thread_id = PsGetCurrentThreadId();
	auto current_process_id = PsGetCurrentProcessId();

	// Try to get the thread
	{
		PETHREAD thread_object = nullptr;
		auto last_status = PsLookupThreadByThreadId(
			current_thread_id,
			&thread_object
		);

		DbgPrintEx(
			0, 0, 
			"TID %p => thread object at %p, last_status = %06X\n",
			current_thread_id, 
			thread_object,
			last_status
		);

		if (thread_object)
			ObDereferenceObject(thread_object);
	}
	
	// Try to get the process
	{
		PEPROCESS process_object = nullptr;
		auto last_status = PsLookupProcessByProcessId(
			current_process_id,
			&process_object
		);

		DbgPrintEx(
			0, 0, 
			"PID %p => process object at %p, last_status = %06X\n",
			current_process_id, 
			process_object, 
			last_status
		);

	}

	return g_OriginalRoutine(Rcx, Rdx);
}

void DriverUnload(
	IN PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);

	// Revert our change to the PRCB.
	// We don't need to know the original value, as it will most likely just be our hook.
	SetPowerInterceptRoutine(g_ModifiedPrcb, g_OriginalRoutine, nullptr);
	return;
}

EXTERN_C NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	DriverObject->DriverUnload = DriverUnload;
	
	// Save the modified PRCB, as we could be re-scheduled to 
	// a different processor at any time. This routine
	// runs at PASSIVE_LEVEL, so context switches are entirely possible.
	g_ModifiedPrcb = KeGetPcr()->CurrentPrcb;
	
	SetPowerInterceptRoutine(
		g_ModifiedPrcb,
		HookRoutine,
		&g_OriginalRoutine
	);

	return STATUS_SUCCESS;
}