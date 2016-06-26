#ifndef __POSIX_H_
#define __POSIX_H_
  
#include <sdl_rwops.h>
  
/*
 * include header file needed for this platform
 */
#ifdef _WIN32
	#include <Windows.h>
	#include <crtdefs.h> // intptr_t
	//
	// warning C4200: nonstandard extension used : zero-sized array in struct/union
    //    Cannot generate copy-ctor or copy-assignment operator when UDT contains a zero-sized array
	// driver: this warning must be disabled, or five functions cann't be linked 
	// application: I think using zero-sized is useful, so disable this warning 
	#pragma warning( disable: 4200 )
#elif defined(ANDROID)
	#include <android/log.h>
#endif
#include <stdint.h>

/*
 *  Things that must be sorted by compiler and then by architecture
 */

// define _MAX_PATH, linux isn't define it
#ifndef _MAX_PATH
#define _MAX_PATH		255
#endif

// file extended name maximal length
#ifndef _MAX_EXTNAME
#define _MAX_EXTNAME	7
#endif

// mmsystem.h has this definition
#if defined(_WIN32) 
	// in mmsystem.h, when define FOURCC and mmioFOURCC, it doesn't dectcte whether this macro is defined.
#else
	typedef uint32_t  FOURCC;

	#ifndef mmioFOURCC
		#define mmioFOURCC(ch0, ch1, ch2, ch3)  \
		((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
	#endif
#endif

#ifndef GENERIC_READ
#define GENERIC_READ        (0x80000000L)
#define GENERIC_WRITE       (0x40000000L)

#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#endif

typedef SDL_RWops*		posix_file_t;
#define INVALID_FILE	NULL

#define posix_fopen(name, desired_access, create_disposition, file)	do { \
    char __mode[5];	\
	int __mode_pos = 0;	\
    if (create_disposition == CREATE_ALWAYS) {  \
        __mode[__mode_pos ++] = 'w';	\
        __mode[__mode_pos ++] = '+';	\
    } else {	\
		__mode[__mode_pos ++] = 'r';	\
        if (desired_access & GENERIC_WRITE) {    \
            __mode[__mode_pos ++] = '+';	\
        }   \
	}	\
	__mode[__mode_pos ++] = 'b';	\
	__mode[__mode_pos] = 0;	\
	file = SDL_RWFromFile(name, __mode);	\
} while(0)

#define posix_fseek(file, offset)	\
	SDL_RWseek(file, offset, RW_SEEK_SET)	

#define posix_fwrite(file, ptr, size)	\
	SDL_RWwrite(file, ptr, 1, size)

#define posix_fread(file, ptr, size)	\
	SDL_RWread(file, ptr, 1, size)

#define posix_fclose(file)			\
	SDL_RWclose(file)

#define posix_fsize(file)	\
	SDL_RWsize(file)

#define posix_fsize_byname(name, fsize) do { \
    SDL_RWops* __h = SDL_RWFromFile(name, "rb");  \
	if (!__h) {  \
		fsize = 0;		\
	} else {		\
		posix_fseek(__h, 0);	\
		fsize = posix_fsize(__h);	\
		posix_fclose(__h);	\
    } \
} while(0)

/*
 * string 
 */
#ifdef _WIN32
	#ifndef strncasecmp
	#define strncasecmp(s1, s2, c)		_strnicmp(s1, s2, c)
	#endif

	#ifndef strcasecmp
	#define strcasecmp(s1, s2)			_stricmp(s1, s2)
	#endif
#endif

// 1.mask must be integer of like xxx1000
// 2.if val is 0, align_ceil will return 0. even if mask is any value.
// 3.if mask is power's 2, use align_ceil, or use align_ceil2.
#define posix_align_ceil(val, mask)     (((val) + (mask) - 1) & ~((mask) - 1)) 
int posix_align_ceil2(int dividend, int divisor);

// assistant macro of byte,word,dword,qword opration. windef.h has this definition.
// for make macro, first parameter is low part, second parameter is high part.
#ifndef posix_mku64
	#define posix_mku64(l, h)		((uint64_t)(((uint32_t)(l)) | ((uint64_t)((uint32_t)(h))) << 32))    
#endif

#ifndef posix_mku32
	#define posix_mku32(l, h)		((uint32_t)(((uint16_t)(l)) | ((uint32_t)((uint16_t)(h))) << 16))
#endif

#ifndef posix_mku16
	#define posix_mku16(l, h)		((uint16_t)(((uint8_t)(l)) | ((uint16_t)((uint8_t)(h))) << 8))
#endif

#ifndef posix_mki64
	#define posix_mki64(l, h)		((int64_t)(((uint32_t)(l)) | ((int64_t)((uint32_t)(h))) << 32))    
#endif

#ifndef posix_mki32
	#define posix_mki32(l, h)		((int32_t)(((uint16_t)(l)) | ((int32_t)((uint16_t)(h))) << 16))
#endif

