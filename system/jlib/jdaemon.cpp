#include <cstdio>
#include "build-config.h"
#include "jstring.hpp"
#include "jlog.hpp"
#include "jfile.hpp"
#include "jargv.hpp"
#include "jprop.hpp"
#include "jdaemon.hpp"


int DaemonCMDShell::run()
{
    try
    {
        if (!parseCommandLineOptions(args))
            return 1;

        //return processCMD(args);
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

bool DaemonCMDShell::parseCommandLineOptions(ArgvIterator &iter)
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
        if (iter.matchFlag(optHelp, "--help") || iter.matchFlag(optHelp, "-h"))
        {
            usage();
            return false;
        }
        if (iter.matchFlag(boolValue, "--version"))
        {
            fprintf(stdout, "%s\n", BUILD_TAG);
            return false;
        }
        if (iter.matchOption(optEnv, DAEMONOPT_ENV) || iter.matchOption(optEnv, DAEMONOPT_ENV_LONG))
        {

        }
        if (iter.matchOption(optName, DAEMONOPT_NAME) || iter.matchOption(optName, DAEMONOPT_NAME_LONG))
        {

        }
        if (iter.matchFlag(optForeground, DAEMONOPT_FOREGROUND) || iter.matchFlag(optForeground, DAEMONOPT_FOREGROUND_LONG))
        {

        }
    }
    if(!optName)
    {
        fprintf(stderr, "\nComponent name not passed. (--name or -n is required)\n");
        usage();
        return false;
    }
    if(!optEnv)
    {
        optEnv.set(DAEMONOPT_ENV_DEFAULT);
    }
    return true;
}

//=========================================================================================

void DaemonCMDShell::usage()
{
    fprintf(stdout,"\nUsage:\n"
        "    %s [--version] [<args>]\n\n"
           "Defaults:\n"
            "      Environment = %s\n\n\n"
           "Required:\n"
            "      --name=<name> , -n=<name>        name of the component to use.\n\n"
           "Optional:\n"
            "      --help, -h                       Display help.\n"
            "      --env=<xml file>, -e=<xml file>  environment xml file to use.\n"
            "      --foreground, -f                 Run in foreground.\n\n", name.str(), DAEMONOPT_ENV_DEFAULT
    );
}
