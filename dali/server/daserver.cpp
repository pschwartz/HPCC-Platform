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

#include "build-config.h"
#include "platform.h"
#include "thirdparty.h"
#include "jlib.hpp"
#include "jlog.ipp"
#include "jptree.hpp"
#include "jmisc.hpp"

#include "mpbase.hpp"
#include "mpcomm.hpp"
#include "mplog.hpp" //MORE: deprecated feature, not used by any new clients, remove once all deployed clients that depend on it are upgraded
#include "rmtfile.hpp"
#include "dacoven.hpp"
#include "dadfs.hpp"
#include "dasess.hpp"
#include "daaudit.hpp"
#include "dasds.hpp"
#include "daclient.hpp"
#include "dasubs.ipp"
#include "danqs.hpp"
#include "dadiags.hpp"

#ifdef _DEBUG
//#define DALI_MIN
#endif

#ifdef DALI_MIN
#define _NO_LDAP
#endif


#include "daserver.hpp"
#ifndef _NO_LDAP
#include "daldap.hpp"
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <pwd.h>
#endif
#include "jfile.hpp"
#include "jargv.hpp"
#include "deployutils.hpp"

#include <map>

using namespace std;


Owned<IPropertyTree> serverConfig;
static IArrayOf<IDaliServer> servers;
static CriticalSection *stopServerCrit;
MODULE_INIT(INIT_PRIORITY_DALI_DASERVER)
{
    stopServerCrit = new CriticalSection;
    return true;
}
MODULE_EXIT()
{
    servers.kill(); // should already be clear when stopped
    serverConfig.clear();
    delete stopServerCrit;
}

ILogMsgHandler * fileMsgHandler;

#define DEFAULT_PERF_REPORT_DELAY 60
#define DEFAULT_MOUNT_POINT "/mnt/dalimirror/"

void setMsgLevel(unsigned level)
{
    ILogMsgFilter *filter = getSwitchLogMsgFilterOwn(getComponentLogMsgFilter(3), getCategoryLogMsgFilter(MSGAUD_all, MSGCLS_all, level, true), getDefaultLogMsgFilter());
    queryLogMsgManager()->changeMonitorFilter(queryStderrLogMsgHandler(), filter);
    queryLogMsgManager()->changeMonitorFilterOwn(fileMsgHandler, filter);
}

void AddServers(const char *auditdir)
{
    // order significant
    servers.append(*createDaliSessionServer());
    servers.append(*createDaliPublisherServer());
    servers.append(*createDaliSDSServer(serverConfig));
    servers.append(*createDaliNamedQueueServer());
    servers.append(*createDaliDFSServer());
    servers.append(*createDaliAuditServer(auditdir));
    servers.append(*createDaliDiagnosticsServer());
    // add new coven servers here
}

static bool serverStopped = false;
static void stopServer()
{
    CriticalBlock b(*stopServerCrit); // NB: will not protect against abort handler, which will interrupt thread and be on same TID.
    if (serverStopped) return;
    serverStopped = true;
    ForEachItemInRev(h,servers)
    {
        IDaliServer &server=servers.item(h);
        LOG(MCprogress, unknownJob, "Suspending %d",h);
        server.suspend();
    }
    ForEachItemInRev(i,servers)
    {
        IDaliServer &server=servers.item(i);
        LOG(MCprogress, unknownJob, "Stopping %d",i);
        server.stop();
    }
    closeCoven();
    ForEachItemInRev(j,servers)
    {
        servers.remove(j);      // ensure correct order for destruction
    }
    stopLogMsgReceivers(); //MORE: deprecated feature, not used by any new clients, remove once all deployed clients that depend on it are upgraded
    stopMPServer();
}

bool actionOnAbort()
{
    stopServer();
    return true;
} 

#ifdef _WIN32
class CReleaseMutex : public CInterface, public Mutex
{
public:
    CReleaseMutex(const char *name) : Mutex(name) { }
    ~CReleaseMutex() { if (owner) unlock(); }
}; 
#endif

#ifndef _WIN32
void sighandler(int signum, siginfo_t *info, void *extra)
{
    PROGLOG("Caught signal %d, %p", signum, info?info->si_addr:0);
    stopServer();
    stopPerformanceMonitor();
    exit(0);
}

int initDaemon()
{
    int ret = make_daemon(true);
    if (ret)
        return ret;

    struct sigaction act;
    sigset_t blockset;
    sigemptyset(&blockset);
    act.sa_mask = blockset;
    act.sa_handler = SIG_IGN;
    sigaction(SIGHUP, &act, NULL);

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &sighandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    return 0;
}
#endif

