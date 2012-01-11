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
#include <cerrno>
#include <sys/types.h>
#include <sys/inotify.h>

#include "jhash.hpp"
#include "jsuperhash.hpp"
#include "jlog.hpp"
#include "jfile.hpp"
#include "jnotify.hpp"

/**
 * CNotify impl
 */
/******************************************************************/
CNotify::CNotify()
{
    inWatch = inotify_init();
    if ( inWatch < 0 )
    {
        ERRLOG("Failed to open inotify file descriptor in Kernel.");
        throw MakeStringException(1, "Failed to open inotify file descriptor in Kernel.");
    }
}

CNotify::~CNotify()
{
    if( watchMap.count() != 0 )
    {
        HashIterator iter(watchMap);
        ForEach(iter)
        {
            WARNLOG("Closing inotify descriptor for %s", (const char*)iter.query().getKey());
            closefdWatch(iter.query().getHash());
        }
    }
    close(inWatch);
}

bool CNotify::add(const char* file)
{
    IMapping *hChk = find(file);
    if ( hChk )
    {
        ERRLOG("Attempted to add file already in watch list.");
        return false;
    }
    WARNLOG("Added path: %s", file);
    int wid = inotify_add_watch(inWatch, file, IN_MODIFY | IN_CREATE | IN_DELETE );
    return watchMap.setValue(file,wid);
}

void CNotify::closefdWatch(int wid)
{
    inotify_rm_watch(inWatch, wid);
}

bool CNotify::remove(const char* file)
{
    IMapping *hChk = find(file);
    if ( !hChk )
    {
        ERRLOG("Attempted to remove file not in watch list.");
        return false;
    }
    closefdWatch(hChk->getHash());
    WARNLOG("Removed path: %s", (char*)hChk->getKey());
    return watchMap.remove((const char*)hChk->getKey());
}

IMapping *CNotify::find(const char* file)
{
    return watchMap.find(file);
}


bool CNotify::addPath(const char* file)
{
    Owned<IFile> _file = createIFile(file);
    if ( _file->isFile() == notFound && _file->isDirectory() == notFound)
    {
        WARNLOG("Attempted to add file or directory that does not exist.");
        //TODO: impl action if file or dir does not exist.
        return false;
    }
    else if ( _file->isFile() )
    {
        StringBuffer dir;
        splitFilename(_file->queryFilename(), &dir, &dir, NULL, NULL);
        Owned<IFile> dP = createIFile(dir);
        bool addedDir = addPath(dP->queryFilename());
        bool addedFile = add(_file->queryFilename());
        if( addedDir && addedFile )
        {
            return true;
        }
        else
        {
            ERRLOG("Could not add file and directory.");
            remove(dP->queryFilename());
            remove(_file->queryFilename());
            return false;
        }
    }
    else if ( _file->isDirectory() )
    {
        return add(_file->queryFilename());
    }
}

bool CNotify::removePath(const char* file)
{
    return remove(file);
}

bool CNotify::startWatch()
{
    //TODO: impl watcher code that spawns a thread.
}

bool CNotify::endWatch()
{
    //TODO: impl watcher cleanup on event found.
}

/******************************************************************/

/**
 * CNotifyThread impl
 */
/******************************************************************/
CNotifyThread::CNotifyThread(INotify *watcher) : Thread("inotify Watchdog Thread")
{
    started = false;
    //inWatch = watcher->getWatcher();
}

CNotifyThread::~CNotifyThread()
{
    if (started)
        stop();
}

void CNotifyThread::start()
{
    if (started)
    {
        WARNLOG("START called when already started\n");
        assert(false);
    }
    started = true;
    Thread::start();
}

void CNotifyThread::stop()
{
    if (started)
        started = false;
}

int CNotifyThread::run()
{
    while(started)
    {
        int length, i = 0;
        char buffer[BUF_LEN];
        length = read(inWatch, buffer, BUF_LEN);
        if ( length < 0 )
        {
            ERRLOG("Cannot read from inotify file descriptor.");
            throw MakeStringException(1, "Cannot read from inotify file descriptor.");
        }
        while ( i < length) {
            struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
            if ( event->len ) {
                if ( event->mask && IN_ALL_EVENTS ) {
                    WARNLOG( "%s was %d.", event->name, event->mask );
                }
            }
            i += EVENT_SIZE + event->len;
        }
    }
}

extern INotify* createINotify()
{
    return new CNotify();
}

extern CNotifyThread* createCNotifyThread(INotify* watcher)
{
    return new CNotifyThread(watcher);
}

/******************************************************************/
