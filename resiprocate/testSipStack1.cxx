

int
main()
{
  SipStack sipStack;
  Message* msg=NULL;

  while (1)
  {
    sipStack.process();

    msg = sipStack.receive();
    if ( msg )
    {
      cout << msg << endl;
    }

    usleep( 50*1000); // sleep for 20 ms
  }
}
