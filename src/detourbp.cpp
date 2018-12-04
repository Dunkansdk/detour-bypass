#include "detourbp.h"

DETOURBP_START_NAMESPACE

WORD _error = ERROR_NONE;

void SetLastError(WORD error)
{
	_error = error;
}
WORD GetLastError()
{
	return _error;
}

DETOURBP_END_NAMESPACE