/*	$OpenBSD: endian.h,v 1.18 2006/03/27 07:09:24 otto Exp $	*/

/*-
 * Copyright (c) 1997 Niklas Hallqvist.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//
// This file is a cut-down version of OpenBSD's sys/types.h.
//

/*
 * Generic definitions for little- and big-endian systems.  Other endianesses
 * has to be dealt with in the specific machine/endian.h file for that port.
 *
 * This file is meant to be included from a little- or big-endian port's
 * machine/endian.h after setting _BYTE_ORDER to either 1234 for little endian
 * or 4321 for big..
 */

#ifndef _CON_PLATFORM_ENDIAN_H_
#define _CON_PLATFORM_ENDIAN_H_

#define __swap16gen(x)							\
    (uint16_t)(((uint16_t)(x) & 0xffU) << 8 | ((uint16_t)(x) & 0xff00U) >> 8)

#define __swap32gen(x)							\
    (uint32_t)(((uint32_t)(x) & 0xff) << 24 |			\
    ((uint32_t)(x) & 0xff00) << 8 | ((uint32_t)(x) & 0xff0000) >> 8 |\
    ((uint32_t)(x) & 0xff000000) >> 24)

#define __swap64gen(x)							\
	(uint64_t)((((uint64_t)(x) & 0xff) << 56) |			\
	    ((uint64_t)(x) & 0xff00ULL) << 40 |			\
	    ((uint64_t)(x) & 0xff0000ULL) << 24 |			\
	    ((uint64_t)(x) & 0xff000000ULL) << 8 |			\
	    ((uint64_t)(x) & 0xff00000000ULL) >> 8 |			\
	    ((uint64_t)(x) & 0xff0000000000ULL) >> 24 |		\
	    ((uint64_t)(x) & 0xff000000000000ULL) >> 40 |		\
	    ((uint64_t)(x) & 0xff00000000000000ULL) >> 56)

#define __swap16 __swap16gen
#define __swap32 __swap32gen
#define __swap64 __swap64gen

#ifdef CON_LITTLE_ENDIAN

#define htobe16 __swap16
#define htobe32 __swap32
#define htobe64 __swap64
#define betoh16 __swap16
#define betoh32 __swap32
#define betoh64 __swap64

#define htole16(x) (x)
#define htole32(x) (x)
#define htole64(x) (x)
#define letoh16(x) (x)
#define letoh32(x) (x)
#define letoh64(x) (x)

#define htons(x) __swap16(x)
#define htonl(x) __swap32(x)
#define ntohs(x) __swap16(x)
#define ntohl(x) __swap32(x)

#endif /* _BYTE_ORDER */

#ifndef CON_LITTLE_ENDIAN

#define htole16 __swap16
#define htole32 __swap32
#define htole64 __swap64
#define letoh16 __swap16
#define letoh32 __swap32
#define letoh64 __swap64

#define htobe16(x) (x)
#define htobe32(x) (x)
#define htobe64(x) (x)
#define betoh16(x) (x)
#define betoh32(x) (x)
#define betoh64(x) (x)

#define htons(x) (x)
#define htonl(x) (x)
#define ntohs(x) (x)
#define ntohl(x) (x)

#endif /* _BYTE_ORDER */

#endif /* _CON_PLATFORM_ENDIAN_H_ */
