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
 * $Id: real.c,v 1.20 2004/12/15 12:53:46 miguelfreitas Exp $
 *
 * special functions for real streams.
 * adopted from joschkas real tools.
 *
 */

#include <stdio.h>
#include <string.h>

#define LOG_MODULE "real"
#define LOG_VERBOSE
/*
#define LOG
*/
#include "xsysio.h"

#include "real.h"
#include "rtsp_asmrp.h"
#include "rtsp_sdpplin.h"


const unsigned char xor_table[] = {
    0x05, 0x18, 0x74, 0xd0, 0x0d, 0x09, 0x02, 0x53,
    0xc0, 0x01, 0x05, 0x05, 0x67, 0x03, 0x19, 0x70,
    0x08, 0x27, 0x66, 0x10, 0x10, 0x72, 0x08, 0x09,
    0x63, 0x11, 0x03, 0x71, 0x08, 0x08, 0x70, 0x02,
    0x10, 0x57, 0x05, 0x18, 0x54, 0x00, 0x00, 0x00 };

#define BE_32C(x,y) do { *(uint32_t *)(x) = be2me_32((y)); } while(0)
#define LE_32C(x,y) do { *(uint32_t *)(x) = le2me_32((y)); } while(0)

#define MAX(x,y) ((x>y) ? x : y)


