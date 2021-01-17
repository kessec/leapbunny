/*
 * Copyright (C) 2000-2006 Erik Andersen <andersen@uclibc.org>
 *
 * Licensed under the LGPL v2.1, see the file COPYING.LIB in this tarball.
 */
#ifndef _BITS_UCLIBC_ERRNO_H
#define _BITS_UCLIBC_ERRNO_H 1

#ifdef IS_IN_rtld
# undef errno
# define errno _dl_errno
extern int _dl_errno; // attribute_hidden;
#endif /* IS_IN_rtld */

#define __set_errno(val) (errno = (val))

#ifndef __ASSEMBLER__
extern int *__errno_location (void) __THROW __attribute__ ((__const__))
# ifdef IS_IN_rtld
	attribute_hidden
# endif
;
#if __UCLIBC_HAS_THREADS_NATIVE__
libc_hidden_proto(__errno_location)
#endif

/* We now need a declaration of the `errno' variable.  */
# ifndef __UCLIBC_HAS_THREADS__
/*extern int errno;*/
libc_hidden_proto(errno)
# endif
#endif /* !__ASSEMBLER__ */

#endif
