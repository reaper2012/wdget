/*
 * Copyright (C) 2000-2003 the xine project
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
 * $Id: rtsp.c,v 1.18.2.1 2005/04/20 17:21:09 mroi Exp $
 *
 * a minimalistic implementation of rtsp protocol,
 * *not* RFC 2326 compilant yet.
 */

#ifdef __GNUC__
# include <unistd.h>
#endif
#ifdef WIN32
#  include <winsock.h>
#else
#  include <sys/time.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_MODULE "rtsp"
#define LOG_VERBOSE
/*
#define LOG
*/ 
#include "xsysio.h"

#include "rtsp.h"
#include "rmff.h"


#define BUF_SIZE 4096
#define HEADER_SIZE 1024
#define MAX_FIELDS 256

struct rtsp_s
{
  int           s;
  int           refers;
  
  int           stail;
  int           stamp;
  
  char         *host;
  int           port;
  char         *path;
  char         *mrl;
  char         *user_agent;

  char         *server;
  unsigned int  server_state;
  uint32_t      server_caps;
  
  unsigned int  cseq;
  char         *session;

  char        *answers[MAX_FIELDS];   /* data of last message */
  char        *scheduled[MAX_FIELDS]; /* will be sent with next message */
};

/*
 * constants
 */

const char rtsp_protocol_version[]="RTSP/1.0";

/* server states */
#define RTSP_CONNECTED 1
#define RTSP_INIT      2
#define RTSP_READY     4
#define RTSP_PLAYING   8
#define RTSP_RECORDING 16

/* server capabilities */
#define RTSP_OPTIONS       0x001
#define RTSP_DESCRIBE      0x002
#define RTSP_ANNOUNCE      0x004
#define RTSP_SETUP         0x008
#define RTSP_GET_PARAMETER 0x010
#define RTSP_SET_PARAMETER 0x020
#define RTSP_TEARDOWN      0x040
#define RTSP_PLAY          0x080
#define RTSP_RECORD        0x100

/**
 * rtsp_get gets a line from stream <BR>
 * and returns a null terminated string (must be freed).
 */
static char * rtsp_get(rtsp_t *s)
{
  char *buffer = (char *)malloc(BUF_SIZE);
  char *string = NULL;
  
  if (xio_tcp_read_line(s->s, buffer, BUF_SIZE) >= 0)
  {
//    lprintf("<< '%s'\n", buffer);
//	fprintf(stderr, "<< %s\r\n", buffer);
    string = strdup( buffer );
  }
  
  free(buffer);
  return string;
}

/**
 * rtsp_put puts a line on stream
 */
static void rtsp_put(rtsp_t *s, const char *string)
{

  int len = strlen(string);
  char *buf = (char *)malloc(sizeof(char)*len+2);
  
//  lprintf(">> '%s'", string);
//  fprintf(stderr, ">> %s\r\n", string);
  
  memcpy(buf,string,len);
  buf[len]=0x0d;
  buf[len+1]=0x0a;

  xio_tcp_write(s->s, buf, len+2);
  
//  lprintf("done.\n");

  free(buf);
}

/**
 * extract server status code
 */
static int rtsp_get_code(rtsp_t *s, const char *string)
{
  char buf[4];
  int code=0;
  
  if (!strncmp(string, rtsp_protocol_version, strlen(rtsp_protocol_version)))
  {
    memcpy(buf, string+strlen(rtsp_protocol_version)+1, 3);
    buf[3]=0;
    code=atoi(buf);
  } else if (!strncmp(string, "SET_PARAMETER",8))
  {
    return RTSP_STATUS_SET_PARAMETER;
  }
  
  if (code != 200) 
    xprintf(XVERBOSITY_DEBUG, "librtsp: server responds: '%s'\n", string);
  
  return code;
}

/**
 * send a request
 */
static void rtsp_send_request(rtsp_t *s, const char *type, const char *what)
{
  char **payload=s->scheduled;
  char *buf;
  
  buf = (char *)malloc(strlen(type)+strlen(what)+strlen(rtsp_protocol_version)+3);
  
  sprintf(buf,"%s %s %s",type, what, rtsp_protocol_version);
  rtsp_put(s,buf);
  free(buf);
  if (payload)
    while (*payload) {
      rtsp_put(s,*payload);
      payload++;
    }
  rtsp_put(s,"");
  rtsp_unschedule_all(s);
}