USE_JLIB_ALLOC_HOOK;

class CCLIOptionCommon{
public:
    virtual const char* getName() = 0;
    virtual const char* getDescription() = 0;
    virtual const char* getDefValue() = 0;
};

template <typename T>
class  CCLIOption : public CCLIOptionCommon{
private:
    T *optType;
    const char* optName;
    const char* optDescription;
    const char* optDefValue;

public:
    CCLIOption(const char* name,
               const char* description,
               const char* defValue):
                optName(name),
                optDescription(description),
                optDefValue(defValue)
    {
        optType = new T();
    }

    T getType()
    {
        return &optType;
    }

    const char* getName()
    {
        return optName;
    }

    const char* getDescrption()
    {
        return optDescription;
     }

    const char* getDefValue()
    {
        return optDefValue;
    }

};

class CCLIOptionFactory
{
public:
    template <typename T>
    static CCLIOption<T> * createCLIOption(const char* name, const char* description, const char* defValue)
    {
        CCLIOption<T> *option = new CCLIOption<T>(name, description, defValue);
        return option;
    }
};

class CCLI
{
public:
    template <typename T>
    void addOption(CCLIOption<T> option)
    {
            this->optMap.insert(option.getPair());
    }
    void generateHelp(){}

private:
    typedef std::map<const char*, CCLIOptionCommon > CLIOptMap;
    CLIOptMap optMap;
};







class CDaemonCLI
{
private:
    ArgvIterator iter;
    StringBuffer name;
    StringAttr optEnvFile;
    StringAttr optCompName;
    bool optForeground;

public:
    CDaemonCLI(int argc, const char* argv[]): name(argv[0]),iter(argc, argv)
    {
        StringBuffer envFile = StringBuffer().append(CONFIG_DIR).append(PATHSEPSTR).append(ENV_XML_FILE);
        optEnvFile.set(envFile.str());
        optForeground = false;
        optCompName.set(NULL);
    }

    void usage()
    {
        fprintf(stdout, "\nUsage:\n"
            "    %s <options>\n"
            "\nRequired Options:\n"
            "  --name=<compName>  Name of the component to start.\n"
            "\nOptional Options:\n"
            "  --env=<EnvFile>    Environment file to use.\n"
            "  -f                 Run in foreground.\n"
            "  --version           Output version information.\n",
            name.str());
    }

    bool inForeground()
    {
        return optForeground;
    }

    const char* getEnvFile()
    {
        return optEnvFile;
    }

    const char* getCompName()
    {
        return optCompName;
    }

    bool parseCommandLineOptions()
    {
        if (iter.done())
        {
            usage();
            return false;
        }
        bool boolValue;
        for(; !iter.done(); iter.next())
        {
            const char * arg = iter.query();
            if ( iter.matchOption(optEnvFile, "--env"))
            {
                if(!checkFileExists(optEnvFile))
                {
                    return false;
                }
            }
            else if( iter.matchOption(optCompName, "--name"))
            {
            }
            else if( iter.matchFlag(optForeground, "-f"))
            {
                optForeground = true;
            }
            else if (strcmp(arg, "--help")==0)
            {
                usage();
                return false;
            }
            else if (iter.matchFlag(boolValue, "--version"))
            {
                fprintf(stdout, "%s\n", BUILD_TAG);
                return false;
            }
        }

        if (getCompName() == NULL)
        {
            usage();
            return false;
        }
        return true;
    }
};

class CDaemonInfo{
private:

    Owned<IPropertyTree> serverEnv;

    void openEnv(const char* envFile)
    {
        OwnedIFile envIFile = createIFile(envFile);
        if ( envIFile->exists())
        {
            serverEnv.setown(createPTreeFromXMLFile(envFile));
        }
    }

#ifndef _WIN32
    passwd pwd;
    void getPW_PWD(const char* username)
    {
        struct passwd *result;
        char *buf;
        size_t bufsize;
        int s;

        bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize == -1)          /* Value was indeterminate */
            bufsize = 16384;        /* Should be more than enough */