static void hash(char *field, char *param)
{

  uint32_t a, b, c, d;
 
  /* fill variables */
  a = LE_32(field);
  b = LE_32(field+4);
  c = LE_32(field+8);
  d = LE_32(field+12);
  
  lprintf("hash input: %x %x %x %x\n", a, b, c, d);
  lprintf("hash parameter:\n");
#ifdef LOG
  xine_hexdump(param, 64);
#endif
  
  a = ((b & c) | (~b & d)) + LE_32((param+0x00)) + a - 0x28955B88;
  a = ((a << 0x07) | (a >> 0x19)) + b;
  d = ((a & b) | (~a & c)) + LE_32((param+0x04)) + d - 0x173848AA;
  d = ((d << 0x0c) | (d >> 0x14)) + a;
  c = ((d & a) | (~d & b)) + LE_32((param+0x08)) + c + 0x242070DB;
  c = ((c << 0x11) | (c >> 0x0f)) + d;
  b = ((c & d) | (~c & a)) + LE_32((param+0x0c)) + b - 0x3E423112;
  b = ((b << 0x16) | (b >> 0x0a)) + c;
  a = ((b & c) | (~b & d)) + LE_32((param+0x10)) + a - 0x0A83F051;
  a = ((a << 0x07) | (a >> 0x19)) + b;
  d = ((a & b) | (~a & c)) + LE_32((param+0x14)) + d + 0x4787C62A;
  d = ((d << 0x0c) | (d >> 0x14)) + a;
  c = ((d & a) | (~d & b)) + LE_32((param+0x18)) + c - 0x57CFB9ED;
  c = ((c << 0x11) | (c >> 0x0f)) + d;
  b = ((c & d) | (~c & a)) + LE_32((param+0x1c)) + b - 0x02B96AFF;
  b = ((b << 0x16) | (b >> 0x0a)) + c;
  a = ((b & c) | (~b & d)) + LE_32((param+0x20)) + a + 0x698098D8;
  a = ((a << 0x07) | (a >> 0x19)) + b;
  d = ((a & b) | (~a & c)) + LE_32((param+0x24)) + d - 0x74BB0851;
  d = ((d << 0x0c) | (d >> 0x14)) + a;
  c = ((d & a) | (~d & b)) + LE_32((param+0x28)) + c - 0x0000A44F;
  c = ((c << 0x11) | (c >> 0x0f)) + d;
  b = ((c & d) | (~c & a)) + LE_32((param+0x2C)) + b - 0x76A32842;
  b = ((b << 0x16) | (b >> 0x0a)) + c;
  a = ((b & c) | (~b & d)) + LE_32((param+0x30)) + a + 0x6B901122;
  a = ((a << 0x07) | (a >> 0x19)) + b;
  d = ((a & b) | (~a & c)) + LE_32((param+0x34)) + d - 0x02678E6D;
  d = ((d << 0x0c) | (d >> 0x14)) + a;
  c = ((d & a) | (~d & b)) + LE_32((param+0x38)) + c - 0x5986BC72;
  c = ((c << 0x11) | (c >> 0x0f)) + d;
  b = ((c & d) | (~c & a)) + LE_32((param+0x3c)) + b + 0x49B40821;
  b = ((b << 0x16) | (b >> 0x0a)) + c;
  
  a = ((b & d) | (~d & c)) + LE_32((param+0x04)) + a - 0x09E1DA9E;
  a = ((a << 0x05) | (a >> 0x1b)) + b;
  d = ((a & c) | (~c & b)) + LE_32((param+0x18)) + d - 0x3FBF4CC0;
  d = ((d << 0x09) | (d >> 0x17)) + a;
  c = ((d & b) | (~b & a)) + LE_32((param+0x2c)) + c + 0x265E5A51;
  c = ((c << 0x0e) | (c >> 0x12)) + d;
  b = ((c & a) | (~a & d)) + LE_32((param+0x00)) + b - 0x16493856;
  b = ((b << 0x14) | (b >> 0x0c)) + c;
  a = ((b & d) | (~d & c)) + LE_32((param+0x14)) + a - 0x29D0EFA3;
  a = ((a << 0x05) | (a >> 0x1b)) + b;
  d = ((a & c) | (~c & b)) + LE_32((param+0x28)) + d + 0x02441453;
  d = ((d << 0x09) | (d >> 0x17)) + a;
  c = ((d & b) | (~b & a)) + LE_32((param+0x3c)) + c - 0x275E197F;
  c = ((c << 0x0e) | (c >> 0x12)) + d;
  b = ((c & a) | (~a & d)) + LE_32((param+0x10)) + b - 0x182C0438;
  b = ((b << 0x14) | (b >> 0x0c)) + c;
  a = ((b & d) | (~d & c)) + LE_32((param+0x24)) + a + 0x21E1CDE6;
  a = ((a << 0x05) | (a >> 0x1b)) + b;
  d = ((a & c) | (~c & b)) + LE_32((param+0x38)) + d - 0x3CC8F82A;
  d = ((d << 0x09) | (d >> 0x17)) + a;
  c = ((d & b) | (~b & a)) + LE_32((param+0x0c)) + c - 0x0B2AF279;
  c = ((c << 0x0e) | (c >> 0x12)) + d;
  b = ((c & a) | (~a & d)) + LE_32((param+0x20)) + b + 0x455A14ED;
  b = ((b << 0x14) | (b >> 0x0c)) + c;
  a = ((b & d) | (~d & c)) + LE_32((param+0x34)) + a - 0x561C16FB;
  a = ((a << 0x05) | (a >> 0x1b)) + b;
  d = ((a & c) | (~c & b)) + LE_32((param+0x08)) + d - 0x03105C08;
  d = ((d << 0x09) | (d >> 0x17)) + a;
  c = ((d & b) | (~b & a)) + LE_32((param+0x1c)) + c + 0x676F02D9;
  c = ((c << 0x0e) | (c >> 0x12)) + d;
  b = ((c & a) | (~a & d)) + LE_32((param+0x30)) + b - 0x72D5B376;
  b = ((b << 0x14) | (b >> 0x0c)) + c;
  
  a = (b ^ c ^ d) + LE_32((param+0x14)) + a - 0x0005C6BE;
  a = ((a << 0x04) | (a >> 0x1c)) + b;
  d = (a ^ b ^ c) + LE_32((param+0x20)) + d - 0x788E097F;
  d = ((d << 0x0b) | (d >> 0x15)) + a;
  c = (d ^ a ^ b) + LE_32((param+0x2c)) + c + 0x6D9D6122;
  c = ((c << 0x10) | (c >> 0x10)) + d;
  b = (c ^ d ^ a) + LE_32((param+0x38)) + b - 0x021AC7F4;
  b = ((b << 0x17) | (b >> 0x09)) + c;
  a = (b ^ c ^ d) + LE_32((param+0x04)) + a - 0x5B4115BC;
  a = ((a << 0x04) | (a >> 0x1c)) + b;
  d = (a ^ b ^ c) + LE_32((param+0x10)) + d + 0x4BDECFA9;
  d = ((d << 0x0b) | (d >> 0x15)) + a;
  c = (d ^ a ^ b) + LE_32((param+0x1c)) + c - 0x0944B4A0;
  c = ((c << 0x10) | (c >> 0x10)) + d;
  b = (c ^ d ^ a) + LE_32((param+0x28)) + b - 0x41404390;
  b = ((b << 0x17) | (b >> 0x09)) + c;
  a = (b ^ c ^ d) + LE_32((param+0x34)) + a + 0x289B7EC6;
  a = ((a << 0x04) | (a >> 0x1c)) + b;
  d = (a ^ b ^ c) + LE_32((param+0x00)) + d - 0x155ED806;
  d = ((d << 0x0b) | (d >> 0x15)) + a;
  c = (d ^ a ^ b) + LE_32((param+0x0c)) + c - 0x2B10CF7B;
  c = ((c << 0x10) | (c >> 0x10)) + d;
  b = (c ^ d ^ a) + LE_32((param+0x18)) + b + 0x04881D05;
  b = ((b << 0x17) | (b >> 0x09)) + c;
  a = (b ^ c ^ d) + LE_32((param+0x24)) + a - 0x262B2FC7;
  a = ((a << 0x04) | (a >> 0x1c)) + b;
  d = (a ^ b ^ c) + LE_32((param+0x30)) + d - 0x1924661B;
  d = ((d << 0x0b) | (d >> 0x15)) + a;
  c = (d ^ a ^ b) + LE_32((param+0x3c)) + c + 0x1fa27cf8;
  c = ((c << 0x10) | (c >> 0x10)) + d;
  b = (c ^ d ^ a) + LE_32((param+0x08)) + b - 0x3B53A99B;
  b = ((b << 0x17) | (b >> 0x09)) + c;
  
  a = ((~d | b) ^ c)  + LE_32((param+0x00)) + a - 0x0BD6DDBC;
  a = ((a << 0x06) | (a >> 0x1a)) + b; 
  d = ((~c | a) ^ b)  + LE_32((param+0x1c)) + d + 0x432AFF97;
  d = ((d << 0x0a) | (d >> 0x16)) + a; 
  c = ((~b | d) ^ a)  + LE_32((param+0x38)) + c - 0x546BDC59;
  c = ((c << 0x0f) | (c >> 0x11)) + d; 
  b = ((~a | c) ^ d)  + LE_32((param+0x14)) + b - 0x036C5FC7;
  b = ((b << 0x15) | (b >> 0x0b)) + c; 
  a = ((~d | b) ^ c)  + LE_32((param+0x30)) + a + 0x655B59C3;
  a = ((a << 0x06) | (a >> 0x1a)) + b; 
  d = ((~c | a) ^ b)  + LE_32((param+0x0C)) + d - 0x70F3336E;
  d = ((d << 0x0a) | (d >> 0x16)) + a; 
  c = ((~b | d) ^ a)  + LE_32((param+0x28)) + c - 0x00100B83;
  c = ((c << 0x0f) | (c >> 0x11)) + d; 
  b = ((~a | c) ^ d)  + LE_32((param+0x04)) + b - 0x7A7BA22F;
  b = ((b << 0x15) | (b >> 0x0b)) + c; 
  a = ((~d | b) ^ c)  + LE_32((param+0x20)) + a + 0x6FA87E4F;
  a = ((a << 0x06) | (a >> 0x1a)) + b; 
  d = ((~c | a) ^ b)  + LE_32((param+0x3c)) + d - 0x01D31920;
  d = ((d << 0x0a) | (d >> 0x16)) + a; 
  c = ((~b | d) ^ a)  + LE_32((param+0x18)) + c - 0x5CFEBCEC;
  c = ((c << 0x0f) | (c >> 0x11)) + d; 
  b = ((~a | c) ^ d)  + LE_32((param+0x34)) + b + 0x4E0811A1;
  b = ((b << 0x15) | (b >> 0x0b)) + c; 
  a = ((~d | b) ^ c)  + LE_32((param+0x10)) + a - 0x08AC817E;
  a = ((a << 0x06) | (a >> 0x1a)) + b; 
  d = ((~c | a) ^ b)  + LE_32((param+0x2c)) + d - 0x42C50DCB;
  d = ((d << 0x0a) | (d >> 0x16)) + a; 
  c = ((~b | d) ^ a)  + LE_32((param+0x08)) + c + 0x2AD7D2BB;
  c = ((c << 0x0f) | (c >> 0x11)) + d; 
  b = ((~a | c) ^ d)  + LE_32((param+0x24)) + b - 0x14792C6F;
  b = ((b << 0x15) | (b >> 0x0b)) + c; 

  lprintf("hash output: %x %x %x %x\n", a, b, c, d);
  
  a += LE_32(field);
  b += LE_32(field+4);
  c += LE_32(field+8);
  d += LE_32(field+12);

  LE_32C(field, a);
  LE_32C(field+4, b);
  LE_32C(field+8, c);
  LE_32C(field+12, d);
}

