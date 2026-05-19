#pragma once

// ============ Config ===============

#ifndef LOG_TRACE
#define LOG_TRACE 	1
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG 	1
#endif

#ifndef LOG_INFO
#define LOG_INFO 	1
#endif

#ifndef LOG_WARNING
#define LOG_WARNING	1
#endif


// ===================================
// todo: linux or other os-es
#if __linux || _WIN32
#include <stdio.h>
#define _log(...)	fprintf(stderr, __VA_ARGS__)


#else
#define log(...)
#endif

#if LOG_TRACE
#define tlog(...)	_log("TRC: "__VA_ARGS__)
#else
#define tlog(...)
#endif

#if LOG_DEBUG
#define dlog(...)	_log("DBG: "__VA_ARGS__)
#else
#define dlog(...)
#endif

#if LOG_INFO
#define ilog(...)	_log("INF: "__VA_ARGS__)
#else
#define ilog(...)
#endif

#if LOG_WARNING
#define wlog(...)	_log("WRN: "__VA_ARGS__)
#else
#define wlog(...)
#endif
