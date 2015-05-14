/*
 * Copyright (C) 2002-2003 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: real.h,v 1.5 2004/04/24 16:55:42 miguelfreitas Exp $
 *
 * special functions for real streams.
 * adopted from joschkas real tools.
 *
 */

#ifndef HAVE_REAL_H
#define HAVE_REAL_H


#ifdef __CYGWIN__
#define uint32_t unsigned int
#define uint16_t unsigned short int
#define uint8_t unsigned char
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "rmff.h"
#include "rtsp.h"

/*
 * calculates response and checksum of a given challenge
 * (RealChallenge1 in rtsp). See implementation for details.
 */
extern void
real_calc_response_and_checksum (char *response, char *chksum, char *challenge);

extern rmff_header_t *
real_parse_sdp(char *data, char **stream_rules, uint32_t bandwidth);

extern rmff_header_t *
real_setup_and_get_header(rtsp_t *session, uint32_t bandwidth);

extern int
real_setup_act_play_range(rtsp_t *session,
	 uint32_t start, uint32_t length);
extern rmff_header_t *
real_setup_and_play_range(rtsp_t *session,
	 uint32_t bandwidth, uint32_t start, uint32_t length);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef HAVE_REAL_H */


