#pragma once
#include "grey.h"
#include "hs_internal.h"

#ifdef __GNUC__
#define _cdecl __attribute__((__cdecl__))
#endif

// don't forget free the return value!!!
char * __cdecl GetHsSerializeDatabaseBytes(const char **patterns,
                                           const unsigned int *flags,
                                           const unsigned int *ids,
                                           unsigned int num,
                                           size_t *size);
