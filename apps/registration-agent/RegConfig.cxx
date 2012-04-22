
#include "RegConfig.hxx"

using namespace std;

RegConfig::RegConfig()
{
}

RegConfig::~RegConfig()
{
}

void
RegConfig::printHelpText(int argc, char **argv)
{
    cout << "Command line format is:" << endl;
    cout << "    " << removePath(argv[0]) << " [<ConfigFilename>] [--<ConfigValueName>=<ConfigValue> ...]" << endl;
}
