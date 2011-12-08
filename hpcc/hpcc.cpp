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

};

class CCLIOption : public CInterface, implements ICLIOption
{
public:
    IMPLEMENT_IINTERFACE;
};

template <typename T>
class TCLIOption : public CCLIOption
{
public:
    pair<char*, T> get()
    {
        return pair<char*, T>(key,value);
    }

private:
    T value;
    char* key;
    char* description;
    T defaultVal;
    CLIOptionLevel level;

};

class CCLIOptionFactory
{
public:
    static CCLIOption* getCLIOption(CLIOptionType type)
    {
        switch(type)
        {
        case CLISTRINGATTR: return new TCLIOption<StringAttr>();
        case CLIBOOL: return new TCLIOption<bool>();
        case CLIUNSIGNED: return new TCLIOption<unsigned>();
        case CLISTRINGBUFFER: return new TCLIOption<StringBuffer>();
        }
    }

};

#define StingAttrOption() CCLIOptionFactory::getCLIOption(CLISTRINGATTR)
#define BoolOption() CCLIOptionFactory::getCLIOption(CLIBOOL)
#define UnsignedOption() CCLIOptionFactory::getCLIOption(CLIUNSIGNED)
#define StingBufferOption() CCLIOptionFactory::getCLIOption(CLISTRINGBUFFER)


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
    return 0;
}