static void call_hash (char *key, char *challenge, int len)
{
  uint8_t *ptr1, *ptr2;
  uint32_t a, b, c, d, tmp;
  
  ptr1 = (uint8_t *)(key+16);
  ptr2 = (uint8_t *)(key+20);
  
  a = LE_32(ptr1);
  b = (a >> 3) & 0x3f;
  a += len * 8;
  LE_32C(ptr1, a);
  
  if ((int)a < (len << 3))
  {
    lprintf("not verified: (len << 3) > a true\n");
    ptr2 += 4;
  }

  tmp = LE_32(ptr2) + (len >> 0x1d);
  LE_32C(ptr2, tmp);
  a = 64 - b;
  c = 0;  
  if ((int)a <= len)
  {

    memcpy(key+b+24, challenge, a);
    hash(key, key+24);
    c = a;
    d = c + 0x3f;
    
    while ((int)d < len ) {
      lprintf("not verified:  while ( d < len )\n");
      hash(key, challenge+d-0x3f);
      d += 64;
      c += 64;
    }
    b = 0;
  }
  
  memcpy(key+b+24, challenge+c, len-c);
}

static void calc_response (char *result, char *field)
{
  char buf1[128];
  char buf2[128];
  int i;

  memset (buf1, 0, 64);
  *buf1 = 128;
  
  memcpy (buf2, field+16, 8);
  
  i = ( LE_32((buf2)) >> 3 ) & 0x3f;
 
  if (i < 56) {
    i = 56 - i;
  } else {
    lprintf("not verified: ! (i < 56)\n");
    i = 120 - i;
  }

  call_hash (field, buf1, i);
  call_hash (field, buf2, 8);

  memcpy (result, field, 16);
}


