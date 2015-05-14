/*
 * Copyright (C) 2002-2004 the xine project
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
 * $Id: mms.c,v 1.52.2.3 2005/04/21 19:45:45 mroi Exp $
 *
 * MMS over TCP protocol
 *   based on work from major mms
 *   utility functions to handle communication with an mms server
 *
 * TODO:
 *   error messages
 *   enable seeking !
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __GNUC__
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#if defined(HAVE_ICONV) && defined(HAVE_LANGINFO_CODESET)
#define USE_ICONV
#include <iconv.h>
#include <locale.h>
#include <langinfo.h>
#endif

/********** logging **********/
#define LOG_MODULE      "mms"
#define LOG_VERBOSE
/*
#define LOG 
*/
#include "xsysio.h"
#include "mms.h"


