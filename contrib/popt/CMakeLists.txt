add_library(popt STATIC
        config.h
        findme.h
        popt.h
        poptint.h
        system.h

        findme.c
        popt.c
        poptconfig.c
        popthelp.c
        poptparse.c
        )
target_include_directories(popt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})