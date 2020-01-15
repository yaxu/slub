/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: lo_macros.h,v 1.2 2004/11/18 15:20:26 theno23 Exp $
 */

#ifndef LO_MACROS_H
#define LO_MACROS_H

/* macros that have to be defined after function signatures */

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum length of UDP messages in bytes */
#define LO_MAX_MSG_SIZE 32768

/* \brief A set of macros to represent different communications transports
 */
#define LO_UDP  0x1
#define LO_UNIX 0x2
#define LO_TCP  0x4

/* an internal value, ignored in transmission but check against LO_MARKER in the
 * argument list. Used to do primitive bounds checking */
#define LO_MARKER_A 0xdeadbeef
#define LO_MARKER_B 0xf00baa23

#define lo_send(targ, path, types...) \
        lo_send_internal(targ, __FILE__, __LINE__, path, types, \
			 LO_MARKER_A, LO_MARKER_B)

#define lo_send_timestamped(targ, ts, path, types...) \
        lo_send_timestamped_internal(targ, __FILE__, __LINE__, ts, path, \
		       	             types, LO_MARKER_A, LO_MARKER_B)

#if 0

This function is deliberatly not avialable, see send.c for details.

#define lo_sendf(targ, path, types...) \
        lo_sendf_internal(targ, __FILE__, __LINE__, path, types, \
			 LO_MARKER_A, LO_MARKER_B)
#endif

#ifdef __cplusplus
}
#endif

#endif