#ifndef posix_mki16
	#define posix_mki16(l, h)		((int16_t)(((uint8_t)(l)) | ((int16_t)((uint8_t)(h))) << 8))
#endif

#ifndef posix_lo32
	#define posix_lo32(v64)			((uint32_t)(v64))
#endif

#ifndef posix_hi32
	#define posix_hi32(v64)			((uint32_t)(((uint64_t)(v64) >> 32) & 0xFFFFFFFF))
#endif

#ifndef posix_lo16
	#define posix_lo16(v32)			((uint16_t)(v32))
#endif

#ifndef posix_hi16
	#define posix_hi16(v32)			((uint16_t)(((uint32_t)(v32) >> 16) & 0xFFFF))
#endif

#ifndef posix_lo8
	#define posix_lo8(v16)			((uint8_t)(v16))
#endif

#ifndef posix_hi8
	#define posix_hi8(v16)			((uint8_t)(((uint16_t)(v16) >> 8) & 0xFF))
#endif


#ifndef posix_max
#define posix_max(a,b)            (((a) > (b))? (a) : (b))
#endif

#ifndef posix_min
#define posix_min(a,b)            (((a) < (b))? (a) : (b))
#endif

#ifndef posix_clip
#define	posix_clip(x, min, max)	  posix_max(posix_min((x), (max)), (min))
#endif

#ifndef posix_abs
#define posix_abs(a)            (((a) >= 0)? (a) : (-(a)))
#endif

/*
 * some macro that assist struct data type 
 */
#if defined(_WIN32) && defined(_DRIVER) /* && defined(_NTDEF_) */
    // in ntdef.h, when define FIELD_OFFSET and CONTAINING_RECORD, it doesn't dectcte whether this macro is defined.
#else
    #ifndef FIELD_OFFSET
    #    define FIELD_OFFSET(type, member)    ((int32_t)&(((type *)0)->member))
    #endif
	
    #ifndef CONTAINING_RECORD
    #    define CONTAINING_RECORD(ptr, type, member) ((type *)((uint8_t*)(ptr) - (uint32_t)(&((type *)0)->member)))
    #endif
#endif


#ifndef container_of
#define container_of(ptr, type, member)  CONTAINING_RECORD(ptr, type, member)
#endif

/*
 * print debug information 
 */

#ifdef _WIN32
	void posix_print(const char* formt, ...);
	void posix_print_mb(const char* formt, ...);

#elif defined(__APPLE__) // mac os x/ios
	#define posix_print(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
	#define posix_print_mb(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

#elif defined(ANDROID) // android
	#define posix_print(format, ...) __android_log_print(ANDROID_LOG_INFO, "rose", format, ##__VA_ARGS__)
	#define posix_print_mb(format, ...) __android_log_print(ANDROID_LOG_INFO, "rose", format, ##__VA_ARGS__)

#else
	#define posix_print(format, arg...) printf(format, ## arg)
	#define posix_print_mb(format, arg...) printf(format, ## arg)
#endif

/*
 * some program uses MSB of uint32_t to indicate special function
 */ 
#ifndef BIT32_MSB_MASK
#define		BIT32_MSB_MASK			0x80000000
#endif

#ifndef BIT32_NONMSB_MASK
#define		BIT32_NONMSB_MASK		0x7fffffff
#endif

/*
 * reference to limits.h
 */
#ifndef INT16_MIN
#define INT16_MIN   (-32768)        /* minimum (signed) short value */
#endif

#ifndef INT16_MAX
#define INT16_MAX      32767         /* maximum (signed) short value */
#endif

#ifndef UINT16_MAX
#define UINT16_MAX     0xffff        /* maximum unsigned short value */
#endif

#ifndef INT32_MIN
#define INT32_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#endif

#ifndef INT32_MAX
#define INT32_MAX       2147483647    /* maximum (signed) int value */
#endif

#ifndef UINT32_MAX
#define UINT32_MAX      0xffffffff    /* maximum unsigned int value */
#endif 

/*
 * pseud float point type
 */
union pseud_float {
	struct {
		uint32_t	decimal_part;
		uint32_t	integer_part;
	} u32;
	uint64_t		u64;
};

#if defined(_MSC_VER) 
	/* microsoft complier */
	#define	CONSTANT_4G					4294967296I64
	#define APPROXIMATELY_CARRY_VAL		0xFFFFFFFFFD000000I64 // if decimal_part is not less than (>=) 0.99, than think it has carry. 
	#define APPROXIMATELY_EQUAL_VAL		0xFFFFFFFFFF000000I64 // if MSB 8 bit is equal between tow value, than think deciaml part of these is equal.
#else 
	/* else, GCC */
	#define	CONSTANT_4G					4294967296ull
	#define APPROXIMATELY_CARRY_VAL		0xFFFFFFFFFD000000ull // if decimal_part is not less than (>=) 0.99, than think it has carry. 
	#define APPROXIMATELY_EQUAL_VAL		0xFFFFFFFFFF000000ull // if MSB 8 bit is equal between tow value, than think deciaml part of these is equal.
