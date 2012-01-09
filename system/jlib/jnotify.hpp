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

#ifndef JNOTIFY_HPP_
#define JNOTIFY_HPP_

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

enum INOmode { INOmodify, INOcreate, INOdelete, INOmodcre, INOmoddel, INOcredel, INOall };

interface INotify : extends IInterface
{
    virtual bool addWatch(const char* path) = 0;
    virtual bool removeWatch(const char* path) = 0;
    virtual bool isWatched(const char* path) = 0;
    virtual int *getWatched(const char* path) = 0;

    virtual bool addWatch(StringBuffer* path) = 0;
    virtual bool removeWatch(StringBuffer* path) = 0;
    virtual bool isWatched(StringBuffer* path) = 0;
    virtual int *getWatched(StringBuffer* path) = 0;

    virtual bool addWatch(IFile* file) = 0;
    virtual bool removeWatch(IFile* file) = 0;
    virtual bool isWatched(IFile* file) = 0;
    virtual int *getWatched(IFile* file) = 0;

    virtual int getWatcher() = 0;
};

class CNotify : public CInterface, implements INotify
{
public:
    IMPLEMENT_IINTERFACE;
    CNotify();
    ~CNotify();
    bool addWatch(const char* path);
    bool removeWatch(const char* path);
    bool isWatched(const char* path);
    int *getWatched(const char* path);

    bool addWatch(StringBuffer* path);
    bool removeWatch(StringBuffer* path);
    bool isWatched(StringBuffer* path);
    int *getWatched(StringBuffer* path);

    bool addWatch(IFile* file);
    bool removeWatch(IFile* file);
    bool isWatched(IFile* file);
    int *getWatched(IFile* file);

    int getWatcher();

private:
    bool add(const char* path, int wfd);
    bool remove(const char* path);
    int *get(const char* path);

private:
    int inWatch;
    MapStringTo<int> watchMap;
};

class CNotifyThread : public Thread
{
public:
    CNotifyThread(INotify *watcher);
    ~CNotifyThread();

    void start();
    void stop();

private:
    int run();
    bool started;
    int inWatch;
};


extern INotify* createINotify();
extern CNotifyThread* createCNotifyThread(INotify* watcher);

#endif /* JNOTIFY_HPP_ */
