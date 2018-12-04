#ifndef _DETOURBP_DETOUR_I_H
#define _DETOURBP_DETOUR_I_H

#include "detourbp.h"

DETOURBP_START_NAMESPACE

class Detour_i
{
	template<typename R, typename... A> friend class Detour;

public:
	BYTE* getSource()
	{
		return _src;
	}

	BYTE* getDest()
	{
		return _dst;
	}

	BYTE getArguments()
	{
		return _arguments;
	}

private:
	Detour_i(BYTE* src, BYTE* dst, BYTE arguments);

	BYTE MinLength();
	BYTE FillByType(BYTE* src, BYTE* dst);

	BYTE* CreateTrampoline();
	BYTE* CreateHook();
	bool Commit();
	bool Restore();

	bool _withTrampoline;

	BYTE* _src;
	BYTE* _dst;
	BYTE* _callee;

	BYTE _arguments;
	BYTE _type;
	BYTE _detourlen;
};

DETOURBP_END_NAMESPACE

#endif