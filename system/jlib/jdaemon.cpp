#include <cstdio>
#include "jfile.hpp"
#include "jmd5.hpp"
#include "build-config.h"

#include "jdaemon.hpp"


char* getMD5Checksum(StringBuffer filename, char* digestStr)
{
  if (filename.length() < 1)
      return NULL;

if (!checkFileExists(filename.str()))
      return NULL;

  OwnedIFile ifile = createIFile(filename);
  if (!ifile)
  return NULL;

  OwnedIFileIO ifileio = ifile->open(IFOread);
  if (!ifileio)
  return NULL;

  size32_t len = (size32_t) ifileio->size();
  if (len < 1)
  return NULL;

  char * buff = new char[1+len];
  size32_t len0 = ifileio->read(0, len, buff);
  buff[len0] = 0;

  md5_state_t md5;
  md5_byte_t digest[16];

  md5_init(&md5);
  md5_append(&md5, (const md5_byte_t *)buff, len0);
  md5_finish(&md5, digest);

  for (int i = 0; i < 16; i++)
      sprintf(&digestStr[i*2],"%02x", digest[i]);

delete[] buff;

  return digestStr;
}

