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

#include <cstdio>
#include <cstdlib>
#include "build-config.h"
#include "jstring.hpp"
#include "jlog.hpp"
#include "jfile.hpp"
#include "jargv.hpp"
#include "jprop.hpp"
#include "jargv.hpp"
#include "jmd5.hpp"
#include "jdaemon.hpp"


/**
 * CDaemonFile impl
 */
/******************************************************************/
void CDaemonFile::create()
{
    if (supportFile && !supportFile->exists())
    {
        supportFileIO->write(0,0,"");
    }
}

void CDaemonFile::remove()
{
    supportFile->remove();
}

void CDaemonFile::truncate()
{
    supportFileIO->setSize(0);
}

void CDaemonFile::read(offset_t off, size32_t len, void *data)
{
    supportFileIO->read(off, len, data);
}

void CDaemonFile::write(offset_t off, size32_t len, void *data)
{
    truncate();
    supportFileIO->write(off, len, data);
}

IFileIO *CDaemonFile::getIFileIO()
{
    return supportFileIO;
}

/******************************************************************/


/**
 * CLockFile impl
 */
/******************************************************************/
CLockFile::CLockFile(IFile *_lockFile)
{
    supportFile.setown(_lockFile);
    supportFileIO.setown(supportFile->openShared(IFOreadwrite, IFSHnone));
    dLock.setown(createDiscretionaryLock(supportFileIO));
    create();
}

CLockFile::CLockFile(StringAttr lockFilename)
{
    supportFile.setown(createIFile(lockFilename));
    supportFileIO.setown(supportFile->openShared(IFOreadwrite, IFSHnone));
    dLock.setown(createDiscretionaryLock(supportFileIO));
    create();
}

bool CLockFile::islocked()
{
    return dLock->isExclusiveLocked();
}

bool CLockFile::lock()
{
    dLock->lock(true, 500);
    return islocked();
}

bool CLockFile::unlock()
{
    dLock->unlock();
    return islocked();
}

IEnvHash *CLockFile::getHash()
{
    StringBuffer hash;
    read(0,32,hash.reserve(32));
    ihash.setown(createIEnvHash());
    ihash->setHash(hash);
    return ihash;
 }

void CLockFile::setHash(IEnvHash *hash)
{
    StringBuffer hashStr;
    hash->getHash(hashStr);
	write(0, hashStr.length(), (char*) hashStr.str());
}

void CLockFile::clearHash()
{
	truncate();
}

/******************************************************************/

/**
 * CPidFile impl
 */
/******************************************************************/
CPidFile::CPidFile(IFile *_pidFile)
{
    supportFile.setown(_pidFile);
    supportFileIO.setown(supportFile->openShared(IFOcreaterw, IFSHnone));
    if(!supportFile->exists())
        supportFileIO->write(0,0, "");
}

CPidFile::CPidFile(StringAttr pidFilename)
{
    supportFile.setown(createIFile(pidFilename));
    supportFileIO.setown(supportFile->openShared(IFOcreaterw, IFSHnone));
    if(!supportFile->exists())
        supportFileIO->write(0,0, "");
}

void CPidFile::setPid(unsigned pid)
{
    StringBuffer pidStr;
    pidStr.append(pid);
    write(0,pidStr.length(), pidStr.detach());
}

void CPidFile::clearPid()
{
	truncate();
}

/******************************************************************/

/**
 * CEnvHash impl
 */
/******************************************************************/

void CEnvHash::hashEnv(const char* env)
{
    if(env)
    {
        md5_filesum(env, hash);
    }
    else
    {
        StringBuffer env;
        env.append(CONFIG_DIR).append(PATHSEPCHAR).append(ENV_XML_FILE);
        md5_filesum(env.str(), hash);
    }
}

void CEnvHash::setHash(StringBuffer &_hash)
{
    hash = _hash;
}

void CEnvHash::getHash(StringBuffer &_hash)
{
    _hash = hash;
}

bool CEnvHash::compareHash(StringBuffer _hash)
{
    return streq(hash,_hash);
}

bool CEnvHash::compareHash(IEnvHash *_hash)
{
    StringBuffer _hashData;
    _hash->getHash(_hashData);
    return compareHash(_hashData);
}

/******************************************************************/

/**
 * DaemonCMDShell impl
 */
/******************************************************************/
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
/******************************************************************/

extern IEnvHash * createIEnvHash()
{
    return new CEnvHash();
}

extern CLockFile * createLockFile(IFile *_lockFile)
{
    return new CLockFile(_lockFile);
}

extern CLockFile * createLockFile(StringAttr lockFilename)
{
    return new CLockFile(lockFilename);
}

extern CPidFile * createPidFile(IFile *_pidFile)
{
    return new CPidFile(_pidFile);
}

extern CPidFile * createPidFile(StringAttr pidFilename)
{
    return new CPidFile(pidFilename);
}
