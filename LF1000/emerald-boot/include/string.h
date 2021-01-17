/* string.h  -- implementations of some standard library string functions
 *
 * Copyright 2009-2010 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef LIGHTNING_BOOT_STRING_H
#define LIGHTNING_BOOT_STRING_H

unsigned int strlen(const char *s);
char * strcpy(char *dest, const char *src);
int    strcmp(const char *s1, const char *s2);
char * strcat(char *restrict s1, const char *restrict s2);
char * memcpy(char *dest, char *src, unsigned int n);

#if 0   // Not called in emerald-boot, but retaining just in case...
int    strncmp(const char *s1, const char *s2, unsigned int n);
char * strncat(char *restrict s1, const char *restrict s2, unsigned int n);
#endif

#endif
