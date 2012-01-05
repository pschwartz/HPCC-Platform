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

#ifndef JCLI_HPP
#define JCLI_HPP

interface ICLICommand : extends IInterface
{
    virtual bool parseCommandLineOptions(ArgvIterator &iter)=0;
    virtual bool finalizeOptions(IProperties *globals)=0;
    virtual int processCMD()=0;
    virtual void usage()=0;
};

typedef ICLICommand *(*CLICommandFactory)(const char *cmdname);

#define CLIOPT_ENV "--env"
#define CLIOPT_ENV_DEFAULT "/etc/HPCCSystems/environment.xml"
#define CLIOPT_NAME "--name"
#define CLIOPT_VERSION "--version"
#define CLIOPT_FOREGROUND "-f"


bool extractCLICmdOption(StringBuffer & option, IProperties * globals, const char * envName, const char * propertyName, const char * defaultPrefix, const char * defaultSuffix);
bool extractCLICmdOption(StringAttr & option, IProperties * globals, const char * envName, const char * propertyName, const char * defaultPrefix, const char * defaultSuffix);
bool extractCLICmdOption(bool & option, IProperties * globals, const char * envName, const char * propertyName, bool defval);
bool extractCLICmdOption(unsigned & option, IProperties * globals, const char * envName, const char * propertyName, unsigned defval);

enum CLICmdOptionMatchIndicator
{
    CLICmdOptionNoMatch=0,
    CLICmdOptionMatch=1,
    CLICmdOptionCompletion=2
};

class CLICmdCommon : public CInterface, implements ICLICommand
{
public:
    IMPLEMENT_IINTERFACE;
    CLICmdCommon()
    {
    }
    virtual CLICmdOptionMatchIndicator matchCommandLineOption(ArgvIterator &iter, bool finalAttempt=false);
    virtual bool finalizeOptions(IProperties *globals);

    virtual void usage()
    {
        fprintf(stdout,
            "      --env=<environment.xml>        environment xml file to use.\n"
            "      --name=<name>      name of the component to use.\n"
        );
    }
public:
    StringAttr optEnv;
    StringAttr optName;
    bool optForeground;
};

ICLICommand *createCoreCLICommand(const char *cmdname);

class CLICMDShell
{
public:
    CLICMDShell(int argc, const char *argv[], CLICommandFactory _factory, const char *_version, bool _runExternals=false)
        : args(argc, argv), factory(_factory), version(_version), optHelp(false), runExternals(_runExternals)
    {
        splitFilename(argv[0], NULL, NULL, &name, NULL);
    }

    bool parseCommandLineOptions(ArgvIterator &iter);
    void finalizeOptions(IProperties *globals);
    int processCMD(ArgvIterator &iter);
    int callExternal(ArgvIterator &iter);
    int run();

    virtual void usage();

protected:
    ArgvIterator args;
    Owned<IProperties> globals;
    CLICommandFactory factory;
    StringBuffer name;
    StringAttr cmd;
    bool runExternals;

    StringAttr version;

    bool optHelp;
};

#endif