/**
 * schedule standard fields
 */
static void rtsp_schedule_standard(rtsp_t *s)
{
  char tmp[17];
  
  sprintf(tmp, "Cseq: %u", s->cseq);
  rtsp_schedule_field(s, tmp);
  
  if (s->session) {
    char *buf = (char *)malloc(strlen(s->session)+15);
    
    sprintf(buf, "Session: %s", s->session);
    rtsp_schedule_field(s, buf);
    free(buf);
  }
}

/**
 * get the answers, if server responses with something != 200, return NULL
 */
static int rtsp_get_answers(rtsp_t *s)
{
  char *answer=NULL;
  unsigned int answer_seq;
  char **answer_ptr=s->answers;
  int code;
  int ans_count = 0;
  
  answer=rtsp_get(s);
  if (!answer)
    return 0;
  code = rtsp_get_code(s, answer);
  free(answer);

  rtsp_free_answers(s);
  
  do { /* while we get answer lines */
  
    answer=rtsp_get(s);
    if (!answer)
      return 0;
    
    if (!strncasecmp(answer,"Cseq:",5)) {
      sscanf(answer,"%*s %u",&answer_seq);
      if (s->cseq != answer_seq) {
        lprintf("warning: Cseq mismatch. got %u, assumed %u", answer_seq, s->cseq);

        s->cseq=answer_seq;
      }
    }
    if (!strncasecmp(answer,"Server:",7)) {
      char *buf = (char *)xutil_malloc(strlen(answer));
      sscanf(answer,"%*s %s",buf);
      if (s->server) free(s->server);
      s->server=strdup(buf);
      free(buf);
    }
    if (!strncasecmp(answer,"Session:",8)) {
      char *buf = (char *)xutil_malloc(strlen(answer));
      sscanf(answer,"%*s %s",buf);
      if (s->session) {
        if (strcmp(buf, s->session)) {
          xprintf(XVERBOSITY_DEBUG, 
		  "rtsp: warning: setting NEW session: %s\n", buf);
          free(s->session);
          s->session=strdup(buf);
        }
      } else
      {
        lprintf("setting session id to: %s\n", buf);

        s->session=strdup(buf);
      }
      free(buf);
    }
    *answer_ptr=answer;
    answer_ptr++;
  } while ((strlen(answer)!=0) && (++ans_count < MAX_FIELDS));
  
  s->cseq++;
  
  *answer_ptr=NULL;
  rtsp_schedule_standard(s);
  
  return code;
}

/**
 * send an ok message
 */
extern int rtsp_send_ok(rtsp_t *s)
{
  char cseq[16];
  
  rtsp_put(s, "RTSP/1.0 200 OK");
  sprintf(cseq,"CSeq: %u", s->cseq);
  rtsp_put(s, cseq);
  rtsp_put(s, "");
  return 0;
}

/**
 * implementation of must-have rtsp requests; functions return
 * server status code.
 */
extern int rtsp_request_options(rtsp_t *s, const char *what)
{
  char *buf;

  if (what)
  {
    buf=strdup(what);
  }
  else
  {
    buf = (char *)malloc(sizeof(char)*(strlen(s->host)+16));
    sprintf(buf,"rtsp://%s:%i", s->host, s->port);
  }
  rtsp_send_request(s,"OPTIONS",buf);
  free(buf);

  return rtsp_get_answers(s);
}

extern int rtsp_request_describe(rtsp_t *s, const char *what)
{
  char *buf;

  if (what)
  {
    buf=strdup(what);
  }
  else
  {
    buf = (char *)malloc(sizeof(char)*(strlen(s->host)+strlen(s->path)+16));
    sprintf(buf,"rtsp://%s:%i/%s", s->host, s->port, s->path);
  }
  rtsp_send_request(s,"DESCRIBE",buf);
  free(buf);
  
  return rtsp_get_answers(s);
}

extern int rtsp_request_setup(rtsp_t *s, const char *what)
{
  rtsp_send_request(s,"SETUP",what);
  
  return rtsp_get_answers(s);
}

extern int rtsp_request_setparameter(rtsp_t *s, const char *what)
{
  char *buf;

  if (what)
  {
    buf=strdup(what);
  }
  else
  {
    buf = (char *)malloc(sizeof(char)*(strlen(s->host)+strlen(s->path)+16));
    sprintf(buf,"rtsp://%s:%i/%s", s->host, s->port, s->path);
  }
  rtsp_send_request(s,"SET_PARAMETER",buf);
  free(buf);
  
  return rtsp_get_answers(s);
}

