#ifndef JDAEMON_HPP
#define JDAEMON_HPP

#include "jlog.hpp"
#include "jfile.hpp"
#include "jargv.hpp"
#include "jprop.hpp"
#include "build-config.h"

#define DAEMONOPT_ENV "-e"
#define DAEMONOPT_ENV_LONG "--environment"
#define DAEMONOPT_ENV_DEFAULT "/etc/HPCCSystems/environment.xml"
#define DAEMONOPT_NAME "-n"
#define DAEMONOPT_NAME_LONG "--name"
#define DAEMONOPT_FOREGROUND "-f"
#define DAEMONOPT_FOREGROUND_LONG "--foreground"

struct sighandler
{
    void (*sigterm)(int);
    void (*sigint)(int);
    void (*sighup)(int);
};

interface iDaemon
{
    virtual bool daemonize() = 0;
    virtual bool checkLock() = 0;
    virtual bool getLock() = 0;
    virtual bool releaseLock() = 0;
    virtual void setName(StringAttr name) = 0;
    virtual void setLockFile(StringAttr name) = 0;
    virtual void setPidFile(StringAttr name) = 0;
    virtual void setEnvHash(StringAttr EnvFile) = 0;
    virtual bool checkEnvHash() = 0;
    virtual bool isRunning() = 0;
};

class DaemonCMDShell
{
public:
    DaemonCMDShell(int argc, const char *argv[], const char *_version)
        : args(argc, argv), version(_version), optHelp(false)
    {
        splitFilename(argv[0], NULL, NULL, &name, NULL);
    }

    bool parseCommandLineOptions(ArgvIterator &iter);
    int run();

    virtual void usage();

protected:
    ArgvIterator args;
    StringBuffer name;

    StringAttr version;

    bool optHelp;
    StringAttr optEnv;
    StringAttr optName;
    bool optForeground;
};

#endif
