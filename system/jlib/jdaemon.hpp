#include "build-config.h"
#include "jlib.hpp"
#include "jmd5.hpp"

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

interface ICLICommand : extends IInterface
{
    virtual bool parseCommandLineOptions(ArgvIterator &iter)=0;
    virtual bool finalizeOptions(IProperties *globals)=0;
    virtual int processCMD()=0;
    virtual void usage()=0;
};

typedef ICLICommand *(*CLICommandFactory)(const char *cmdname);

#define CLIOPT_ENV "--env"
#define CLIOPT_ENV_DEFAULT "/etc/HPCCSystems/environment.xml"
#define CLIOPT_NAME "--name"
#define CLIOPT_VERSION "--version"
#define CLIOPT_FOREGROUND "-f"


bool extractCLICmdOption(StringBuffer & option, IProperties * globals, const char * envName, const char * propertyName, const char * defaultPrefix, const char * defaultSuffix);
bool extractCLICmdOption(StringAttr & option, IProperties * globals, const char * envName, const char * propertyName, const char * defaultPrefix, const char * defaultSuffix);
bool extractCLICmdOption(bool & option, IProperties * globals, const char * envName, const char * propertyName, bool defval);
bool extractCLICmdOption(unsigned & option, IProperties * globals, const char * envName, const char * propertyName, unsigned defval);

enum CLICmdOptionMatchIndicator
{
    CLICmdOptionNoMatch=0,
    CLICmdOptionMatch=1,
    CLICmdOptionCompletion=2
};

class CLICmdCommon : public CInterface, implements ICLICommand
{
public:
    IMPLEMENT_IINTERFACE;
    CLICmdCommon()
    {
    }
    virtual CLICmdOptionMatchIndicator matchCommandLineOption(ArgvIterator &iter, bool finalAttempt=false);
    virtual bool finalizeOptions(IProperties *globals);

    virtual void usage()
    {
        fprintf(stdout,
            "      --env=<environment.xml>        environment xml file to use.\n"
            "      --name=<name>      name of the component to use.\n"
        );
    }
public:
    StringAttr optEnv;
    StringAttr optName;
    bool optForeground;
};
