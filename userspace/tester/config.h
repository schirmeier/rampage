#ifndef _CONFIG_H_
#define _CONFIG_H_ 1

// FIXME: This value is archdep
#define PAGE_SIZE 4096

/* set to output additional debugging information */
//#define DEBUG 1

/* set to corrupt memory while testing */
//#define CORRUPT 1

#if __WORDSIZE == 64
typedef u_int64_t def_int_t;
#else
typedef u_int32_t def_int_t;
#endif

#endif