extern int rtsp_request_play(rtsp_t *s, const char *what)
{
  char *buf;
  
  if (what)
  {
    buf=strdup(what);
  }
  else
  {
    buf = (char *)malloc(sizeof(char)*(strlen(s->host)+strlen(s->path)+16));
    sprintf(buf,"rtsp://%s:%i/%s", s->host, s->port, s->path);
  }
  
  s->stail = -1;
  rtsp_send_request(s,"PLAY",buf);
  free(buf);
  
  return rtsp_get_answers(s);
}

extern int rtsp_request_tearoff(rtsp_t *s, const char *what)
{
  rtsp_send_request(s,"TEAROFF",what);
  
  return rtsp_get_answers(s);
}

/** read opaque data from stream */
extern int rtsp_read_data(rtsp_t *s, char *buffer, unsigned int size)
{
  int i,seq;

  if (size>=4)
  {
    i = xio_tcp_read(s->s, buffer, 4);
    if (i<4) return i;
    if ((buffer[0]=='S')&&(buffer[1]=='E')&&(buffer[2]=='T')&&(buffer[3]=='_'))
    {
      char *rest=rtsp_get(s);
      if (!rest)
        return -1;
      
      seq=-1;
      do {
        free(rest);
        rest=rtsp_get(s);
        if (!rest)
          return -1;
        if (!strncasecmp(rest,"Cseq:",5))
          sscanf(rest,"%*s %u",&seq);
      } while (strlen(rest)!=0);
      free(rest);
      if (seq<0) {
        lprintf("warning: cseq not recognized!\n");

        seq=1;
      }
      /* lets make the server happy */
      rtsp_put(s, "RTSP/1.0 451 Parameter Not Understood");
      rest = (char *)malloc(sizeof(char)*17);
      sprintf(rest,"CSeq: %u", seq);
      rtsp_put(s, rest);
      rtsp_put(s, "");
      i = xio_tcp_read(s->s, buffer, size);
    } else
    {
      i = xio_tcp_read(s->s, buffer+4, size-4);
      i += 4;
    }
  } else
    i = xio_tcp_read(s->s, buffer, size);
  
  lprintf("<< %d of %d bytes\n", i, size);
  
  return i;
}

