#ifndef JDAEMON_HPP
#define JDAEMON_HPP

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


#endif