        buf = (char*)malloc(bufsize);
        if (buf == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        s = getpwnam_r(username, &pwd, buf, bufsize, &result);
        if (result == NULL)
        {
            if (s == 0)
            {
                printf("Not found\n");
            }
            else
            {
                errno = s;
                perror("getpwnam_r");
            }
            exit(EXIT_FAILURE);
        }
    }
#else
    void getPW_PWD(const char* username)
    {

    }
#endif

public:
    CDaemonInfo(const char* envFile)
    {
        openEnv(envFile);
        getPW_PWD(getSetting("user"));
    }

    StringBuffer getEnvXML()
    {
        StringBuffer envXML;
        toXML(serverEnv, envXML);
        return envXML;
    }

    Owned<IPropertyTree> getPtree()
    {
        return serverEnv;
    }

#ifndef _WIN32
    uid_t getUID()
    {
        return pwd.pw_uid;
    }

    gid_t getGID()
    {
        return pwd.pw_gid;
    }
#else
    int getUID()
    {
        return 0;
    }

    int getGID()
    {
        return 0;
    }
#endif

    const char* getSetting(const char* setting)
    {
        StringBuffer xpath= StringBuffer().append("EnvSettings/").append(setting);
        return serverEnv->queryProp(xpath.str());
    }
};

