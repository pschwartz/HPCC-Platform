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
#include "build-config.h"
#include "jstring.hpp"
#include "jlog.hpp"
#include "jfile.hpp"
#include "jargv.hpp"
#include "jprop.hpp"
#include "jargv.hpp"
#include "jdaemon.hpp"


/**
 * CDaemonSupportFile impl
 */
/******************************************************************/
void CDaemonSupportFile::create()
{
    if (supportFile && !supportFile->exists())
    {
        supportFileIO->write(0,0,"");
    }
}

void CDaemonSupportFile::truncate()
{
    supportFileIO->setSize(0);
}

void CDaemonSupportFile::read(StringBuffer &data)
{
    data.loadFile(supportFile);
}

void CDaemonSupportFile::write(StringBuffer data)
{
    truncate();
    supportFileIO->write(0, data.length(), data);
}

IFileIO *CDaemonSupportFile::getIFileIO()
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
