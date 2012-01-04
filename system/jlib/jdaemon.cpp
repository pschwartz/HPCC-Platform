#include <cstdio>
#include "jlog.hpp"
#include "jfile.hpp"
#include "jargv.hpp"
#include "junicode.hpp"
#include "jprop.hpp"
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

bool extractCLICmdOption(StringBuffer & option, IProperties * globals, const char * envName, const char * propertyName, const char * defaultPrefix, const char * defaultSuffix)
{
  if (option.length())        // check if already specified via a command line option
      return true;
  if (propertyName && globals->getProp(propertyName, option))
      return true;
  if (envName && *envName)
  {
      const char * env = getenv(envName);
      if (env)
      {
          option.append(env);
          return true;
      }
  }
  if (defaultPrefix)
      option.append(defaultPrefix);
  if (defaultSuffix)
      option.append(defaultSuffix);
  return false;
}

bool extractCLICmdOption(StringAttr & option, IProperties * globals, const char * envName, const char * propertyName, const char * defaultPrefix, const char * defaultSuffix)
{
  if (option)
      return true;
  StringBuffer temp;
  bool ret = extractCLICmdOption(temp, globals, envName, propertyName, defaultPrefix, defaultSuffix);
  option.set(temp.str());
  return ret;
}

bool extractCLICmdOption(bool & option, IProperties * globals, const char * envName, const char * propertyName, bool defval)
{
  StringBuffer temp;
  bool ret = extractCLICmdOption(temp, globals, envName, propertyName, defval ? "1" : "0", NULL);
  option=(streq(temp.str(),"1")||strieq(temp.str(),"true"));
  return ret;
}

bool extractCLICmdOption(unsigned & option, IProperties * globals, const char * envName, const char * propertyName, unsigned defval)
{
  StringBuffer temp;
  bool ret = extractCLICmdOption(temp, globals, envName, propertyName, NULL, NULL);
  option = (ret) ? strtoul(temp.str(), NULL, 10) : defval;
  return ret;
}

CLICmdOptionMatchIndicator CLICmdCommon::matchCommandLineOption(ArgvIterator &iter, bool finalAttempt)
{
    bool boolValue;
    if (iter.matchFlag(boolValue, CLIOPT_VERSION))
    {
        fprintf(stdout, "%s\n", BUILD_TAG);
        return CLICmdOptionCompletion;
    }
    if (iter.matchOption(optEnv, CLIOPT_ENV))
        return CLICmdOptionMatch;
    if (iter.matchOption(optName, CLIOPT_NAME))
        return CLICmdOptionMatch;
    if (iter.matchFlag(optForeground, CLIOPT_FOREGROUND))
            return CLICmdOptionMatch;

    StringAttr tempArg;
    if (iter.matchOption(tempArg, "-brk"))
    {
#if defined(_WIN32) && defined(_DEBUG)
        unsigned id = atoi(tempArg.sget());
        if (id == 0)
            DebugBreak();
        else
            _CrtSetBreakAlloc(id);
#endif
        return CLICmdOptionMatch;
    }
    if (finalAttempt)
        fprintf(stderr, "\n%s option not recognized\n", iter.query());
    return CLICmdOptionNoMatch;
}

bool CLICmdCommon::finalizeOptions(IProperties *globals)
{
    extractCLICmdOption(optEnv, globals, NULL, NULL, CLIOPT_ENV_DEFAULT, NULL);
    extractCLICmdOption(optName, globals, NULL, NULL, NULL, NULL);
    extractCLICmdOption(optForeground, globals, NULL, NULL, NULL);
    return true;
}
