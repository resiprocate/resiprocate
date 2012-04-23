#ifndef REGCONFIG_HXX
#define REGCONFIG_HXX

#include <rutil/ConfigParse.hxx>

namespace registrationclient {

class RegConfig : public resip::ConfigParse
{

public:
    RegConfig();
    virtual ~RegConfig();

    virtual void printHelpText(int argc, char **argv);

};

} // namespace

#endif
