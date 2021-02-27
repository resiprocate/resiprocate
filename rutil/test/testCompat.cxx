#include <cassert>
#include<iostream>
#include "rutil/compat.hxx"
#include <cstdio>
using namespace resip;
using namespace std;

int
main()
{

   // make sure these are consistent on all platforms

   assert(sizeof(UInt8) == 1);
   assert(sizeof(UInt16) == 2);
   assert(sizeof(UInt32) == 4);
   assert(sizeof(Int32) == 4);
   assert(sizeof(UInt64) == 8);

   UInt64 data = 0;
   UInt8 *tmp = (UInt8*)&data;
   tmp[7] = 1;
   assert(ntoh64(data) == 1);
   UInt64 data2 = hton64(data);
   assert(tmp[7] == 1);


   long long i1 = 5223372036854775807,i2 = 9223372036854775807;
   double d1 = 5223372036854775807.9965443,d2 = 9223372036854775807.1265443;
   long long i11 = -5223372036854775807, i22 = -9223372036854775807;
   double d11 = -5223372036854775807.99, d22 = -9223372036854775807.12;
   
//test for resipMax
   {
	   //for postive integer
	   assert(resipMax(3,2) == 3);
	   cerr << "max(3,2 ): " << resipMax(3,2)<<endl;
	   //reverse of above combination
	   assert(resipMax(2,3) == 3);
	   cerr << "max(2,3 ): " << resipMax(2,3)<<endl;
	   //for negative integers
	   assert(resipMax(-3,-2) == -2);
	   cerr << "max(-3,-2):" << resipMax(-3,-2)<< endl;
	   //for large integers
	   assert(resipMax(i1,i2) == i2);
	   cerr<< "max(5223372036854775807,9223372036854775807): "<< resipMax(i1,i2)<< endl;
	   //for large floating point numbers
	   assert(resipMax(d1,d2) == d2) ;
	   cerr<< "max(5223372036854775807.9965443,5223372036854775807.1265443 ): "<< resipMax(d1,d2)<< endl;
	   //for large negative numbers
	   assert(resipMax(i11,i22) == i11);
	   cerr<< "max(-5223372036854775807,-9223372036854775807): "<< resipMax(i11,i22)<<endl;
	   //for large negative floating point numbers
	   assert(resipMax(d11,d22) == d11);
	   cerr<< "max(-5223372036854775807.99,-9223372036854775807.12): "<< resipMax(d11,d22)<< endl;
	   //for same numbers
	   assert(resipMax(3,3) == 3);
	   cerr << "max(3,3 ): " << resipMax(3,3)<<endl;
	   //for char type arguements
	   assert(resipMax('a','b') == 'b');
	   cerr<< "max('a','b'): "<<resipMax('a','b')<<endl;
	   //test for string type arguements
	   assert(resipMax("hello","jello") == "jello");
	   cerr<<"max('hello','jello'): "<<resipMax("hello","jello")<<endl;
	   //reverse of above combination
	   assert(resipMax("jello","hello") == "jello");
	   cerr<<"max('jello','hello'): "<<resipMax("jello","hello")<<endl;
   } 
	//test for resipMin
   {
	   //for postive integer
	   assert(resipMin(3,2) == 2);
	   cerr << "min(3,2 ): " << resipMin(3,2)<< endl;
	   //reverse of above combination
	   assert(resipMin(2,3) == 2);
	   cerr << "min(2,3 ): " << resipMin(2,3)<<endl;
	   //for negative integers
	   assert(resipMin(-3,-2) == -3);
	   cerr << "min(-3,-2): " << resipMin(-3,-2)<< endl;
	   //for large integers
	   assert(resipMin(i1,i2) == i1);
	   cerr<< "min(5223372036854775807,9223372036854775807): "<< resipMin(i1,i2)<< endl;
	   //for large floating point numbers
       assert(resipMin(d1,d2) == d1) ;
	   cerr<< "min(5223372036854775807.9965443,5223372036854775807.1265443 ): "<< resipMin(d1,d2)<< endl;
	   //for large negative numbers
	   assert(resipMin(i11,i22) == i22);
	   cerr<< "min(-5223372036854775807,-9223372036854775807): "<< resipMin(i11,i22)<< endl;
	   //for large negative floating point numbers
       assert(resipMin(d11,d22) == d22);
	   cerr<< "min(-5223372036854775807.99,-9223372036854775807.12): "<< resipMin(d11,d22)<< endl;
	   //for same numbers
	   assert(resipMin(3,3) == 3);
	   cerr << "min(3,3 ): " << resipMin(3,3)<< endl;
	   //for char type arguements
	   assert(resipMin('a','b') == 'a');
	   cerr<< "min('a','b'): "<<resipMin('a','b')<<endl;
	   //test for string type arguements
	   assert(resipMin("hello","jello") == "hello");
	   cerr<<"min('hello','jello'): "<<resipMin("hello","jello")<<endl;
	   //reverse of above combination
	   assert(resipMin("jello","hello") == "hello");
	   cerr<<"min('jello','hello'): "<<resipMin("jello","hello")<<endl;
	}
   //test for strncasecmp
   {
	   cerr<<"test strncasecmp"<<endl;
	    {  //one upper case string and the other lower case
		   const char *str1 = "HELLO";
		   const char *str2 = "hell";
		   assert(strncasecmp(str1, str2, 4) == 0);		}
		{  //both strings same
		   const char *str1 = "HELLO";
		   const char *str2 = "HELLO";
		   assert(strncasecmp(str1, str2, 4) == 0);		}
		{  //for large strings
		   const char *str1 = "Once there lived in a forest a hair and a tortoise.The hare was very proud of his speed. He made fun of the tortoise for his slow speed. The tortoise challenged the hare to have a race with him. The hare accepted the challenge.The race started.The hare ran very fast. The tortoise was left much behind.";
		   const char *str2 = "Once there lived in a forest a hair and a tortoise.The hare was very proud of his speed. He made fun of the tortoise for his slow speed. The tortoise challenged the hare to have a race with him. The hare accepted the challenge.The race started.The hare ran very fast. The tortoise was left much behind.The race started. The crow was the referee. The hare ran very fast. The tortoise was left much behind.";
		   assert(strncasecmp(str1, str2, 200) == 0);	}
		{  //one string is empty
		   const char *str1 = "";
		   const char *str2 = "hell";
		   assert(strncasecmp(str1, str2, 0) == 0);		}
		{  //both are empty
		   const char *str1 = "";
		   const char *str2 = "";
		   assert(strncasecmp(str1, str2, 0) == 0);		}
		{  //special character
		   const char *str1 = "~~`@##";
		   const char *str2 = "~~`#$@";
		   assert(strncasecmp(str1, str2, 3) == 0);		}
	}
	//test for snprintf
	{
	   cerr<<"test snprintf"<<endl;
	    char buffers [50];
	    int cx;
	    char *c = "hungary";
	    //string having some format specifiers
	    cx = snprintf( buffers, 50, "I have %f$ and I am %s.There are %d apples and %d banana on that stand",35.50,c,6,7);
	    assert(cx > 0);
	    //for n being less than string lenth
		cx = snprintf( buffers, 5, "testing the snprintf function");
	    assert(cx == 29);
	    //for n being greater than sting length
	    cx = snprintf( buffers, 15, "hello there");
	    assert(cx == 11);
	    //for large string
	    char buffer1 [250];
	    cx = snprintf( buffer1, 250, "Once there lived in a forest a hair and a tortoise. The hare was very proud of his speed. He made fun of the tortoise for his slow speed. The tortoise challenged the hare to have a race with him. The hare accepted the challenge.");
	    assert(cx > 0);
	    char buffer2 [11];
	    //value of n and size of buffer exactly of same lengths 
	    cx = snprintf( buffer2, 11, "hellothere");
		assert(cx == 10);
	    // value of n and size of buffer  smaller than length of string
	    cx = snprintf( buffer2, 11, "hello!,someone there ?");
	    assert(cx == 22);
	    
   	}
   cerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
 *
 * Copyright (c) 2013 Daniel Pocock  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
