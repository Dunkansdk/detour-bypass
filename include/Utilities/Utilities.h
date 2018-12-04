#ifndef _DETOURBP_UTILITIES_H
#define _DETOURBP_UTILITIES_H

#include "detourbp.h"

#include <vector>
#include <algorithm>

DETOURBP_START_NAMESPACE

/// <summary>
/// Creates a console on the current process and redirects stdin, stdout, stderr
/// to that console
/// </summary>
/// <returns>true if succeeds, false otherwise</returns>
bool CreateConsole();

/// <summary>
/// Retrieves information of a module (DLL)
/// </summary>
/// <param name="module">Module (DLL) handle</param>
/// <returns>Struct containing module information, 0 if not found</returns>
MODULEINFO GetModuleInfo(HMODULE handle);

/// <summary>
/// Gets the address of a known pattern in the process memory
/// </summary>
/// <param name="haystack">Start address to search from</param>
/// <param name="hlen">Maximum length to search in</param>
/// <param name="needle">Byte array to search for</param>
/// <param name="mask">String contaning 'x' if the byte is known, or '?' if the byte is unknown. Must be equal length as `mask`</param>
/// <returns>Struct containing module information, 0 if not found</returns>
BYTE* FindPattern(BYTE* haystack, size_t hlen, BYTE* needle, const char* mask);

/// <sumary>
/// Unlinks a module (DLL) header from PEB
/// </sumary>
/// <param name="module">Module (DLL) to unlink from PEB</param>
void UnlinkModuleFromPEB(HMODULE module);

/// <sumary>
/// Links an unlinked module (DLL) header back to PEB
/// </sumary>
/// <param name="module">Module (DLL) to link to PEB</param>
void RelinkModuleToPEB(HMODULE module);

DETOURBP_END_NAMESPACE

#endif