static void
calc_response_string (char *result, char *challenge)
{
  char field[128];
  char zres[20];
  int  i;
      
  /* initialize our field */
  BE_32C (field,      0x01234567);
  BE_32C ((field+4),  0x89ABCDEF);
  BE_32C ((field+8),  0xFEDCBA98);
  BE_32C ((field+12), 0x76543210);
  BE_32C ((field+16), 0x00000000);
  BE_32C ((field+20), 0x00000000);

  /* calculate response */
  call_hash(field, challenge, 64);
  calc_response(zres,field);
 
  /* convert zres to ascii string */
  for (i=0; i<16; i++ ) {
    char a, b;
    
    a = (zres[i] >> 4) & 15;
    b = zres[i] & 15;

    result[i*2]   = ((a<10) ? (a+48) : (a+87)) & 255;
    result[i*2+1] = ((b<10) ? (b+48) : (b+87)) & 255;
  }
}

extern void
real_calc_response_and_checksum (char *response, char *chksum, char *challenge)
{
  int   ch_len, table_len, resp_len;
  int   i;
  char *ptr;
  char  buf[128];

  /* initialize return values */
  memset(response, 0, 64);
  memset(chksum, 0, 34);

  /* initialize buffer */
  memset(buf, 0, 128);
  ptr=buf;
  BE_32C(ptr, 0xa1e9149d);
  ptr+=4;
  BE_32C(ptr, 0x0e6b3b59);
  ptr+=4;

  /* some (length) checks */
  if (challenge != NULL)
  {
    ch_len = strlen (challenge);

    if (ch_len == 40) /* what a hack... */
    {
      challenge[32]=0;
      ch_len=32;
    }
    if ( ch_len > 56 ) ch_len=56;
    
    /* copy challenge to buf */
    memcpy(ptr, challenge, ch_len);
  }
  
  if (xor_table != NULL)
  {
    table_len = strlen((const char *)xor_table);

    if (table_len > 56) table_len=56;

    /* xor challenge bytewise with xor_table */
    for (i=0; i<table_len; i++)
      ptr[i] = ptr[i] ^ xor_table[i];
  }

  calc_response_string (response, buf);

  /* add tail */
  resp_len = strlen (response);
  strcpy (&response[resp_len], "01d0a8e3");

  /* calculate checksum */
  for (i=0; i<resp_len/4; i++)
    chksum[i] = response[i*4];
}


