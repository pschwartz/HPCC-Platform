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
    close(inWatch);
}

bool CNotify::add(const char* path, int wfd)
{
    int *hfd = watchMap.getValue(path);
    if ( !hfd )
    {
        return watchMap.setValue(path, wfd);
    }
    return false;
}

bool CNotify::remove(const char* path)
{
    int *hfd = watchMap.getValue(path);
    if ( hfd )
    {
        return watchMap.remove(path);
    }
    return false;
}

int *CNotify::get(const char* path)
{
    int *hfd = watchMap.getValue(path);
    if ( hfd )
    {
        return hfd;
    }
    return NULL;
}

bool CNotify::addWatch(const char* path)
{
    WARNLOG("Adding Path: %s", path);
    int wd = inotify_add_watch(inWatch, path, IN_MODIFY|IN_CREATE|IN_DELETE);
    return add(path, wd);
}

bool CNotify::addWatch(StringBuffer* path)
{
    return addWatch(path->str());
}

bool CNotify::addWatch(IFile* file)
{
    return addWatch(file->queryFilename());
}

bool CNotify::removeWatch(const char* path)
{
    return remove(path);
}

bool CNotify::removeWatch(StringBuffer* path)
{
    return removeWatch(path->str());
}

bool CNotify::removeWatch(IFile* file)
{
    return removeWatch(file->queryFilename());
}

bool CNotify::isWatched(const char* path)
{
    int *val = get(path);
    if( val )
        return true;
    return false;
}

bool CNotify::isWatched(StringBuffer* path)
{
    return isWatched(path->str());
}

bool CNotify::isWatched(IFile* file)
{
    return isWatched(file->queryFilename());
}

int *CNotify::getWatched(const char* path)
{
    if(isWatched(path))
        return get(path);
    return NULL;
}

int *CNotify::getWatched(StringBuffer* path)
{
    return getWatched(path->str());
}

int *CNotify::getWatched(IFile* file)
{
    return getWatched(file->queryFilename());
}

int CNotify::getWatcher()
{
    return inWatch;
}

/******************************************************************/

/**
 * CNotifyThread impl
 */
/******************************************************************/
CNotifyThread::CNotifyThread(INotify *watcher) : Thread("inotify Watchdog Thread")
{
    started = false;
    inWatch = watcher->getWatcher();
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
                if ( event->mask && IN_CREATE ) {
                    if ( event->mask && IN_ISDIR ) {
                        WARNLOG( "The directory %s was created.\n", event->name );
                    }
                    else {
                        WARNLOG( "The file %s was created.\n", event->name );
                    }
                }
                else if ( event->mask && IN_DELETE ) {
                    if ( event->mask && IN_ISDIR ) {
                        WARNLOG( "The directory %s was deleted.\n", event->name );
                    }
                    else {
                        WARNLOG( "The file %s was deleted.\n", event->name );
                    }
                }
                else if ( event->mask && IN_MODIFY ) {
                    if ( event->mask && IN_ISDIR ) {
                        WARNLOG( "The directory %s was modified.\n", event->name );
                    }
                    else {
                        WARNLOG( "The file %s was modified.\n", event->name );
                    }
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
