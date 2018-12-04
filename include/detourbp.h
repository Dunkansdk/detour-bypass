#if !defined(_MSC_VER)
#error Support for non MSC compilers is not yet implemented
#else

#if (_MSC_VER < 1700) // < 2012
#error At least Visual Studio 2012 is required
#endif

#if (_MSC_VER < 1800) // < 2013
#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)
#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#define WARN(exp) (FILE_LINE_LINK "WARNING: " exp)
#pragma message WARN("Visual Studio 2013 is recommended")
#endif

// Define custom namespace macro
#ifndef DETOURBP_START_NAMESPACE
#define NAMESPACE DETOURBP
#define DETOURBP_START_NAMESPACE namespace DETOURBP {
#define DETOURBP_END_NAMESPACE }
#endif

// Disable warnings 
#pragma warning (disable: 4731)

// Check compiler bits mode
#if defined(__x86_64__) || defined(__LP64__) || defined(_WIN64)
#error No support for 64 bits yet
#define _64BITS_BUILD_
#endif

#endif

#ifndef _DETOURBP_MAIN_H
#define _DETOURBP_MAIN_H

#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <functional>
#include <typeinfo>

#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

DETOURBP_START_NAMESPACE

class Loader;
template<typename R, typename... A> class Detour;

template<typename T>
struct count_arg;

template<typename R, typename... Args>
struct count_arg<std::function<R(Args...)>>
{
	static const size_t value = sizeof...(Args);
};

DETOURBP_END_NAMESPACE


DETOURBP_START_NAMESPACE

enum ERROR_CODES
{
	// GENERAL
	ERROR_NONE = 0x0000,
	ERROR_CONSOLE_ALLOC = 0x0001,

	// LOADER
	LOADER_WAIT_WITHOUT_CALLBACK = 0x1001,
	LOADER_TIMEOUT = 0x1002,
	LOADER_LOAD_ERROR = 0x1003,

	// DETOUR
	DETOUR_VIRTUAL_PROTECT_ERROR = 0x2001,
	DETOUR_VEH_ERROR = 0x2002,
	DETOUR_LENGTH_ERROR = 0x2003,
	DETOUR_MALLOC_ERROR = 0x2004,
	DETOUR_RESTORE_VP_ERROR = 0x2005,

	// API
	API_NOT_READY = 0x3001,
	API_MALLOC_ERROR = 0x3002,
	API_AUTOFAKE_NOT_FOUND = 0x3003,

	// UTILITIES
	UTILITY_PEB_ALREADY_UNLIKED = 0x4001,
	UTILITY_PEB_NOT_UNLIKED = 0x4001,
};

/// <summary>
/// Sets the last error
/// </summary>
/// <param name="error">The error code as stated in ERROR_CODES.</param>
void SetLastError(WORD error);

/// <summary>
/// Gets the last error
/// </summary>
/// <returns>The error code as stated in ERROR_CODES.</returns>
WORD GetLastError();

DETOURBP_END_NAMESPACE

#endif