/*
 *  libSipImp.h
 *
 */

extern "C" 
{
   // initalize everything 
   void libSipImp_Init();
   
   // process any message that may be waiting to be received and call the callbacks
   void libSipImp_Process();
   
   // caller must pass in an array and set maxFd to size of array
   void libSipImp_GetAllFd(int fileDescriptors[], int maxFd );
   
   // login and register 
   void libSipImp_Login( char* aor, char* passwd );
   
   // send an message 
   void libSipImp_SendMessage( char* dest , char* msg );
  
   // set your curent presense state 
   void libSipImp_SetState( int open, char* note );

   // subscribe to this buddy 
   void libSipImp_AddBuddy( char* aor );

   // callback gets a message and from 
   void libSipImp_SetReceiveIMCallback( void (*func)(char*, char* ) );

   // callback gets from, open/close bool, note 
   void libSipImp_SetUpdateCallback( void (*func)(char*, int,  char* ) );
};



