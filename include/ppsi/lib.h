/*
 * Alessandro Rubini and Aurelio Colosimo for CERN, 2011 -- public domain
 */
#ifndef __PPSI_LIB_H__
#define __PPSI_LIB_H__
#include <stdint.h>
#include <string.h>

extern void pp_puts(const char *s);
extern int atoi(const char *s);

extern uint32_t __div64_32(uint64_t *n, uint32_t base);

#endif /* __PPSI_LIB_H__ */
