/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "contiki-net.h"
#include "device.h"
#include "device-fs.h"
#include "device-fsdata.h"

#include "device-fsdata.c"

#if DEVICE_FS_STATISTICS
static uint16_t count[DEVICE_FS_NUMFILES];
#endif /* DEVICE_FS_STATISTICS */

/*-----------------------------------------------------------------------------------*/
static uint8_t
device_fs_strcmp(const char *str1, const char *str2)
{
  uint8_t i;
  i = 0;

loop:
  if(str2[i] == 0 ||
     str1[i] == '\r' || 
     str1[i] == '\n') {
    return 0;
  }

  if(str1[i] != str2[i]) {
    return 1;
  }

  ++i;
  goto loop;
}
/*-----------------------------------------------------------------------------------*/
int
device_fs_open(const char *name, struct device_fs_file *file)
{
#if DEVICE_FS_STATISTICS
  uint16_t i = 0;
#endif /* DEVICE_FS_STATISTICS */
  struct device_fsdata_file_noconst *f;

  for(f = (struct device_fsdata_file_noconst *)DEVICE_FS_ROOT;
      f != NULL;
      f = (struct device_fsdata_file_noconst *)f->next) {

    if(device_fs_strcmp(name, f->name) == 0) {
      file->data = f->data;
      file->len = f->len;
#if DEVICE_FS_STATISTICS
      ++count[i];
#endif /* DEVICE_FS_STATISTICS */
      return 1;
    }
#if DEVICE_FS_STATISTICS
    ++i;
#endif /* DEVICE_FS_STATISTICS */

  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void
device_fs_init(void)
{
#if DEVICE_FS_STATISTICS
  uint16_t i;
  for(i = 0; i < DEVICE_FS_NUMFILES; i++) {
    count[i] = 0;
  }
#endif /* DEVICE_FS_STATISTICS */
}
/*-----------------------------------------------------------------------------------*/
#if DEVICE_FS_STATISTICS
uint16_t
device_fs_count(char *name)
{
  struct device_fsdata_file_noconst *f;
  uint16_t i;

  i = 0;
  for(f = (struct device_fsdata_file_noconst *)DEVICE_FS_ROOT;
      f != NULL;
      f = (struct device_fsdata_file_noconst *)f->next) {

    if(device_fs_strcmp(name, f->name) == 0) {
      return count[i];
    }
    ++i;
  }
  return 0;
}
#endif /* DEVICE_FS_STATISTICS */
/*-----------------------------------------------------------------------------------*/
