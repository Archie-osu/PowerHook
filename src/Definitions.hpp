#pragma once
#include <ntifs.h>
using FN_IdlePreselect = NTSTATUS(*)(PVOID, PVOID);

struct _PPM_IDLE_STATES
{
	UCHAR Pad[0x268];
	FN_IdlePreselect IdlePreselect;
};

struct _PROCESSOR_POWER_STATE
{
	_PPM_IDLE_STATES* IdleStates;
};

struct _KPRCB
{
	UCHAR Pad[0x8840];
	_PROCESSOR_POWER_STATE PowerState;
};