/*
 * takes a MLTI-Chunk and a rule number got from match_asm_rule,
 * returns a pointer to selected data and number of bytes in that.
 */
static int
select_mlti_data(const char *mlti_chunk, int mlti_size, int selection, char **out)
{
  int numrules, codec, size;
  int i;
  
  /* MLTI chunk should begin with MLTI */

  if ((mlti_chunk[0] != 'M')
      ||(mlti_chunk[1] != 'L')
      ||(mlti_chunk[2] != 'T')
      ||(mlti_chunk[3] != 'I'))
  {
    lprintf("MLTI tag not detected, copying data\n");
    xbuffer_copyin(*out, 0, (void *)mlti_chunk, mlti_size);
    return mlti_size;
  }

  mlti_chunk+=4;

  /* next 16 bits are the number of rules */
  numrules=BE_16(mlti_chunk);
  if (selection >= numrules) return 0;

  /* now <numrules> indices of codecs follows */
  /* we skip to selection                     */
  mlti_chunk+=(selection+1)*2;

  /* get our index */
  codec=BE_16(mlti_chunk);

  /* skip to number of codecs */
  mlti_chunk+=(numrules-selection)*2;

  /* get number of codecs */
  numrules=BE_16(mlti_chunk);

  if (codec >= numrules) {
    lprintf("codec index >= number of codecs. %i %i\n", codec, numrules);
    return 0;
  }

  mlti_chunk+=2;
 
  /* now seek to selected codec */
  for (i=0; i<codec; i++) {
    size=BE_32(mlti_chunk);
    mlti_chunk+=size+4;
  }
  
  size=BE_32(mlti_chunk);

#ifdef LOG
  xine_hexdump(mlti_chunk+4, size);
#endif
  xbuffer_copyin(*out, 0, mlti_chunk+4, size);
  return size;
}

/*
 * looking at stream description.
 */
extern rmff_header_t *
real_parse_sdp(char *data, char **stream_rules, uint32_t bandwidth)
{
  sdpplin_t *desc;
  rmff_header_t *header;
  char *buf;
  int len, i;
  int max_bit_rate=0;
  int avg_bit_rate=0;
  int max_packet_size=0;
  int avg_packet_size=0;
  int preroll=0;
  int duration=0;

  if (!data) return NULL;

  desc=sdpplin_parse(data);

  if (!desc) return NULL;
 
  buf = (char *)xbuffer_init(2048);
  header = (rmff_header_t *)xutil_malloc(sizeof(rmff_header_t));
  
  header->fileheader=rmff_new_fileheader(4+desc->stream_count);
  header->cont=rmff_new_cont(desc->title, desc->author,
	   desc->copyright, desc->abstract);
  header->data=rmff_new_dataheader(0,0);
  header->streams = (rmff_mdpr_t **)xutil_malloc(
	   sizeof(rmff_mdpr_t*)*(desc->stream_count+1));
  lprintf("number of streams: %u\n", desc->stream_count);

  for (i=0; i<desc->stream_count; i++)
  {
    int j=0;
    int n;
    char b[64];
    int rulematches[16];

    lprintf("calling asmrp_match with:\n%s\n%u\n", desc->stream[i]->asm_rule_book, bandwidth);

    n=asmrp_match(desc->stream[i]->asm_rule_book, bandwidth, rulematches);
    for (j=0; j<n; j++) {
      lprintf("asmrp rule match: %u for stream %u\n", rulematches[j], desc->stream[i]->stream_id);
      sprintf(b,"stream=%u;rule=%u,", desc->stream[i]->stream_id, rulematches[j]);
      xbuffer_strcat(*stream_rules, b);
    }

    if (!desc->stream[i]->mlti_data) return NULL;

    len=select_mlti_data(desc->stream[i]->mlti_data, desc->stream[i]->mlti_data_size, rulematches[0], &buf);
    
    header->streams[i]=rmff_new_mdpr(
	desc->stream[i]->stream_id,
        desc->stream[i]->max_bit_rate,
        desc->stream[i]->avg_bit_rate,
        desc->stream[i]->max_packet_size,
        desc->stream[i]->avg_packet_size,
        desc->stream[i]->start_time,
        desc->stream[i]->preroll,
        desc->stream[i]->duration,
        desc->stream[i]->stream_name,
        desc->stream[i]->mime_type,
        len,
        buf);
    
	preroll=MAX(preroll,desc->stream[i]->preroll);
	duration=MAX(duration,desc->stream[i]->duration);
    max_bit_rate+=desc->stream[i]->max_bit_rate;
    avg_bit_rate+=desc->stream[i]->avg_bit_rate;
    max_packet_size=MAX(max_packet_size, desc->stream[i]->max_packet_size);
    if (avg_packet_size)
      avg_packet_size=(avg_packet_size + desc->stream[i]->avg_packet_size) / 2;
    else
      avg_packet_size=desc->stream[i]->avg_packet_size;
  }
  
  if (*stream_rules && strlen(*stream_rules) && (*stream_rules)[strlen(*stream_rules)-1] == ',')
    (*stream_rules)[strlen(*stream_rules)-1]=0; /* delete last ',' in stream_rules */

  header->prop=rmff_new_prop(
      max_bit_rate,
      avg_bit_rate,
      max_packet_size,
      avg_packet_size,
      0,
      duration,
      preroll,
      0,
      0,
      desc->stream_count,
      desc->flags);

  rmff_fix_header(header);
  xbuffer_free(buf);

  return header;
}


