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

interface IDaemonFile : extends IInterface
{
    virtual void create() = 0;
    virtual void remove() = 0;
    virtual void truncate() = 0;
    virtual void read(offset_t off, size32_t len, void *data) = 0;
    virtual void write(offset_t off, size32_t len, void *data) = 0;
    virtual IFileIO *getIFileIO() = 0;
};

interface IEnvHash : extends IInterface
{
    virtual void hashEnv(const char* env=NULL) = 0;
    virtual void setHash(StringBuffer &_hash) = 0;
    virtual void getHash(StringBuffer &_hash) = 0;
    virtual bool compareHash(StringBuffer _hash) = 0;
    virtual bool compareHash(IEnvHash *_hash) = 0;
};

interface IDaemon : extends IInterface
{
    virtual bool daemonize() = 0; // Spawn Daemon
    virtual bool isRunning() = 0;

    virtual void setName(StringAttr name) = 0;
    virtual void setLockFile(StringAttr name) = 0;
    virtual void setPidFile(StringAttr name) = 0;
    virtual void setEnvHash(StringAttr EnvFile) = 0;
    virtual bool checkEnvHash() = 0;

};

class CDaemonFile : public CInterface, implements IDaemonFile
{
public:
    IMPLEMENT_IINTERFACE;
    void create();
    void remove();
    void truncate();
    void read(offset_t off, size32_t len, void *data);
    void write(offset_t off, size32_t len, void *data);
    IFileIO *getIFileIO();

protected:
    Owned<IFile> supportFile;
    Owned<IFileIO> supportFileIO;
};

class CLockFile : public CDaemonFile
{
public:
    CLockFile(IFile *_lockFile);
    CLockFile(StringAttr lockFilename);
    bool islocked();
    bool lock();
    bool unlock();
    IEnvHash *getHash();
    void setHash(IEnvHash *hash);
    void clearHash();

private:
    Owned<IDiscretionaryLock> dLock;
    Owned<IEnvHash> ihash;
};

class CPidFile : public CDaemonFile
{
public:
    CPidFile(IFile *_pidFile);
    CPidFile(StringAttr pidFilename);
    void setPid(unsigned pid);
    void clearPid();
};

class CEnvHash : public CInterface, implements IEnvHash
{
public:
    IMPLEMENT_IINTERFACE;
    void hashEnv(const char* env=NULL);
    void setHash(StringBuffer &_hash);
    void getHash(StringBuffer &_hash);
    bool compareHash(StringBuffer _hash);
    bool compareHash(IEnvHash *_hash);

private:
    StringBuffer hash;
};

class CDaemon : public CInterface, implements IDaemon
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


extern IEnvHash * createIEnvHash();
extern CLockFile * createLockFile(IFile *_lockFile);
extern CLockFile * createLockFile(StringAttr lockFilename);
extern CPidFile * createPidFile(IFile *_pidFile);
extern CPidFile * createPidFile(StringAttr pidFilename);


#endif
