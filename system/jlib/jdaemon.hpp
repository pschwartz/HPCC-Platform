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
    void (*sighup)(int); //TODO: implement usage of sighup, placeholder at this time.
};

interface IDaemonFile : extends IInterface
{
    virtual void create() = 0;
    virtual void remove() = 0;
    virtual void truncate() = 0;
    virtual size32_t size() = 0;
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

    virtual void setName(StringAttr name) = 0;  // Set component name passed from CLI.
    virtual void setLockFile(StringAttr name) = 0; // Set component LockFile name.
    virtual void setPidFile(StringAttr name) = 0; // Set component PidFile name.
    virtual void setEnvHash(StringAttr EnvFile) = 0; // Set component env file passed from CLI.
    virtual bool checkEnvHash() = 0; // Check if running daemon's env passes current env.

};

class CDaemonFile : public CInterface, implements IDaemonFile
{
public:
    IMPLEMENT_IINTERFACE;
    void create();
    void remove();
    void truncate();
    size32_t size();
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
    unsigned getPid();
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
    bool daemonize(); // Spawn Daemon
    bool isRunning();
    void setName(StringAttr name);
    void setLockFile(StringAttr name);
    void setPidFile(StringAttr name);
    void setEnvHash(StringAttr EnvFile);
    bool checkEnvHash();
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

#ifdef _USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>
#define ASSERT(a) { if (!(a)) CPPUNIT_ASSERT(a); }

class CEnvHashTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( CEnvHashTest  );
        CPPUNIT_TEST(test_hashEnv);
        CPPUNIT_TEST(test_setHash);
        CPPUNIT_TEST(test_getHash);
        CPPUNIT_TEST(test_compareHash);
    CPPUNIT_TEST_SUITE_END();
private:
    Owned<IEnvHash> h_1, h_2;
    Owned<IFile> file;
    Owned<IFileIO> fileio;
public:
    void setUp();
    void tearDown();
    void test_hashEnv();
    void test_setHash();
    void test_getHash();
    void test_compareHash();
};

void CEnvHashTest::setUp()
{
    file.setown(createIFile("test"));
    fileio.setown(file->open(IFOreadwrite));
    fileio->write(0, 4, "test");
    h_1.setown(createIEnvHash());
    h_2.setown(createIEnvHash());
    h_1->hashEnv("test");
    h_2->hashEnv("test");
}

void  CEnvHashTest::tearDown()
{

}

void CEnvHashTest::test_hashEnv()
{
    ASSERT(h_1->compareHash(h_2));
}

void CEnvHashTest::test_setHash()
{
    StringBuffer hash;
    h_1->getHash(hash);
    h_2->setHash(hash);
    ASSERT(h_2->compareHash(hash));
}

void CEnvHashTest::test_getHash()
{
    StringBuffer hash, hashB;
    h_1->getHash(hash);
    h_1->getHash(hashB);
    ASSERT(hash == hashB);
}

void CEnvHashTest::test_compareHash()
{
    ASSERT(h_1->compareHash(h_2));
}

CPPUNIT_TEST_SUITE_REGISTRATION( CEnvHashTest );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( CEnvHashTest, "CEnvHashTest" );

#endif

#endif