//! maximum size of the rtsp description, must be < INT_MAX
#define MAX_DESC_BUF (20 * 1024 * 1024)

extern rmff_header_t *
real_setup_and_get_header(rtsp_t *session, uint32_t bandwidth)
{
  char *description=NULL;
  char *session_id=NULL;
  rmff_header_t *h;
  char *challenge1;
  char challenge2[64];
  char checksum[34];
  char *subscribe;
  char *buf=(char *)xbuffer_init(256);
  char *mrl=rtsp_get_mrl(session);
  unsigned int size;
  int status;
  const char *found = NULL;
  
  /* get challenge */
  challenge1=strdup(rtsp_search_answers(session,"RealChallenge1"));
  lprintf("Challenge1: %s\n", challenge1);
  
  if (bandwidth <= 0)
    bandwidth = 10485800;
  
  /* request stream description */
  rtsp_schedule_field(session, "Accept: application/sdp");
  sprintf(buf, "Bandwidth: %u", bandwidth);
  rtsp_schedule_field(session, buf);

  rtsp_schedule_field(session, "GUID: 00000000-0000-0000-0000-000000000000");
  rtsp_schedule_field(session, "RegionData: 0");
  rtsp_schedule_field(session, "ClientID: Linux_2.4_6.0.9.1235_play32_RN01_EN_586");
  rtsp_schedule_field(session, "SupportsMaximumASMBandwidth: 1");
  rtsp_schedule_field(session, "Language: en-US");
  rtsp_schedule_field(session, "Require: com.real.retain-entity-for-setup");
/** */
  status=rtsp_request_describe(session,NULL);
  
  /* (status < 200 || status > 299) */
  if (RTSP_STATUS_FAILED(status))
  {
    char *alert=rtsp_search_answers(session,"Alert");
    if (alert) {
      lprintf("real: got message from server:\n%s\n", alert);
    }
    rtsp_send_ok(session);
    xbuffer_free(buf);
    return NULL;
  }

  /* receive description */
  size=0;
  if (!rtsp_search_answers(session,"Content-length"))
    lprintf("real: got no Content-length!\n");
  else
    size=atoi(rtsp_search_answers(session,"Content-length"));

  if (size > MAX_DESC_BUF) {
    printf("real: Content-length for description too big (> %uMB)!\n",
           MAX_DESC_BUF/(1024*1024) );
    xbuffer_free(buf);
    return NULL;
  }
/**
  if (!rtsp_search_answers(session,"ETag"))
    lprintf("real: got no ETag!\n");
  else
    session_id=strdup(rtsp_search_answers(session,"ETag"));
 */
  found = rtsp_search_answers(session,"ETag");
  if (NULL == found)
    found = rtsp_search_answers(session,"Session");
  if (NULL == found)
    lprintf("real: got no ETag/Session!\n");
  else
    session_id = strdup(found);
  
  lprintf("Stream description size: %i\n", size);
  
  description = (char *)malloc(sizeof(char)*(size+1));

  if( rtsp_read_data(session, description, size) <= 0) {
    xbuffer_free(buf);
    return NULL;
  }
  description[size]=0;

  /* parse sdp (sdpplin) and create a header and a subscribe string */
  subscribe=(char *)xbuffer_init(256);
  strcpy(subscribe, "Subscribe: ");
  h=real_parse_sdp(description, &subscribe, bandwidth);
  if (!h) {
    xbuffer_free(subscribe);
    xbuffer_free(buf);
    return NULL;
  }
  rmff_fix_header(h);

  lprintf("Title: %s\nCopyright: %s\nAuthor: %s\nStreams: %i\n",
	  h->cont->title, h->cont->copyright, h->cont->author, h->prop->num_streams);
  
  /* setup our streams */
  real_calc_response_and_checksum (challenge2, checksum, challenge1);
  xbuffer_ensure_size(buf, strlen(challenge2) + strlen(checksum) + 32);
  sprintf(buf, "RealChallenge2: %s, sd=%s", challenge2, checksum);
  rtsp_schedule_field(session, buf);
  xbuffer_ensure_size(buf, strlen(session_id) + 32);
  sprintf(buf, "If-Match: %s", session_id);
  rtsp_schedule_field(session, buf);
//  rtsp_schedule_field(session, "RDTFeatureLevel: 3");
  rtsp_schedule_field(session, "Transport: x-pn-tng/tcp;mode=play,rtp/avp/tcp;unicast;mode=play");
//  rtsp_schedule_field(session, "Transport: x-pn-tng/tcp;mode=play");
  
  xbuffer_ensure_size(buf, strlen(mrl) + 32);
  sprintf(buf, "%s/streamid=0", mrl);
  rtsp_request_setup(session, buf);

  if (h->prop->num_streams > 1) {
//    rtsp_schedule_field(session, "RDTFeatureLevel: 3");
    rtsp_schedule_field(session, "Transport: x-pn-tng/tcp;mode=play,rtp/avp/tcp;unicast;mode=play");
//    rtsp_schedule_field(session, "Transport: x-pn-tng/tcp;mode=play");
    xbuffer_ensure_size(buf, strlen(session_id) + 32);
    sprintf(buf, "If-Match: %s", session_id);
    rtsp_schedule_field(session, buf);
    
    xbuffer_ensure_size(buf, strlen(mrl) + 32);
    sprintf(buf, "%s/streamid=1", mrl);
    rtsp_request_setup(session, buf);
  }
  /* set stream parameter (bandwidth) with our subscribe string */
  rtsp_schedule_field(session, subscribe);
  rtsp_request_setparameter(session, NULL);

  /* and finally send a play request */
  
/**
  rtsp_schedule_field(session, "Range: npt=0-");
  rtsp_request_play(session, NULL);
 */

  xbuffer_free(subscribe);
  xbuffer_free(buf);
  return h;
}

