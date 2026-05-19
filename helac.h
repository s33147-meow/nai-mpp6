#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef char*		cstr;
typedef size_t		usz;

typedef int8_t		i8;
typedef int16_t		i16;
typedef int32_t		i32;
typedef int64_t		i64;

typedef uint8_t		u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef uint64_t	u64;

typedef float		f32;
typedef double		f64;

#define MakeOptional(type) \
	struct Opt_##type { type value; const char* error; void* error_data; }

#define Optional(type) \
	struct Opt_##type

struct Opt_void {
	const char* error;
    void* error_data;
};

struct Opt_ptr {
	void* value;
	const char* error;
    void* error_data;
};

#define Result(type, val) (struct Opt_##type) { .error = NULL, .error_data = NULL, .value = (val) }
#define Ok(type) (struct Opt_##type) { .error = NULL, .error_data = NULL }
#define None(type) (struct Opt_##type) { .error = "NONE", .error_data = NULL }
#define Err(type, err, ...) (struct Opt_##type) { .error = ERROR_##err }

#define unpack_ignore(source, what, output) \
	do { struct Opt_##source tmp = (what); if(!tmp.error) *(output) = tmp.value; else if(tmp.error_data) } while(0)

#define expect_assure(source, what) \
	do { struct Opt_##source tmp = (what); assure(!tmp.error, "An error occured: %s\n", (cstr)tmp.error); } while(0)

#define unpack_assure(source, what, output) \
	do { struct Opt_##source tmp = (what); if(!tmp.error) *(output) = tmp.value; else assure(!tmp.error, "An error occured: %s\n", (cstr)tmp.error); } while(0)

#define expect_propagate(source, as, what) \
	do { struct Opt_##source tmp = (what); if(tmp.error) return Err(as, tmp.error); } while(0)

#define unpack_propagate(source, as, what, output) \
	do { struct Opt_##source tmp = (what); if(!tmp.error) *(output) = tmp.value; else return Err(as, tmp.error); } while(0)

#define MakeError(name, ...) \
	extern const char* ERROR_##name; struct Err_##name { __VA_ARGS__ }

#define Error(name) \
	const char* ERROR_##name = #name

#define __helac_STRINGIZE(x) __helac_STRINGIZE2(x)
#define __helac_STRINGIZE2(x) #x

#ifndef RELEASE
#define assure(expression,...) \
	do { if (!(expression)) { printf("Assertion '" #expression "' failed (" __FILE__ ":" __helac_STRINGIZE(__LINE__) "): " __VA_ARGS__); abort(); } } while(0)
#else
#define assure(expression,...) {}
#endif

#define unreachable(...) \
	do { printf("Unreachable state reached: " __VA_ARGS__); abort(); } while(0)

#define min(a,b) \
	((a) < (b) ? (a) : (b))

#define max(a,b) \
	((a) > (b) ? (a) : (b))

#define todo(...)

MakeOptional(bool);
MakeOptional(usz);
MakeOptional(i8);
MakeOptional(i16);
MakeOptional(i32);
MakeOptional(i64);
MakeOptional(u8);
MakeOptional(u16);
MakeOptional(u32);
MakeOptional(u64);
MakeOptional(f32);
MakeOptional(f64);
MakeOptional(char);
MakeOptional(cstr);
