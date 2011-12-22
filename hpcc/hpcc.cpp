#include "build-config.h"
#include "platform.h"
#include "jlog.hpp"
#include "jlib.hpp"
#include "jargv.hpp"
#include "jsuperhash.hpp"

#include <iostream>
#include <utility>

using namespace std;


enum CLIOptionType
{
    CLIFlag,
    CLIOption,
    CLIPath
};

enum CLIOptionAction
{
    CLINone,
    CLIStop,
    CLIRunChain
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
    virtual CLIOptionAction getAction() = 0;
    virtual bool isSet() = 0;
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
            CLIOptionType type, const char* defaultValue, bool required, CLIOptionAction action=CLINone)
    {
        _flag.set(flag);
        _name.set(name);
        _description.set(description);
        _type = type;
        _defaultValue.set(defaultValue);
        _required = required;
        _action = action;
        _optFlag.clear().append(CLI_FLAGSTRING).append(_flag);
        _optName.clear().append(CLI_OPTIONSTRING).append(_name);
        _Set = false;
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
            iter.matchOption(_value, _optName);
            iter.matchFlag(_value, _optFlag);

            //Add flag to run this one.
            iter.matchOption(_value, _name);

            if( !_value.isEmpty() )
            {
                _Set=true;
                return true;
            }
        }else if (_type == CLIFlag )
        {
            bool hld;
            iter.matchFlag(hld, _optName);
            iter.matchFlag(hld, _optFlag);

            //Add flag to run this one.
            iter.matchFlag(hld, _name);

            _value.clear();
            if ( hld )
            {
                _value.set("1");
                _Set=true;
                return true;
            }
            else
            {
                _value.set("0");
                return true;
            }
            return false;
        }else{
            //ToDo: implement MatchPathFlag logic.
        }
        return false;
    }

    inline void setValue(const char* value) { _value.set(value); }
    inline void setValue(StringAttr value) { _value.set(value); }
    inline const char* getValue(){ return _value; }
    inline bool isRequired(){ return _required == true; }
    inline CLIOptionAction getAction() { return _action; }
    inline bool isSet() { return _Set == true; }

    const char *queryFindString() const { return (const char*) _name; }

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
    CLIOptionAction _action;
    CLIOptionType _type;
    CCLIChain* _chain;
};

interface ICLIChain
{
    virtual void addLink(CCLIOption &opt, bool end) = 0;
    virtual void removeLink(StringAttr name) = 0;
    virtual bool runChain(ArgvIterator &iter) = 0;
    virtual StringAttr getValue(StringAttr name) = 0;
    virtual void setEnd(StringAttr name) = 0;
};

class CCLIChain : public CInterface, implements ICLIChain
{
public:
    IMPLEMENT_IINTERFACE;
    void addLink(CCLIOption &opt, bool end=false)
    {
        _chain.add(opt);
        if( end )
            setEnd(opt.queryFindString());
    }

    void removeLink(StringAttr name)
    {
        _chain.remove((const char*) name);
    }

    bool runChain(ArgvIterator &iter)
    {
        SuperHashIteratorOf<CCLIOption> SHiter(_chain);
        ForEach(iter)
        {
            ForEach(SHiter)
            {
                if ( SHiter.query().checkIterator(iter) )
                {
                    cout<<"Option Name: "<<SHiter.query().queryFindString();
                    cout<<" - Value: "<<SHiter.query()<<endl;
                    cout<<"Act: "<<SHiter.query().getAction()<<endl;
                    cout<<"Set: "<<SHiter.query().isSet()<<endl;
                    if ( SHiter.query().getAction()==CLIStop && SHiter.query().isSet() )
                    {
                        cout<<"Found Stop Point! : "<<SHiter.query().queryFindString()<<endl;
                        return true;
                    }
                }
            }
        }
    }

    StringAttr getValue(StringAttr name)
    {
        CCLIOption *opt =  _chain.find(name);
        return opt->getValue();
    }

    int getCount()
    {
        return _chain.count();
    }

    void setEnd(StringAttr name)
    {
        _end.set(name);
    }


protected:
    StringSuperHashTableOf<CCLIOption> _chain;
    StringAttr _end;

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
    CCLIOption bo("h","help", "Display Help", CLIFlag, NULL, false, CLIStop);
    CCLIOption bo2("n","node", "Display Help", CLIOption, NULL, false);
    CCLIChain c1;

    c1.addLink(bo);
    c1.addLink(bo2, true);

    ArgvIterator ai(argc, argv);
    if( c1.runChain(ai) )
    {
        cout<<"bo value: "<<c1.getValue("help")<<endl;
        cout<<"bo2 value: "<<c1.getValue("node")<<endl;
    }
    releaseAtoms();
    return 0;
}