extern int real_setup_act_play_range(rtsp_t *session, uint32_t start, uint32_t length)
{
	int ret = 0;
	char srange[256];
	
	uint32_t sending = start;
	
	sending += length;
	
	if ((int)length <= 0)
	{
		ret = snprintf(srange, sizeof(srange)-2,
			 "Range: npt=%u.%03u-", start/1000, start%1000);
		x_assert(ret > 0);
	}
	else
	{
		ret = snprintf(srange, sizeof(srange)-2,
			 "Range: npt=%u.%03u-%u.%03u",
			 start/1000, start%1000, sending/1000, sending%1000);
		x_assert(ret > 0);
	}
	
/*	rtsp_schedule_field(session, "Range: npt=0:-");
 */
	rtsp_schedule_field(session, srange);
	
	return rtsp_request_play(session, NULL);
}

extern rmff_header_t *
real_setup_and_play_range(rtsp_t *session,
	 uint32_t bandwidth, uint32_t start, uint32_t length)
{
	int ret = 0;
	rmff_header_t *h = real_setup_and_get_header(session, bandwidth);
	
	if (NULL == h)
		return NULL;
	
	ret = real_setup_act_play_range(session, start, length);
	if ((ret < 0) || (ret != RTSP_STATUS_OK))
	{
		rmff_free_header(h);
		return NULL;
	}
	
	return h;
}


