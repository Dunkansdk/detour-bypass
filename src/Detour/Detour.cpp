#include "Detour\Detour.h"

DETOURBP_START_NAMESPACE

LONG WINAPI EHandler(EXCEPTION_POINTERS* /*ExceptionInfo*/);
bool EHApplied = false;
std::vector<Detour_i*> EHandlers;

Detour_i::Detour_i(BYTE* src, BYTE* dst, BYTE arguments) :
	_src(src),
	_dst(dst),
	_arguments(arguments),
	_withTrampoline(false),
	_detourlen(0),
	_type(DETOUR_JMP)
{}

BYTE Detour_i::MinLength()
{
	switch (_type)
	{
	case DETOUR_JMP: return 5;
	case DETOUR_RET: return 6;
	case DETOUR_MEM: return 0;
	default: return 0;
	}
}

BYTE Detour_i::FillByType(BYTE* src, BYTE* dst)
{
	switch (_type)
	{
	case DETOUR_JMP:
		*(BYTE*)(src + 0) = 0xE9;
		*(DWORD*)(src + 1) = (DWORD)(dst - src) - 5;
		break;

	case DETOUR_RET:
		*(BYTE*)(src + 0) = 0x68;
		*(DWORD*)(src + 1) = (DWORD)(dst);
		*(BYTE*)(src + 5) = 0xC3;
		break;

	default:
		break;
	}

	return MinLength();
}

BYTE* Detour_i::CreateTrampoline()
{
	BYTE retlen = MinLength();
	BYTE* trampoline = (BYTE*)malloc(4 + (_arguments * 4) + 5 + 2 + _detourlen + retlen);
	if (!trampoline)
		return false;

	// PUSH EBP
	// MOV EBP, ESP
	// PUSH EAX
	*(BYTE*)(trampoline + 0) = 0x55;
	*(WORD*)(trampoline + 1) = 0xEC8B;
	*(BYTE*)(trampoline + 3) = 0x50;
	trampoline += 4;

	BYTE offset = _arguments * 4 + 4; // +4 due push + mov above
	for (BYTE i = 0; i < _arguments; i++)
	{
		// MOV EAX, [EBP + (numarguments - i)*4]
		*(WORD*)(trampoline + 0) = 0x458B;
		*(BYTE*)(trampoline + 2) = offset;

		// PUSH EAX
		*(BYTE*)(trampoline + 3) = 0x50;

		trampoline += 4;
		offset -= 4;
	}

	// call dst
	*(BYTE*)(trampoline + 0) = 0xE8;
	*(DWORD*)(trampoline + 1) = (DWORD)(_dst - trampoline) - 5;
	trampoline += 5;

	// POP EAX
	// POP EBP
	*(BYTE*)(trampoline + 0) = 0x58;
	*(BYTE*)(trampoline + 1) = 0x5D;
	trampoline += 2;

	// src bytes (detourlen)
	memcpy(trampoline, _src, _detourlen);
	trampoline += _detourlen;

	// jmp src + len (retlen)
	FillByType(trampoline, _src + _detourlen);

	return (trampoline - _detourlen - 2 - 5 - (_arguments * 4) - 4);
}

BYTE* Detour_i::CreateHook()
{
	BYTE* jump = (BYTE*)malloc(_detourlen + 5);
	if (!jump)
		return NULL;

	memcpy(jump, _src, _detourlen);
	jump += _detourlen;

	// jmp back
	jump[0] = 0xE9;
	*(DWORD*)(jump + 1) = (DWORD)(_src + _detourlen - (DWORD)jump) - 5;

	return jump - _detourlen;
}

