
To build this in windows ....

form a new project that creates a console application. Add in all the .cxx and .hxx fiels in sip2/sipstack and sip2/util and one of the test programs from sip2/sipstack/test 

YOu will have to remove some of the files that don't compile and aren't used.

turn off pre compiled headers

turn on RTTI (Real time type Information)

Set the compiler heap to 300 megs (/Zm300)

add the path where sip2 lives to the include path - generally (../..)

add Ws2_32.lib to link libraries


Have fun 