int startDali(rank_t &myrank, SocketEndpoint& ep, SocketEndpointArray& epa, CDaemonCLI cli, CDaemonInfo dinfo)
{
    try{
        StringBuffer logName;
        StringBuffer auditDir;
        StringBuffer compname = cli.getCompName();
        generateConfigs(dinfo.getEnvXML(), compname.str(), dinfo.getSetting("runtime"));

        OwnedIFile confIFile = createIFile(DALICONF);
        if (confIFile->exists())
            serverConfig.setown(createPTreeFromXMLFile(DALICONF));

        {
            Owned<IComponentLogFileCreator> lf = createComponentLogFileCreator(serverConfig, "dali");
            lf->setLogDirSubdir("server");//add to tail of config log dir
            lf->setName("DaServer");//override default filename
            lf->beginLogging();
        }

        DBGLOG("Build %s", BUILD_TAG);
        PROGLOG("UID: %d", dinfo.getUID());
        PROGLOG("GID: %d", dinfo.getGID());

        if (serverConfig)
        {
            StringBuffer dataPath;
            if (getConfigurationDirectory(serverConfig->queryPropTree("Directories"),"data","dali",serverConfig->queryProp("@name"),dataPath))
                serverConfig->setProp("@dataPath",dataPath.str());
            else
                serverConfig->getProp("@dataPath",dataPath);
            if (dataPath.length()) {
                RemoteFilename rfn;
                rfn.setRemotePath(dataPath);
                if (!rfn.isLocal()) {
                    ERRLOG("if a dataPath is specified, it must be on local machine");
                    return 0;
                }
                addPathSepChar(dataPath);
                serverConfig->setProp("@dataPath", dataPath.str());
                if (dataPath.length())
                    recursiveCreateDirectory(dataPath.str());
            }

            // JCSMORE remoteBackupLocation should not be a property of SDS section really.
            StringBuffer mirrorPath;
            if (!getConfigurationDirectory(serverConfig->queryPropTree("Directories"),"mirror","dali",serverConfig->queryProp("@name"),mirrorPath))
                serverConfig->getProp("SDS/@remoteBackupLocation",mirrorPath);

            if (mirrorPath.length())
            {
                try
                {
                    addPathSepChar(mirrorPath);
                    serverConfig->setProp("SDS/@remoteBackupLocation", mirrorPath.str());
                    PROGLOG("Checking backup location: %s", mirrorPath.str());

                    try
                    {
                        recursiveCreateDirectory(mirrorPath);
                        StringBuffer backupURL;
                        if (mirrorPath.length()<=2 || !isPathSepChar(mirrorPath.charAt(0)) || !isPathSepChar(mirrorPath.charAt(1)))
                        { // local machine path, convert to url
                            const char *backupnode = serverConfig->queryProp("SDS/@backupComputer");
                            RemoteFilename rfn;
                            if (backupnode&&*backupnode) {
                                SocketEndpoint ep(backupnode);
                                rfn.setPath(ep,mirrorPath.str());
                            }
                            else {
                                WARNLOG("Local path used for backup url: %s", mirrorPath.str());
                                rfn.setLocalPath(mirrorPath.str());
                            }
                            rfn.getRemotePath(backupURL);
                        }
                        else
                            backupURL.append(mirrorPath);
                        addPathSepChar(backupURL);
                        serverConfig->setProp("SDS/@remoteBackupLocation", backupURL.str());
                        PROGLOG("Backup URL = %s", backupURL.str());
                    }
                    catch (IException *e)
                    {
                        EXCLOG(e, "Failed to create remote backup directory, disabling backups", MSGCLS_warning);
                        serverConfig->removeProp("SDS/@remoteBackupLocation");
                        mirrorPath.clear();
                        e->Release();
                    }

                    if (mirrorPath.length())
                    {
#if defined(__linux__)
                        if (serverConfig->getPropBool("@useNFSBackupMount", false))
                        {
                            RemoteFilename rfn;
                            if (mirrorPath.length()<=2 || !isPathSepChar(mirrorPath.charAt(0)) || !isPathSepChar(mirrorPath.charAt(1)))
                                rfn.setLocalPath(mirrorPath.str());
                            else
                                rfn.setRemotePath(mirrorPath.str());

                            if (!rfn.getPort() && !rfn.isLocal())
                            {
                                StringBuffer mountPoint;
                                serverConfig->getProp("@mountPoint", mountPoint);
                                if (!mountPoint.length())
                                    mountPoint.append(DEFAULT_MOUNT_POINT);
                                addPathSepChar(mountPoint);
                                recursiveCreateDirectory(mountPoint.str());
                                PROGLOG("Mounting url \"%s\" on mount point \"%s\"", mirrorPath.str(), mountPoint.str());
                                bool ub = unmountDrive(mountPoint.str());
                                if (!mountDrive(mountPoint.str(), rfn))
                                {
                                    if (!ub)
                                        PROGLOG("Failed to remount mount point \"%s\", possibly in use?", mountPoint.str());
                                    else
                                        PROGLOG("Failed to mount \"%s\"", mountPoint.str());
                                    return 0;
                                }
                                else
                                    serverConfig->setProp("SDS/@remoteBackupLocation", mountPoint.str());
                                mirrorPath.clear().append(mountPoint);
                            }
                        }
#endif
                        StringBuffer backupCheck(dataPath);
                        backupCheck.append("bakchk.").append((unsigned)GetCurrentProcessId());
                        OwnedIFile iFileDataDir = createIFile(backupCheck.str());
                        OwnedIFileIO iFileIO = iFileDataDir->open(IFOcreate);
                        iFileIO.clear();
                        try
                        {
                            backupCheck.clear().append(mirrorPath).append("bakchk.").append((unsigned)GetCurrentProcessId());
                            OwnedIFile iFileBackup = createIFile(backupCheck.str());
                            if (iFileBackup->exists())
                            {
                                PROGLOG("remoteBackupLocation and dali data path point to same location! : %s", mirrorPath.str());
                                iFileDataDir->remove();
                                return 0;
                            }
                        }
                        catch (IException *)
                        {
                            try { iFileDataDir->remove(); } catch (IException *e) { EXCLOG(e, NULL); e->Release(); }
                            throw;
                        }
                        iFileDataDir->remove();

                        StringBuffer dest(mirrorPath.str());
                        dest.append(DALICONF);
                        copyFile(dest.str(), DALICONF);
                        StringBuffer covenPath(dataPath);
                        OwnedIFile ifile = createIFile(covenPath.append(DALICOVEN).str());
                        if (ifile->exists())
                        {
                            dest.clear().append(mirrorPath.str()).append(DALICOVEN);
                            copyFile(dest.str(), covenPath.str());
                        }
                    }
                    if (serverConfig->getPropBool("@daliServixCaching", true))
                        setDaliServixSocketCaching(true);
                }
                catch (IException *e)
                {
                    StringBuffer s("Failure whilst preparing dali backup location: ");
                    LOG(MCoperatorError, unknownJob, e, s.append(mirrorPath).append(". Backup disabled").str());
                    serverConfig->removeProp("SDS/@remoteBackupLocation");
                    e->Release();
                }
            }
        }
        else
            serverConfig.setown(createPTree());
#ifdef _WIN32
        Owned<CReleaseMutex> globalNamedMutex;
        if (!serverConfig->getPropBool("allowMultipleDalis"))
        {
            PROGLOG("Checking for existing daserver instances");
            StringBuffer s("DASERVER");
            globalNamedMutex.setown(new CReleaseMutex(s.str()));
            if (!globalNamedMutex->lockWait(10*1000)) // wait for 10 secs
            {
                PrintLog("Another DASERVER process is currently running");
                return 0;
            }
        }
#endif
        unsigned short myport = epa.item(myrank).port;
        startMPServer(myport,true);
        setMsgLevel(serverConfig->getPropInt("SDS/@msgLevel", 100));
        startLogMsgChildReceiver();
        startLogMsgParentReceiver();

        IGroup *group = createIGroup(epa);
        initCoven(group,serverConfig);
        group->Release();
        epa.kill();

// Audit logging
        StringBuffer auditDir;
        {
            Owned<IComponentLogFileCreator> lf = createComponentLogFileCreator(serverConfig, "dali");
            lf->setLogDirSubdir("audit");//add to tail of config log dir
            lf->setName("DaAudit");//override default filename
            lf->setCreateAliasFile(false);
            lf->setMsgFields(MSGFIELD_timeDate | MSGFIELD_code);
            lf->setMsgAudiences(MSGAUD_audit);
            lf->setMaxDetail(TopDetail);
            lf->beginLogging();
            auditDir.set(lf->queryLogDir());
        }

// SNMP logging
        bool enableSNMP = serverConfig->getPropBool("SDS/@enableSNMP");
        if (serverConfig->getPropBool("SDS/@enableSysLog",true))
            UseSysLogForOperatorMessages();
        AddServers(auditDir.str());
        addAbortHandler(actionOnAbort);
        Owned<IPerfMonHook> perfMonHook;
        startPerformanceMonitor(serverConfig->getPropInt("Coven/@perfReportDelay", DEFAULT_PERF_REPORT_DELAY)*1000, PerfMonStandard, perfMonHook);
        StringBuffer absPath;
        StringBuffer dataPath;
        serverConfig->getProp("@dataPath",dataPath);
        makeAbsolutePath(dataPath.str(), absPath);
        setPerformanceMonitorPrimaryFileSystem(absPath.str());
        if(serverConfig->hasProp("SDS/@remoteBackupLocation"))
        {
            absPath.clear();
            serverConfig->getProp("SDS/@remoteBackupLocation",dataPath.clear());
            makeAbsolutePath(dataPath.str(), absPath);
            setPerformanceMonitorSecondaryFileSystem(absPath.str());
        }

        try
        {
            ForEachItemIn(i1,servers)
            {
                IDaliServer &server=servers.item(i1);
                server.start();
            }
        }
        catch (IException *e)
        {
            EXCLOG(e, "Failed whilst starting servers");
            stopServer();
            stopPerformanceMonitor();
            throw;
        }
        try {
#ifndef _NO_LDAP
            setLDAPconnection(createDaliLdapConnection(serverConfig->getPropTree("Coven/ldapSecurity")));
#endif
        }
        catch (IException *e) {
            EXCLOG(e, "LDAP initialization error");
            stopServer();
            stopPerformanceMonitor();
            throw;
        }
        PrintLog("DASERVER[%d] starting - listening to port %d",myrank,queryMyNode()->endpoint().port);
        startMPServer(myport,false);
        bool ok = true;
        ForEachItemIn(i2,servers)
        {
            IDaliServer &server=servers.item(i2);
            try {
                server.ready();
            }
            catch (IException *e) {
                EXCLOG(e,"Exception starting Dali Server");
                ok = false;
            }

        }
        if (ok) {
            covenMain();
            removeAbortHandler(actionOnAbort);
        }
        stopLogMsgListener();
        stopServer();
        stopPerformanceMonitor();
    }
    catch (IException *e) {
        EXCLOG(e, "Exception");
    }
    return 0;
}

int main(int argc, const char* argv[])
{
    InitModuleObjects();
    NoQuickEditSection x;
    CDaemonCLI cli(argc, argv);
    if(! cli.parseCommandLineOptions() )
    {
        exit(0);
    }
    try
    {
        EnableSEHtoExceptionMapping();
#ifndef __64BIT__
        Thread::setDefaultStackSize(0x20000);
#endif
        setAllocHook(true);

        rank_t myrank = 0;

        CDaemonInfo dinfo(cli.getEnvFile());

#ifndef _WIN32
        if ( ! cli.inForeground() )
        {
            int ret = initDaemon();
            if (ret)
                return ret;
        }
#endif
        SocketEndpoint ep;
        SocketEndpointArray epa;
        ep.setLocalHost(DALI_SERVER_PORT);
        epa.append(ep);

        int d = startDali(myrank, ep, epa, cli, dinfo);
        if ( d != 0 )
        {
            return d;
        }
    }
    catch (IException *e)
    {
        EXCLOG(e, "Exception");
    }
    UseSysLogForOperatorMessages(false);
    return 0;
}
