#include <cstdio>
#include "jlog.hpp"
#include "jfile.hpp"
#include "jargv.hpp"
#include "junicode.hpp"
#include "jprop.hpp"
#include "build-config.h"

#include "jcli.hpp"

#ifdef _WIN32
#include "process.h"
#endif

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

ICLICommand *createCoreCLICommand(const char *cmdname)
{
    if (!cmdname || !*cmdname)
        return NULL;
    /*
    if (strieq(cmdname, "deploy"))
        return new EclCmdDeploy();
    if (strieq(cmdname, "publish"))
        return new EclCmdPublish();
    if (strieq(cmdname, "activate"))
        return new EclCmdActivate();
    if (strieq(cmdname, "deactivate"))
        return new EclCmdDeactivate();
        */
    return NULL;
}

int CLICMDShell::callExternal(ArgvIterator &iter)
{
    const char *argv[100];
    StringBuffer cmdstr("ecl-");
    cmdstr.append(cmd.sget());
    int i=0;
    argv[i++]=cmdstr.str();
    if (optHelp)
        argv[i++]="help";
    for (; !iter.done(); iter.next())
        argv[i++]=iter.query();
    argv[i]=NULL;
//TODO - add common routine or use existing in jlib
#ifdef _WIN32
    if (_spawnvp(_P_WAIT, cmdstr.str(), const_cast<char **>(argv))==-1)
#else
    if (execvp(cmdstr.str(), const_cast<char **>(argv))==-1)
#endif
    {
        switch(errno)
        {
        case ENOENT:
            fprintf(stderr, "ecl '%s' command not found\n", cmd.sget());
            return 1;
        default:
            fprintf(stderr, "ecl '%s' command error %d\n", cmd.sget(), errno);
            return 1;
        }
    }
    return 0;
}

int CLICMDShell::processCMD(ArgvIterator &iter)
{
    Owned<ICLICommand> c = factory(cmd.get());
    if (!c)
    {
        if (cmd.length())
        {
            if (runExternals)
                return callExternal(iter);
            fprintf(stderr, "ecl '%s' command not found\n", cmd.sget());
        }
        usage();
        return 1;
    }
    if (optHelp)
    {
        c->usage();
        return 0;
    }
    if (!c->parseCommandLineOptions(iter))
        return 0;

    if (!c->finalizeOptions(globals))
        return 0;

    return c->processCMD();
}

void CLICMDShell::finalizeOptions(IProperties *globals)
{
}

int CLICMDShell::run()
{
    try
    {
        if (!parseCommandLineOptions(args))
            return 1;

        return processCMD(args);
    }
    catch (IException *E)
    {
        StringBuffer m("Error: ");
        fputs(E->errorMessage(m).newline().str(), stderr);
        E->Release();
        return 2;
    }
#ifndef _DEBUG
    catch (...)
    {
        ERRLOG("Unexpected exception\n");
        return 4;
    }
#endif
    return 0;
}

//=========================================================================================

bool CLICMDShell::parseCommandLineOptions(ArgvIterator &iter)
{
    if (iter.done())
    {
        usage();
        return false;
    }

    bool boolValue;
    for (; !iter.done(); iter.next())
    {
        const char * arg = iter.query();
        if (iter.matchFlag(optHelp, "help"))
            continue;
        if (*arg!='-')
        {
            cmd.set(arg);
            iter.next();
            break;
        }
        if (iter.matchFlag(boolValue, "--version"))
        {
            fprintf(stdout, "%s\n", BUILD_TAG);
            return false;
        }
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
        }
    }
    return true;
}

//=========================================================================================

void CLICMDShell::usage()
{
    fprintf(stdout,"\nUsage:\n"
        "    ecl [--version] <command> [<args>]\n\n"
           "Commonly used commands:\n"
           "   deploy    create an HPCC workunit from a local archive or shared object\n"
           "   publish   add an HPCC workunit to a query set\n"
           "\nRun 'ecl help <command>' for more information on a specific command\n\n"
    );
}