#endif

extern void pseud_float_evaluate(union pseud_float *p, uint64_t dividend, uint64_t divisor); 

extern int pseud_float_cmp(union pseud_float *p1, union pseud_float *p2);

#define pseud_float_add(p1, p2) \
    (p1)->u64 = (p1)->u64 + (p2)->u64

#define pseud_float_sub(p1, p2) \
    (p1)->u64 = (p1)->u64 - (p2)->u64

#define CONSTANT_1G			1073741824
#define CONSTANT_768M		805306368
#define CONSTANT_512M		536870912
#define CONSTANT_384M		402653184
#define CONSTANT_256M		268435456
#define CONSTANT_128M		134217728
#define CONSTANT_64M		67108864
#define CONSTANT_32M		33554432
#define CONSTANT_16M		16777216
#define CONSTANT_8M			8388608
#define CONSTANT_4M			4194304
#define CONSTANT_3M			3145728
#define CONSTANT_2M			2097152
#define CONSTANT_1M			1048576
#define CONSTANT_768K		786432
#define CONSTANT_512K		524288
#define CONSTANT_384K		393216
#define CONSTANT_256K		262144
#define CONSTANT_192K		196608
#define CONSTANT_128K		131072
#define CONSTANT_64K		65536
#define CONSTANT_32K		32768
#define CONSTANT_16K		16384
#define CONSTANT_8K			8192
#define CONSTANT_4K			4096
#define CONSTANT_2K			2048
#define CONSTANT_1K			1024

#ifndef _PUT_16
#define _PUT_16
#define PUT_16(p,v) ((p)[0]=((v)>>8)&0xff,(p)[1]=(v)&0xff)
#define PUT_32(p,v) ((p)[0]=((v)>>24)&0xff,(p)[1]=((v)>>16)&0xff,(p)[2]=((v)>>8)&0xff,(p)[3]=(v)&0xff)
// #define PUT_64(p,v) ((p)[0]=((v)>>56)&0xff,(p)[1]=((v)>>48)&0xff,(p)[2]=((v)>>40)&0xff,(p)[3]=((v)>>32)&0xff,(p)[4]=((v)>>24)&0xff,(p)[5]=((v)>>16)&0xff,(p)[6]=((v)>>8)&0xff,(p)[7]=(v)&0xff)
#define PUT_64(p,v) ((p)[0]=(char)((v)>>56)&0xff,(p)[1]=(char)((v)>>48)&0xff,(p)[2]=(char)((v)>>40)&0xff,(p)[3]=(char)((v)>>32)&0xff,(p)[4]=(char)((v)>>24)&0xff,(p)[5]=(char)((v)>>16)&0xff,(p)[6]=(char)((v)>>8)&0xff,(p)[7]=(char)(v)&0xff)
#define GET_16(p) (((p)[0]<<8)|(p)[1])
#define GET_32(p) (((p)[0]<<24)|((p)[1]<<16)|((p)[2]<<8)|(p)[3])
// #define GET_64(p) (((p)[0]<<56)|((p)[1]<<48)|((p)[2]<<40)|((p)[3]<<32)|((p)[4]<<24)|((p)[5]<<16)|((p)[6]<<8)|(p)[7])
#endif

#define posix_swap64(ll) (((ll) >> 56) |	\
	(((ll) & 0x00ff000000000000) >> 40) |	\
	(((ll) & 0x0000ff0000000000) >> 24) |	\
	(((ll) & 0x000000ff00000000) >> 8)    |	\
	(((ll) & 0x00000000ff000000) << 8)    |	\
	(((ll) & 0x0000000000ff0000) << 24) |	\
	(((ll) & 0x000000000000ff00) << 40) |	\
	(((ll) << 56)))
#define posix_swap32(a) ((((a) & 0xff) << 24)  | (((a) & 0xff00) << 8) | (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))
#define posix_swap16(a) ((((a) << 8) & 0xff00) | (((a) >> 8) & 0xff))

#define posix_swapmem64(ptr)	\
	do {	\
		uint8_t val;	\
		val = (ptr)[0];	\
		(ptr)[0] = (ptr)[7];	\
		(ptr)[7] = val;	\
		val = (ptr)[1];	\
		(ptr)[1] = (ptr)[6];	\
		(ptr)[6] = val;	\
		val = ptr[2];	\
		(ptr)[2] = (ptr)[5];	\
		ptr[5] = val;	\
		val = (ptr)[3];	\
		(ptr)[3] = (ptr)[4];	\
		(ptr)[4] = val;	\
	} while (0)

// @w: width of bmp. for example 79 x 100 x 24, w is 79
// @bpp: bit per pixel. for example 79 x 100 x 24, bpp is 24
// return value: align with 4, require bytes when save. for example 79 x 100 x 24, value is 240
#define bmp_wbpp2bpl(w, bpp)		(((((w)*(bpp)) + 31) >> 5) << 2)

#endif // __POSIX_H_