bool Detour_i::Commit()
{
	// MEM Detour!
	if (_type == DETOUR_MEM)
	{
		DWORD old;
		MEMORY_BASIC_INFORMATION meminfo;
		VirtualQuery(_src, &meminfo, sizeof(MEMORY_BASIC_INFORMATION));
		if (!VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, meminfo.AllocationProtect | PAGE_GUARD, &old))
		{
			NAMESPACE::SetLastError(DETOUR_VIRTUAL_PROTECT_ERROR);
			return false;
		}

		if (!EHApplied)
		{
			if (!AddVectoredExceptionHandler(1, EHandler) && !SetUnhandledExceptionFilter(EHandler))
			{
				NAMESPACE::SetLastError(DETOUR_VEH_ERROR);
				return false;
			}

			EHApplied = true;
		}

		EHandlers.push_back(this);
		return true;
	}

	// ASM Detour
	if (!_detourlen)
		_detourlen = MinLength();
	else if (_detourlen < MinLength())
	{
		NAMESPACE::SetLastError(DETOUR_LENGTH_ERROR);
		return false;
	}

	BYTE* hook = _withTrampoline ? CreateTrampoline() : CreateHook();
	if (!hook)
	{
		NAMESPACE::SetLastError(DETOUR_MALLOC_ERROR);
		return false;
	}

	DWORD old;
	VirtualProtect(_src, _detourlen, PAGE_READWRITE, &old);

	if (_withTrampoline)
	{
		memset(_src, 0x90, _detourlen);
		FillByType(_src, hook);
	}
	else
	{
		memset(_src, 0x90, _detourlen);
		FillByType(_src, _dst);
	}

	_callee = hook;

	VirtualProtect(_src, _detourlen, old, &old);

	return true;
}

bool Detour_i::Restore()
{
	switch (_type)
	{
	case DETOUR_MEM:
	{
		DWORD old;
		MEMORY_BASIC_INFORMATION meminfo;
		VirtualQuery(_src, &meminfo, sizeof(MEMORY_BASIC_INFORMATION));
		if (!VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, meminfo.AllocationProtect & ~PAGE_GUARD, &old))
		{
			NAMESPACE::SetLastError(DETOUR_RESTORE_VP_ERROR);
			return false;
		}

		EHandlers.erase(std::find(EHandlers.begin(), EHandlers.end(), this));
		return true;
	}

	case DETOUR_JMP:
	case DETOUR_RET:
	{
		DWORD old;
		VirtualProtect(_src, _detourlen, PAGE_READWRITE, &old);

		BYTE* dst = _callee;
		if (_withTrampoline)
		{
			dst += 4 + (_arguments * 4) + 5 + 2;
		}

		memcpy(_src, dst, _detourlen);

		VirtualProtect(_src, _detourlen, old, &old);

		return true;
	}
	}

	return false;
}

LONG WINAPI EHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) {
		std::vector<Detour_i*>::iterator it = EHandlers.begin();
		for (; it != EHandlers.end(); it++)
		{
			Detour_i* detour = *it;

			if ((DWORD)ExceptionInfo->ExceptionRecord->ExceptionAddress == (DWORD)detour->getSource())
			{
				DWORD offs = detour->getArguments() * 4 + 4;
				DWORD dest = (DWORD)detour->getDest();
				DWORD oEBP = 0;

				// Save EBP (just in case)
				__asm MOV oEBP, EBP

				// Push parameters
				for (int i = 0; i < detour->getArguments(); i++)
				{
					DWORD param = *(DWORD*)(ExceptionInfo->ContextRecord->Ebp + offs);
					offs -= 4;

					__asm PUSH param;
				}

				// Call dest
				__asm MOV EAX, dest
				__asm CALL EAX

				// Restore EBP
				__asm MOV EBP, oEBP
			}

			ExceptionInfo->ContextRecord->EFlags |= 0x100;
		}

		return EXCEPTION_CONTINUE_EXECUTION;
	}
	else if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		std::vector<Detour_i*>::iterator it = EHandlers.begin();
		for (; it != EHandlers.end(); it++)
		{
			Detour_i* detour = *it;
			DWORD old;
			MEMORY_BASIC_INFORMATION meminfo;
			VirtualQuery(detour->getSource(), &meminfo, sizeof(MEMORY_BASIC_INFORMATION));
			VirtualProtect(meminfo.BaseAddress, meminfo.RegionSize, meminfo.AllocationProtect | PAGE_GUARD, &old);
		}

		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

DETOURBP_END_NAMESPACE