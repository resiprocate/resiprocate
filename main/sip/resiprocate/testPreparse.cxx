#include <iostream>
#include <stdio.h>

#include <sipstack/Preparse.hxx>


main()
{
  using namespace Vocal2;
  int c;
  //bool done = false;
  static char buffer[1024];
  char * p = buffer;
  while ((c = getchar()) != EOF)
  {
     char cc = (char)c;
     *p++ = cc;
  }
  Preparse pre;

  pre.addBuffer(buffer,p-buffer);

  return pre.process();

}
