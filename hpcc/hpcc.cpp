#include "build-config.h"
#include "platform.h"
#include "jlib.hpp"
#include "jargv.hpp"

#include <iostream>
#include <utility>

using namespace std;


enum CLIOptionType
{
    CLISTRINGATTR,
    CLIBOOL,
    CLIUNSIGNED,
    CLISTRINGBUFFER
};

enum CLIOptionLevel
{
    CLIGlobal,
    CLILocal
};


interface ICLIOption : extends IInterface
{
    virtual const char* getDescription() = 0;
    virtual const char* getKey() = 0;
    virtual CLIOptionLevel getLevel() = 0;
    virtual void setDescription(const char*) = 0;
    virtual void setKey(const char*) = 0;
    virtual void setLevel(CLIOptionLevel) = 0;
};

class CCLIOption : public CInterface, implements ICLIOption
{
public:
    IMPLEMENT_IINTERFACE;


    const char* getDescription()
    {
        return description;
    }

    void setDescription(const char* desc)
    {
        description = desc;
    }

    const char* getKey()
    {
        return key;
    }

    void setKey(const char* _key)
    {
        key = _key;
    }

    CLIOptionLevel getLevel()
    {
        return level;
    }

    void setLevel(CLIOptionLevel _level)
    {
        level = _level;
    }

protected:
    const char* key;
    const char* description;
    CLIOptionLevel level;

};

template <typename T>
class TCLIOption : public CCLIOption
{
public:
    /*
    TCLIOption(T &_value, const char* _key, const char* _description, T _defaultValue=null, CLIOptionLevel _level=CLIGlobal):
        value(_value), key(_key), description(_description), defaultValue(_defaultValue), level(_level)
    {

    }
    */
    TCLIOption(const char* _key, const char* _description, CLIOptionLevel _level=CLIGlobal)
    {
        key = _key;
        description = _description;
        level = _level;
    }

    void set(pair<const char*, T> kvpair)
    {
        key = kvpair.first();
        value = kvpair.value();
    }

    pair<const char*, T> get()
    {
        if(!value)
        {
            value = defaultVal;
        }
        return pair<const char*, T>(key,value);
    }

private:
    T *value;
    T defaultVal;
};

class CCLIOptionFactory
{
public:
    static CCLIOption* getCLIOption(CLIOptionType type, const char* key, const char* description, CLIOptionLevel level=CLIGlobal)
    {
        switch(type)
        {
        case CLISTRINGATTR: return new TCLIOption<StringAttr>(key, description, level);
        case CLIBOOL: return new TCLIOption<bool>(key, description, level);
        case CLIUNSIGNED: return new TCLIOption<unsigned>(key, description, level);
        case CLISTRINGBUFFER: return new TCLIOption<StringBuffer>(key, description, level);
        }
    }

};

#define StingAttrOption(x,y) CCLIOptionFactory::getCLIOption(CLISTRINGATTR,x,y)
#define BoolOption(x,y) CCLIOptionFactory::getCLIOption(CLIBOOL,x,y)
#define UnsignedOption(x,y) CCLIOptionFactory::getCLIOption(CLIUNSIGNED,x,y)
#define StingBufferOption(x,y) CCLIOptionFactory::getCLIOption(CLISTRINGBUFFER,x,y)


class CLIChain
{

};

class CLI
{
    virtual CLIChain* getChain() = 0;
    virtual void addChain(CLIChain &chain) = 0;
    virtual void validateChain(const char** argv, int argc) = 0;

};

int main(int argc, const char** argv)
{
    CCLIOption *opt = BoolOption("OPT1", "This is a test.");
    cout<<"OPT: "<<opt->getKey()<<" - "<<opt->getDescription()<<" - L:"<<opt->getLevel()<<endl;
    return 0;
}