/** connect to a rtsp server */
extern rtsp_t * rtsp_connect(int fd, const char *mrl, const char *user_agent)
{
  rtsp_t *s = (rtsp_t *)malloc(sizeof(rtsp_t));
  
  char *mrl_ptr=strdup(mrl);
  char *slash, *colon;
  int hostend, pathbegin, i;
  
  if (strncmp(mrl,"rtsp://",7))
  {
    xprintf(XVERBOSITY_LOG, _("rtsp: bad mrl: %s\n"), mrl);
    free(s);
    return NULL;
  }
  
  mrl_ptr+=7;

  for (i=0; i<MAX_FIELDS; i++)
  {
    s->answers[i]=NULL;
    s->scheduled[i]=NULL;
  }

//  s->stream=stream;
  s->refers = 0;
  
  s->stail = -1;
  s->stamp = -1;
  
  s->host=NULL;
  s->port=554; /* rtsp standard port */
  s->path=NULL;
  s->mrl=NULL;
  s->mrl=strdup(mrl);
  
  s->server=NULL;
  s->server_state=0;
  s->server_caps=0;
  
  s->cseq=0;
  s->session=NULL;
  
  if (user_agent)
    s->user_agent=strdup(user_agent);
  else
    s->user_agent=strdup("User-Agent: RealMedia Player (HelixDNAClient)/10.0.0.0 (win32)");

//    s->user_agent=strdup("User-Agent: RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)");

  slash=strchr(mrl_ptr,'/');
  colon=strchr(mrl_ptr,':');

  if(!slash) slash=mrl_ptr+strlen(mrl_ptr)+1;
  if(!colon) colon=slash; 
  if(colon > slash) colon=slash;

  pathbegin=slash-mrl_ptr;
  hostend=colon-mrl_ptr;

  s->host = (char *)malloc(sizeof(char)*hostend+1);
  strncpy(s->host, mrl_ptr, hostend);
  s->host[hostend]=0;

  if (pathbegin < (int)strlen(mrl_ptr))
	  s->path=strdup(mrl_ptr+pathbegin+1);
  if (colon != slash) {
    int maxs;
    char buffer[64];

    maxs = pathbegin-hostend-1;
    if (maxs >= sizeof(buffer))
      maxs = sizeof(buffer) - 1;
    
    strncpy(buffer,mrl_ptr+hostend+1, maxs);
    buffer[maxs]=0;
    s->port=atoi(buffer);
    if (s->port < 0 || s->port > 65535) s->port = 554; /* rtsp standard port */
  }

  lprintf("got mrl: %s %i %s\n",s->host,s->port,s->path);

  if (fd >= 0)
  {
	  s->s = fd;
	  s->refers = 1;
	  fd = -1;
  }
  else
  {
	  s->s = xio_tcp_connect(s->host, s->port);
	  if (s->s < 0) {
		xprintf (XVERBOSITY_LOG, _("rtsp: failed to connect to '%s'\n"), s->host);
		rtsp_close(s);
		return NULL;
	  }
	  s->refers = 0;
	  s->server_state=RTSP_CONNECTED;
  }
  
  /* now lets send an options request. */
  rtsp_schedule_field(s, "CSeq: 1");
  rtsp_schedule_field(s, s->user_agent);
  rtsp_schedule_field(s, "ClientChallenge: 9e26d33f2984236010ef6253fb1887f7");
  rtsp_schedule_field(s, "PlayerStarttime: [28/03/2003:22:50:23 00:00]");
  rtsp_schedule_field(s, "CompanyID: KnKV4M4I/B2FjJ1TToLycw==");
  rtsp_schedule_field(s, "GUID: 00000000-0000-0000-0000-000000000000");
  rtsp_schedule_field(s, "RegionData: 0");
  rtsp_schedule_field(s, "ClientID: Linux_2.4_6.0.9.1235_play32_RN01_EN_586");
  rtsp_schedule_field(s, "Pragma: initiate-session");
  rtsp_request_options(s, NULL);
  
  return s;
}


/*
 * closes an rtsp connection 
 */
extern void rtsp_close(rtsp_t *s)
{
//  if (s->server_state) close(s->s); /* TODO: send a TEAROFF */
  if ((0 == s->refers) && (s->s >= 0))
  {
    close(s->s);
    s->s = -1;
  }
  if (s->path) free(s->path);
  if (s->host) free(s->host);
  if (s->mrl) free(s->mrl);
  if (s->session) free(s->session);
  if (s->user_agent) free(s->user_agent);
  rtsp_free_answers(s);
  rtsp_unschedule_all(s);
  free(s);  
}

/*
 * search in answers for tags. returns a pointer to the content
 * after the first matched tag. returns NULL if no match found.
 */
extern char * rtsp_search_answers(rtsp_t *s, const char *tag)
{
  char **answer;
  char *ptr;
  
  if (!s->answers) return NULL;
  answer=s->answers;

  while (*answer) {
    if (!strncasecmp(*answer,tag,strlen(tag))) {
      ptr=strchr(*answer,':');
      ptr++;
      while(*ptr==' ') ptr++;
      return ptr;
    }
    answer++;
  }

  return NULL;
}

/*
 * session id management
 */
extern void rtsp_set_session(rtsp_t *s, const char *id)
{
  if (s->session) free(s->session);

  s->session=strdup(id);

}

char *rtsp_get_session(rtsp_t *s) 
{

  return s->session;

}

char *rtsp_get_mrl(rtsp_t *s) 
{
  return s->mrl;
}

/*
 * schedules a field for transmission
 */

void rtsp_schedule_field(rtsp_t *s, const char *string) 
{

  int i=0;
  
  if (!string) return;

  while(s->scheduled[i]) 
  {
    i++;
  }
  s->scheduled[i]=strdup(string);
}

/*
 * removes the first scheduled field which prefix matches string. 
 */

void rtsp_unschedule_field(rtsp_t *s, const char *string) 
{

  char **ptr=s->scheduled;
  
  if (!string) return;

  while(*ptr) {
    if (!strncmp(*ptr, string, strlen(string)))
      break;
    ptr++;
  }
  if (*ptr) free(*ptr);
  ptr++;
  do {
    *(ptr-1)=*ptr;
    ptr++;
  } while(*ptr);
  
  *(ptr-1) = NULL;
}

