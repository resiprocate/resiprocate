#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string>

const size_t indent = 4;

class ArGen
{
    public:
        ArGen(const char *arname = 0,
              const int arnum = 0,
              size_t wrapCol = 72);
        ~ArGen();
        void add(const unsigned char c);
        void add(const unsigned char * buf, size_t sz);
        void end();
        int outputDef(int fd);
    private:
        void addRaw(const unsigned char c);
        void addRaw(const char* str);
        std::string code;
        size_t mColumn;
        size_t mNcols;
};

static std::string
toString(int v )
{
    std::string s;
    bool b = false;
    for( int i = 1000000; i > 0 ; i/=10)
    {
        int p = v/i;
        if (b = ( b || p || ( i == 1)))
            s += '0' + p;
        v = v % i;
    }
    return s;
}

ArGen::ArGen(const char *arname, int arnum, size_t wrapCol) :
    mColumn(0),
    mNcols(wrapCol)

{
    if (!arname)
        arname = "data";

    code += "const char ";
    code += arname;
    if (arnum) code += toString(arnum);
    code += "[] = {\n";
}



void
ArGen::addRaw(const unsigned char c)
{
    code += c;
    ++mColumn;
}

void ArGen::addRaw(const char* s)
{
    for(size_t i = 0 ; s[i] ; ++i)
        addRaw(s[i]);
}

/// bulk of formatting work done here.
void
ArGen::add(const unsigned char c)
{
    // take care of 1st part of line
    if (!mColumn)
        addRaw("        \"");

    // take care of switching modes and opening or closing any running
    // quotations.

    if (isalnum(c) || c == ' ' ||
        ( ispunct(c) && c != '\\' && c != '"' ))
    {
        addRaw(c);
    }
    else
    {
        static const char hexv[] ="0123456789abcdef";
        addRaw("\\x");
        addRaw(hexv[(c&0xf0)>>8]);
        addRaw(hexv[(c&0x0f)]);
    }
    if (mColumn > mNcols || c == 0x0a)
    {
        addRaw("\"\n");
        mColumn = 0;
    }
}

void
ArGen::add(const unsigned char *c , size_t sz)
{
    for(size_t i = 0 ; i < sz ; ++i)
        add(c[i]);
}

ArGen::~ArGen() {}


void
ArGen::end()
{

    code += '\n';
    code += "};\n";
}

int
ArGen::outputDef(int fd)
{
    return write(fd,code.data(),code.size());
}

void
dumpraw(const char *path, const char * basename, int arNo)
{
    
    if (access(path,R_OK))
    {
        perror(path);
        return;
    }
    int fd = open(path,O_RDONLY);
    if (fd < 0)
    {
        perror(path);
        return;
    }
    int hdr = 0;
    int s;
    char unsigned buf[16 * 1024];
    const int bsz = sizeof(buf)/sizeof(*buf);
    const int nCols = 8;
    int col = 0;
    ArGen cg(basename,arNo);
    while((s=read(fd,buf,bsz)) > 0)
    {
        cg.add(buf,s);
    }
    cg.end();

    if (s < 0)
    {
        perror(path);
        return;
    }
    cg.outputDef(1);

    return;
}

const char * pName = 0;
int
usage()
{
    printf("usage: %s [-n arraybasename] file ...\n",pName);
}
int
main(int argc, char * argv[])
{
    pName = *argv;
    const char * basename = 0;
    int basearg = 1;
    if (argc < 2) return usage();
    if (argv[1][0] == '-' &&
        argv[1][1] == 'n')
    {
        if (argc < 4) return usage();
        basename = argv[2];
        basearg = 3;
    }
    for (int i = basearg ; i < argc ; ++i )
        dumpraw(argv[i],basename,i-basearg);
    return 0;
}

