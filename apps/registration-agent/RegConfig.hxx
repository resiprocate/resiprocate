#ifndef REGCONFIG_HXX
#define REGCONFIG_HXX

#include <rutil/ConfigParse.hxx>

class RegConfig : public resip::ConfigParse
{

public:
    RegConfig();
    virtual ~RegConfig();

    virtual void printHelpText(int argc, char **argv);

};

#endif
