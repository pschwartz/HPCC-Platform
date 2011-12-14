#include "build-config.h"
#include "platform.h"
#include "jlog.hpp"
#include "jlib.hpp"
#include "jargv.hpp"

#include <iostream>
#include <utility>

using namespace std;


enum CLIOptionType
{
    CLIFlag,
    CLIOption,
    CLIPath
};

class CCLIChain;

interface ICLIOption
{
    virtual void getUsageNameString(StringBuffer &usageString) = 0;
    virtual void getUsageFlagString(StringBuffer &usageString) = 0;
    virtual void setValue(const char* value) = 0;
    virtual void setValue(StringAttr value) = 0;
    virtual const char* getValue() = 0;
    virtual bool isRequired() = 0;
    virtual bool checkIterator(ArgvIterator &iter) = 0;
};

#ifndef CLI_FLAGSTRING
#define CLI_FLAGSTRING "-"
#endif
#ifndef CLI_OPTIONSTRING
#define CLI_OPTIONSTRING "--"
#endif

class CCLIOption : public CInterface, implements ICLIOption
{
    friend ostream & operator<<(ostream& os, const CCLIOption& _cli)
    {
        os<<_cli._value;
        return os;
    }

private:
    StringBuffer _optFlag;
    StringBuffer _optName;

public:
    IMPLEMENT_IINTERFACE;
    CCLIOption(const char* flag, const char* name, const char* description,
            CLIOptionType type, const char* defaultValue, bool required)
    {
        _flag.set(flag);
        _name.set(name);
        _description.set(description);
        _type = type;
        _defaultValue.set(defaultValue);
        _required = required;
        _optFlag.clear().append(CLI_FLAGSTRING).append(_flag);
        _optName.clear().append(CLI_OPTIONSTRING).append(_name);
    }

    void getUsageFlagString(StringBuffer &usageString)
    {
        usageString.clear();
        usageString.append("  ").append(_optFlag);
        if ( _type != CLIFlag )
            usageString.append("=<").append(_name).append(">");
        usageString.append("  ").append(_description);
        if (_required)
            usageString.append(" (required)");
    }

    void getUsageNameString(StringBuffer &usageString)
    {
        usageString.clear();
        usageString.append("  ").append(_optName);
        if ( _type != CLIFlag )
            usageString.append("=<").append(_name).append(">");
        usageString.append("  ").append(_description);
        if (_required)
            usageString.append(" (required)");
    }

    bool checkIterator(ArgvIterator &iter)
    {
        if (iter.done())
        {
            return false;
        }

        if ( _type == CLIOption )
        {
            if(iter.matchOption(_value, _optName))
            {
                cout<<"Found Option: "<<_name<<endl;
                return true;
            }

            if(iter.matchFlag(_value, _optFlag))
            {
                cout<<"Found Flag: "<<_name<<endl;
                return true;
            }
        }else if (_type == CLIFlag )
        {
            bool hld;
            if(iter.matchFlag(hld, _optName))
            {
                cout<<"Found Option: "<<_name<<endl;
            }

            if(iter.matchFlag(hld, _optFlag))
            {
                cout<<"Found Flag: "<<_name<<endl;
            }
            _value.clear();
            if ( hld )
            {
                _value.set("1");
                return true;
            }
            else
            {
                _value.set("0");
                return true;
            }
            return false;
        }else{

        }
        return false;
    }

    inline void setValue(const char* value) { _value.set(value); }
    inline void setValue(StringAttr value) { _value.set(value); }
    inline const char* getValue(){ return (const char*) _value; }
    inline bool isRequired(){ return _required == true; }

    void operator=(const char* in)
    {
        setValue(in);
    }

    virtual ~CCLIOption()
    {
    }

protected:
    StringAttr _flag;
    StringAttr _name;
    StringAttr _description;
    StringAttr _defaultValue;
    StringAttr _value;
    bool _required;
    bool _Set;
    CLIOptionType _type;
    CCLIChain* _chain;
};

interface ICLIChain
{

};

class CCLIChain : public CInterface, implements ICLIChain
{
public:
    IMPLEMENT_IINTERFACE;

};

class CLI
{
    virtual CCLIChain* getChain() = 0;
    virtual void addChain(CCLIChain &chain) = 0;
    virtual void validateChain(const char** argv, int argc) = 0;
};

int main(int argc, const char** argv)
{
    InitModuleObjects();
    queryStderrLogMsgHandler()->setMessageFields(0);
    CCLIOption bo("h","help", "Display Help", CLIFlag, NULL, false);
    CCLIOption bo2("n","node", "Display Help", CLIOption, NULL, false);

    ArgvIterator ai(argc, argv);
    if(ai.done())
        return 1;
    for(; !ai.done(); ai.next())
    {
        cout<<"argv "<<ai.query()<<endl;
        if ( bo.checkIterator(ai) )
        {
            cout<<"Option Value1: "<<bo<<endl;
        }
        if ( bo2.checkIterator(ai) )
        {
            cout<<"Option Value2: "<<bo2<<endl;
        }
    }
    releaseAtoms();
    return 0;
}
