#include <iostream>

using namespace std;

inline int f(char *in)
{
   
   char str[4];
   register char * p = in;

   str[0] = *p++;
   str[1] = *p++;
   str[2] = *p++;
   str[3] = 0;

   return *(unsigned int*)str;
}


int
main()
{
#if 0   
   cout << "INV " << " " << hex << ((*(unsigned int*)"INV ") >> 8) << endl;
   cout << "ACK " << " " << ((*(unsigned int*)"ACK ") >> 8) << endl;
   cout << "BYE " << " " << ((*(unsigned int*)"BYE ") >> 8) << endl;
   cout << "CAN " << " " << ((*(unsigned int*)"CAN ") >> 8) << endl;
   cout << "REG " << " " << ((*(unsigned int*)"REG ") >> 8) << endl;
   cout << "SUB " << " " << ((*(unsigned int*)"SUB ") >> 8) << endl;
   cout << "OPT " << " " << ((*(unsigned int*)"OPT ") >> 8) << endl;
   cout << "REF " << " " << ((*(unsigned int*)"REF ") >> 8) << endl;
   cout << "NOT " << " " << ((*(unsigned int*)"NOT ")) << endl;
   cout << "NOT " << " " << ((*(unsigned int*)"NOT ") >> 8) << endl;
   cout << "NOT " << " " << ((*(unsigned int*)"NOTj") >> 8) << endl;
#else   
   cout << "INV " << " " << f("INV ") << endl;
   cout << "ACK " << " " << f("ACK ") << endl;
   cout << "BYE " << " " << f("BYE ") << endl;
   cout << "CAN " << " " << f("CAN ") << endl;
   cout << "REG " << " " << f("REG ") << endl;
   cout << "SUB " << " " << f("SUB ") << endl;
   cout << "OPT " << " " << f("OPT ") << endl;
   cout << "REF " << " " << f("REF ") << endl;
   cout << "NOT " << " " << f("NOT ") << endl;
   cout << "NOT " << " " << f("NOT ") << endl;
   cout << "NOT " << " " << f("NOT ") << endl;
#endif

   return 0;
}
