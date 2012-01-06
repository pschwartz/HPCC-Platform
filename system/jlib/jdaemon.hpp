/*##############################################################################

    Copyright (C) 2011 HPCC Systems.

    All rights reserved. This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
############################################################################## */

#ifndef JDAEMON_HPP
#define JDAEMON_HPP

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

interface iDaemonSupportFile : extends IInterface
{
    virtual void create() = 0;
    virtual void truncate() = 0;
    virtual void read(StringBuffer &data) = 0;
    virtual void write(StringBuffer data) = 0;
    virtual IFileIO *getIFileIO() = 0;
};

class CDaemonSupportFile : public CInterface, implements iDaemonSupportFile
{
public:
    IMPLEMENT_IINTERFACE;
    void create();
    void truncate();
    void read(StringBuffer &data);
    void write(offset_t off, size32_t len, void *data);
    IFileIO *getIFileIO();

protected:
    Owned<IFile> supportFile;
    Owned<IFileIO> supportFileIO;
};


class CLockFile : public CDaemonSupportFile
{
public:
    CLockFile(IFile *_lockFile);
    CLockFile(StringAttr lockFilename);
    bool islocked();
    bool lock();
    bool unlock();
    void setHash(StringBuffer hash);
    void clearHash();

private:
    Owned<IDiscretionaryLock> dLock;

};

class CPidFile : public CDaemonSupportFile
{
public:
    CPidFile(IFile *_pidFile);
    CPidFile(StringAttr pidFilename);
    void setPid(int pid);
    void clearPid();

};

interface iEnvHash : extends IInterface
{
    virtual void hashEnv(StringAttr env) = 0;
    virtual bool compareHash(StringBuffer hash) = 0;
};

interface iDaemon : extends IInterface
{
    virtual bool daemonize() = 0; // Spawn Daemon
    virtual bool isRunning() = 0;

    virtual void setName(StringAttr name) = 0;
    virtual void setLockFile(StringAttr name) = 0;
    virtual void setPidFile(StringAttr name) = 0;
    virtual void setEnvHash(StringAttr EnvFile) = 0;
    virtual bool checkEnvHash() = 0;

};

class CDaemon : public CInterface, implements iDaemon
{
public:
    IMPLEMENT_IINTERFACE;
};


class DaemonCMDShell
{
public:
    DaemonCMDShell(int argc, const char *argv[], const char *_version)
        : args(argc, argv), version(_version), optHelp(false), optForeground(false)
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