/*
 * unschedule all fields
 */

void rtsp_unschedule_all(rtsp_t *s) 
{

  char **ptr;
  
  if (!s->scheduled) return;
  ptr=s->scheduled;

  while (*ptr) {
    free(*ptr);
    *ptr=NULL;
    ptr++;
  }
}
/*
 * free answers
 */

void rtsp_free_answers(rtsp_t *s) 
{

  char **answer;
  
  if (!s->answers) return;
  answer=s->answers;

  while (*answer) {
    free(*answer);
    *answer=NULL;
    answer++;
  }
}

extern int rtsp_get_rdt_stamp(rtsp_t *s, unsigned int *stamp)
{
	if (s->stamp < 0)
		return -ENODATA;
	if (stamp != NULL)
		*stamp = (unsigned int)s->stamp;
	
	return s->stamp;
}

/// get a packet from stream
extern int rtsp_get_rdt_chunk(rtsp_t *s, void *packet, unsigned char **buffer)
{
  int n=1;
  uint32_t ts;
  
  uint8_t header[8];
  rmff_pheader_t pheader;
  rmff_pheader_t *ph = &pheader;
  
  int size = 0;
  int flags1 = 0;
  int flags2 = 0;
  int sequence = 0;
  
  if (s->stail >= 0)
    return 0;
  
  n = rtsp_read_data(s, (char *)header, 8);
  if (n<8) return 0;
  if (header[0] != 0x24)
  {
    lprintf("rdt chunk not recognized: got 0x%02x\n", header[0]);
    return 0;
  }
  size = (header[1]<<16)+(header[2]<<8)+(header[3]);
  flags1 = header[4];
  if ((flags1 != 0x40) && (flags1 != 0x42))
  {
    lprintf("got flags1: 0x%02x\n",flags1);
    if (header[6]==0x06)
    {
      lprintf("got end of stream packet\n");
      s->stail = 0;
      return 0;
    }
    header[0] = header[5];
    header[1] = header[6];
    header[2] = header[7];
    n = rtsp_read_data(s, (char *)header+3, 5);
    if (n<5) return 0;
    lprintf("ignoring bytes:\n");
#ifdef LOG
    xutil_hexdump(header, 8);
#endif
    n = rtsp_read_data(s, (char *)header+4, 4);
    if (n<4) return 0;
    flags1 = header[4];
    size -= 9;
  }
  //unknown1 = (header[5]<<16)+(header[6]<<8)+(header[7]);
  
  /* ->DO: determine keyframe flag2 and insert here */
  
  if ((0x00 == header[7]) || (0x40 == header[7]) || 
      (0x80 == header[7]) || (0xC0 == header[7]))
  {
    flags2 |= RTSP_DATA_KEYFRAME;
  }
  sequence = ((int)header[5] << 8) + (int)header[6];
  
  n = rtsp_read_data(s, (char *)header, 6);
  if (n<6) return 0;
  
  ts = BE_32(header);
//  lprintf("ts: %u size: %u, flags: 0x%02x, unknown values: %u 0x%02x 0x%02x\n", 
//          ts, size, flags1, unknown1, header[4], header[5]);
// 
/**  if (stderr != NULL)
    fprintf(stderr, "%s stamp: %4u.%03u, size: %4u, flags: 0x%02x 0x%02x, "\
       "seq: %4u%s\n",
       ((flags1 >> 1) & 1) ? "video" : "audio",
       ts / 1000, ts % 1000, size, flags1, header[7],
       sequence, (flags2 & RTSP_DATA_KEYFRAME) ? ", keyframe" : "");
 */
  
  size += 2;
  s->stamp = (int)ts;
  
  if (packet != NULL)
    ph = (rmff_pheader_t *)packet;
  ph->object_version = 0;
  ph->length = size;
  ph->stream_number = (flags1 >> 1) & 1;
  ph->timestamp = ts;
  ph->reserved = 0;
  ph->flags = (uint8_t)flags2;
  
  xbuffer_ensure_size(*buffer, 12+size);
  rmff_dump_pheader(ph, (char *)*buffer);
  
  size-=12;
  n = rtsp_read_data(s, (char *)(*buffer)+12, size);
  
  if (s->stail >= 0)
  {
    s->stail = (int)ts;
    if (s->stail < 0)
      s->stail = 0;
  }
  return (n <= 0) ? 0 : n+12;
}


