CC = cl
CFLAGS = /nologo /G6 /MT /GX /DSUPPORT_UTF8 /DPOSIX_MALLOC_THRESHOLD=10

all: pcre.lib pcreposix.lib pcretest.obj
        link /nologo /out:pcretest.exe pcretest.obj pcre.lib pcreposix.lib

dll: chartables.c maketables.obj get.obj study.obj pcre.obj pcreposix.obj
        link /nologo /DLL /def:pcre.def /out:pcre.dll /implib:pcre.lib maketables.obj get.obj study.obj pcre.obj pcreposix.obj

pcre.lib: chartables.c maketables.obj get.obj study.obj pcre.obj
        lib /nologo /out:pcre.lib maketables.obj get.obj study.obj pcre.obj

pcreposix.lib: pcreposix.obj
        lib /nologo /out:pcreposix.lib pcreposix.obj

chartables.c: dftables.exe
        dftables.exe chartables.c

dftables.exe:
        $(CC) $(CFLAGS) dftables.c

.c.obj:
        $(CC) /c $(CFLAGS) /Tp$< /Fo$@

clean:
        del *.obj
        del *.exe
        del *.lib
        del *.exp
        del *.dll
