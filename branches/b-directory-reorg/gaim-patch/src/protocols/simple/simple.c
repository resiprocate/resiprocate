#include <assert.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "internal.h"


#include "account.h"
#include "accountopt.h"
#include "debug.h"
#include "multi.h"
#include "notify.h"
#include "prpl.h"
#include "request.h"
#include "server.h"
#include "util.h"

#ifdef NO_FORK
#include <shlwapi.h>
#endif

#include "simple.h"

static GaimPlugin *my_protocol = NULL;

typedef enum
{
      /* GAIM <-> GAG */
      SIMPLE_IM                 = 0x00,
      SIMPLE_PRESENCE           = 0x01,
      SIMPLE_HELLO              = 0x02,
  
      /* GAIM --> GAG */
      SIMPLE_LOGIN              = 0x40,
      SIMPLE_LOGOUT             = 0x41,
      SIMPLE_ADD_BUDDY          = 0x42,
      SIMPLE_REMOVE_BUDDY       = 0x43,
      SIMPLE_SHUTDOWN           = 0x44,

      /* GAG --> GAIM */
      SIMPLE_ERROR              = 0x80,
      SIMPLE_LOGIN_STATUS       = 0x81
} gag_command_t;


/*
   In the parent
   Read from gag using FD_GAG_TO_GAIM
   Write to gag using FD_GAIM_TO_GAG
*/
#ifdef NO_FORK
static int gagfd = 0;
#define FD_GAG_TO_GAIM gagfd
#define FD_GAIM_TO_GAG gagfd
#else
static int gaimtogag[2];
static int gagtogaim[2];
#define FD_GAG_TO_GAIM gagtogaim[0]
#define FD_GAIM_TO_GAG gaimtogag[1]
#endif

static int gaginitialized = 0;

static int
isAccountNameUnsafe(char *accountName)
{
   /* There's an open-ended minefield to be explored here.
    * This is just a first attempt to keep gaim and gag
    * from blowing apart because some user typed in
    * "bad stuff (tm)".
    */

   if (g_strrstr(accountName,"<")!=NULL) {return 1;}
   if (g_strrstr(accountName,">")!=NULL) {return 1;}
   if (g_strrstr(accountName," ")!=NULL) {return 1;}
   if (g_strrstr(accountName,":")!=NULL)
   {
    if (
        ! (    g_str_has_prefix(accountName,"sip:")
            || g_str_has_prefix(accountName,"sips:")
          )
       ) return 1;
   }
   return 0; 
}

char *
buildSIPURI(char *accountName)
{
  if (g_str_has_prefix(accountName,"sip:")) {return g_strdup(accountName);}
  if (g_str_has_prefix(accountName,"sips:")) {return g_strdup(accountName);}
  return g_strconcat("sip:",accountName,NULL);
}

static void 
sippy_get_string( char* buf, int bufSize )
{ 
   int len;
   int length;
   
   len = read(  FD_GAG_TO_GAIM, &length, sizeof(length) );
   assert( len == sizeof(length) );
   
   if ( length+1 >= bufSize )
   {
      assert(0);
   }
   
   if (length)
   {
     len = read( FD_GAG_TO_GAIM, buf, length );
     assert( len == length );
   }
   
   buf[length]=0;
}

static void 
sippy_get_int( int* i )
{ 
   int len;
   int length;
   
   len = read(  FD_GAG_TO_GAIM, &length, sizeof(length) );
   assert( len == sizeof(length) );
   
   if ( length != sizeof(int) )
   {
      assert(0);
   }
   
   len = read( FD_GAG_TO_GAIM, i, length );
   assert( len == length );
}


static void 
sippy_get_bool( unsigned char* i )
{ 
   int len;
   int length;
   
   len = read(  FD_GAG_TO_GAIM, &length, sizeof(length) );
   assert( len == sizeof(length) );
   
   if ( length != sizeof(char) )
   {
      assert(0);
   }
   
   len = read( FD_GAG_TO_GAIM, i, length );
   assert( len == length );
}


static void 
sippy_send_bool( const unsigned char b )
{  
   int len;
   int l;
   assert( FD_GAIM_TO_GAG );
   
   len=sizeof(b);
   l=write( FD_GAIM_TO_GAG, &len, sizeof(len) );
   assert( l == sizeof(len) );
   l=write( FD_GAIM_TO_GAG, &b, len );
   assert( l == len );
}

static void 
sippy_send_string( const char* buf )
{  
   int len;
   int l;
   assert( FD_GAIM_TO_GAG );
   
   /*RjS - Gaim gives us NULLs instead of empty strings sometimes
   */      
   len=(buf==NULL?0:strlen(buf));
   l=write( FD_GAIM_TO_GAG, &len, sizeof(len) );
   assert( l == sizeof(len) );
   if (len>0)
   {
     l=write( FD_GAIM_TO_GAG, buf, len );
     assert( l == len );
   }
}

static void 
sippy_send_command( const gag_command_t command )
{  
   int l;
   assert( FD_GAIM_TO_GAG );
   l=write( FD_GAIM_TO_GAG, &command, sizeof(command) );
   assert( l == sizeof(command) );
}

static const char *
sippy_list_icon(GaimAccount *a, GaimBuddy *b)
{
	return "sippy";
}

static void
sippy_recv_cb(gpointer data, gint source, GaimInputCondition condition)
{
  GaimConnection *gc = data;
  int len;
  int command;
  static char buf[4096];
  static char bufFrom[4096];
  static char bufTo[4096];
  
  len = read( FD_GAG_TO_GAIM, &command, sizeof(command ) );
  if ( len == -1 )
  {
     int err = errno;
     gaim_debug(GAIM_DEBUG_INFO,"sippy","err=%d\n",err);  
     /* something really bad happened */
     assert(0);
  }
  if ( len == 0 )
  {
     /* not ready */
     return;
  }
  assert( len == sizeof(command) );
  
  switch ( command )
  {
     case SIMPLE_IM:
     {	
        gaim_debug(GAIM_DEBUG_INFO,"sippy","got an IM messages from gag\n");  

        sippy_get_string( bufTo, sizeof(bufTo) );
        sippy_get_string( bufFrom, sizeof(bufFrom) );
        sippy_get_string( buf, sizeof(buf) );

        gaim_debug(GAIM_DEBUG_INFO,"sippy","got an IM from=%s to=%s data=%s\n",bufFrom,bufTo,buf);  

        serv_got_im(gc,bufFrom,buf,0,time(NULL)); 
     }
     break;
     
     case SIMPLE_LOGIN_STATUS:
     {
        unsigned char success;
        int sipCode;
        
        /* bool success, int sipCode, data message */
        sippy_get_bool( &success );
        sippy_get_int( &sipCode );
        sippy_get_string( buf, sizeof(buf) );  
 
        gaim_debug(GAIM_DEBUG_INFO,"sippy","got a LOGIN_STATUS status=%d msg=%s\n",sipCode,buf);
	if (gaim_connection_get_state(gc) != GAIM_CONNECTED)
	{
          if (success)
          {
            gaim_connection_update_progress(gc,_("Connecting"),1,2);
            gaim_connection_set_state(gc, GAIM_CONNECTED);
          }
          else
          {
            gaim_connection_error(gc, _(buf));
          }
	}
     }
     break;
     
     case SIMPLE_PRESENCE: /* uri aor, bool  available, Data status */
     { 
        unsigned char open;
	int lastReportedUCStatus;
        
        gaim_debug(GAIM_DEBUG_INFO,"sippy","got a PRES messages from gag\n");

        sippy_get_string( bufFrom, sizeof(bufFrom) );
        sippy_get_bool( &open );
        sippy_get_string( buf, sizeof(buf) );
        
        gaim_debug(GAIM_DEBUG_INFO,"sippy","got a PRES messages from gag from=%s open=%d, msg=%s\n",bufFrom,open,buf);

        {
	  struct simple_connection_cache *sc_cache = gc->proto_data;
	  struct simple_friend_cache *sf;
	  
	  sf = g_hash_table_lookup(sc_cache->friends,bufFrom);
	  if (!sf) {
	    sf = simple_friend_cache_new();
	    g_hash_table_insert(sc_cache->friends,g_strdup(bufFrom),sf);
	  }
          if (sf->status_string)
	  {
	    g_free(sf->status_string);
	  } 
	  sf->status_string = g_strdup(buf);

	  lastReportedUCStatus = (sf->status_int ^= 2);
	} 

	/* AFAICT, there is a half-done change to the use of
	 * the last argument to serv_got_update that is messing
	 * up Yahoo as well as us. The code above the prpl abstraction
	 * claims the integer is ours to do with as we please, but they
	 * use the lowest order bit to mean available or not.
	 *
	 * Other code looks for changes to this integer - if it has
	 * not changed it shortcuts all the GUI updates. So...
	 * We're going to game gaim for now by toggling the second
	 * lowest order bit each time we call this function.
         *
         * TODO: This was true in the gaim 0.71 code - need to review to
         * see if this is still appropriate.
	 */

        serv_got_update(gc, bufFrom, (int)open, 0, 0, 0, lastReportedUCStatus);

     }
     break;
     
     case SIMPLE_HELLO: /* bool ok */
     {
        unsigned char ok;
        sippy_get_bool( &ok ); 
        gaim_debug(GAIM_DEBUG_INFO,"sippy","got a HELLO ok=%d\n",ok);
     }
     
     case SIMPLE_ERROR:
     { 
        gaim_debug(GAIM_DEBUG_INFO,"sippy","got an ERROR messages from gag\n");
        sippy_get_string( buf, sizeof(buf) );
        gaim_debug(GAIM_DEBUG_INFO,"sippy","gag ERROR %s\n",buf);
        gaim_request_action(gc, _("SIP Error"), _("SIP Error"), buf, 
                             0, NULL, 1, _("Okay"), 0);
     }
     break;
     
     default:
        assert(0);
  }
}



static void
sippy_add_buddy(GaimConnection *gc, GaimBuddy *who, GaimGroup *dontCare)
{
   if (!gaginitialized) {return;}
   gaim_debug(GAIM_DEBUG_INFO,"sippy","sending ADD BUDDY to gag us=%s them=%s\n",gc->account->username,who->name);
   sippy_send_command( SIMPLE_ADD_BUDDY );
   sippy_send_string( gc->account->username );
   sippy_send_string( who->name );
}

static void
sippy_logout(GaimConnection *gc)
{
   if (!gaginitialized) {return;}
   gaim_debug(GAIM_DEBUG_INFO,"sippy","sending LOGOUT to gag aor=%s\n",gc->account->username);
   sippy_send_command( SIMPLE_LOGOUT );
   sippy_send_string( gc->account->username );
   gaim_debug(GAIM_DEBUG_INFO,"sippy","Removing gag's gaim_input\n");
   gaim_input_remove(gc->inpa);
}

static void
sippy_remove_buddy(GaimConnection *gc, GaimBuddy *who, GaimGroup *group)
{
   if (!gaginitialized) {return;}
   gaim_debug(GAIM_DEBUG_INFO,"sippy","sending REMOVE BUDDY to gag us=%s them=%s\n",gc->account->username,who->name);
   sippy_send_command( SIMPLE_REMOVE_BUDDY );
   sippy_send_string( gc->account->username );
   sippy_send_string( who->name );
}

static gboolean sippy_unload_plugin(GaimPlugin *plugin)
{ 
   if (gaginitialized)
   {
     gaim_debug(GAIM_DEBUG_INFO,"sippy","sending SHUTDOWN to gag\n");
     sippy_send_command( SIMPLE_SHUTDOWN );
   }
   return 1;
}

static void
init_gag(GaimConnection *gc)
{
  char sanity_check_buffer[8];
  int sane;
#ifdef NO_FORK
  char gag_path[256];
  DWORD gag_path_size = 240;
#endif

  gaim_debug(GAIM_DEBUG_INFO,"sippy","Initializing Gag\n");  

#ifdef NO_FORK
  {
    int listenfd;
    int status;
    int shell_exec_status;
    struct sockaddr_in us;
    struct sockaddr_in them;
    size_t addr_len = sizeof(them);
    char buffer[1024];

    memset((void *)&us, 0, sizeof(struct sockaddr_in));
    memset((void *)&them, 0, sizeof(struct sockaddr_in));
    us.sin_family = AF_INET;
    us.sin_port = 0xBEEF;
    inet_aton("127.0.0.1", &(us.sin_addr));

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
      /* We're single threaded, so strerror is safe */
      snprintf(buffer, sizeof(buffer), 
               "Unable to start the SIP engine - "
               "Could not create loopback socket; err = %d (%s)",
               errno, strerror(errno));

      gaim_connection_error(gc, buffer);
      return;
    }

    status = bind (listenfd, (struct sockaddr*)&us, sizeof(struct sockaddr));
    if (status != 0)
    {
      /* We're single threaded, so strerror is safe */
      snprintf(buffer, sizeof(buffer), 
               "Unable to start the SIP engine - "
               "Could not bind to loopback socket; err = %d (%s)",
               errno, strerror(errno));

      gaim_connection_error(gc, buffer);
      return;
    }
 
    status = listen(listenfd, 1);
    if (status != 0)
    {
      /* We're single threaded, so strerror is safe */
      snprintf(buffer, sizeof(buffer), 
               "Unable to start the SIP engine - "
               "Could not listen on loopback socket; err = %d (%s)",
               errno, strerror(errno));

      gaim_connection_error(gc, buffer);
      return;
    }

    /* Start SIP engine */

    /* First, check in the directory into which Gaim is installed */
    SHRegGetUSValue ("SOFTWARE\\gaim", NULL, NULL, 
                     (LPVOID)gag_path, 
                     &gag_path_size, 
                     TRUE,
                     NULL, 0);
    strcat(gag_path, "\\gag.exe");

    shell_exec_status = 
      (int)ShellExecute(NULL, "open", gag_path, "-l", NULL, 0);

    /* If that fails, see if it's in the path somewhere. */
    if (shell_exec_status <= 32)
    {
      shell_exec_status = 
        (int)ShellExecute(NULL, "open", "gag.exe", "-l", NULL, 0);
    }

    /* Well, if we can't find the gag.exe binary, we can't continue. */
    if (shell_exec_status <= 32)
    {
      gaim_connection_error(gc,
         _("Unable to start the SIP engine - gag.exe is not in your path"));
      close(listenfd);
      return;
    }

    gagfd = accept(listenfd, (struct sockaddr *)&them, &addr_len);
    if (gagfd < 0)
    {
      gaim_connection_error(gc,
         _("Unable to start the SIP engine - "
           "Could not accept incoming loopback connection"));
      close(listenfd);
      return;
    }

    close (listenfd);
  }
#else
  {
    int fork_result;

    if ( (pipe(gaimtogag)!=0) || (pipe(gagtogaim)!=0) )
    {
      gaim_connection_error(gc,
         _("Unable to start the SIP engine - resources unavailable"));
      return;
    }
    
    if ( (fork_result = fork()) == -1 )
    {
      gaim_connection_error(gc,
         _("Unable to start the SIP engine - process unavailable"));
      return;
    }
 
    if (fork_result == 0)
    {
      /* We are the child
         Read from gaim using gaimtogag[0]
         Write to gaim using gagtogaim[1]
      */

      close(0);
      close(1);

      dup(gaimtogag[0]);  /* new 0 */
      close(gaimtogag[0]);
      close(gaimtogag[1]);

      dup(gagtogaim[1]);  /* new 1 */
      close(gagtogaim[0]);
      close(gagtogaim[1]);
    
      execlp("gag","gag", (char*)0);

      /* if we're here, exec went horribly wrong */
      { 
        int t,l;
        char v;
        t=2; l=1; v=0;
        write (1,(void *)&t,sizeof(int));
        write (1,(void *)&l,sizeof(int));
        write (1,(void *)&v,1);
      }
      _exit(-1);
    }
  }
#endif

  /* In the parent
     Read from gag using FD_GAG_TO_GAIM
     Write to gag using FD_GAIM_TO_GAG
  */
    
  gaim_debug(GAIM_DEBUG_INFO,"sippy","starting sanity check\n");  
  sane = 1;
  read(FD_GAG_TO_GAIM,sanity_check_buffer,sizeof(int));
  sane = (*((int*)sanity_check_buffer)==2);
  if (sane) {
    read(FD_GAG_TO_GAIM,sanity_check_buffer,sizeof(int));
    sane = (*((int*)sanity_check_buffer)==1);
  }
  if (sane) {
    read(FD_GAG_TO_GAIM,sanity_check_buffer,1);
    sane = ((*sanity_check_buffer)!=0x00);
  }
  
  if (!sane)
  {
    return;
  }

  gaim_debug(GAIM_DEBUG_INFO,"sippy","gag is sane\n");  

  gaginitialized = 1;
}

static struct simple_friend_cache *
simple_friend_cache_new()
{
  struct simple_friend_cache *retval;

  retval = g_new0(struct simple_friend_cache, 1);

  return retval;
}

static void
simple_friend_cache_free(gpointer gp)
{
  struct simple_friend_cache *f = gp;
  if (f)
  {
    if (f->status_string) g_free(f->status_string);
    g_free(f);
  }
}

static void
sippy_login(GaimAccount *account)
{
  char *sipAccountURI = NULL;

  GaimConnection *gc = gaim_account_get_connection(account);

  if (gaim_connection_get_state(gc) == GAIM_CONNECTED)
  {
    gaim_debug(GAIM_DEBUG_INFO,"sippy","Somebody's trying to login to something that's already connected - ignoring the request\n");  
    return;
  }

  /* 
   * Do a safety check on the account name
   */


  if (isAccountNameUnsafe(account->username))
  {
    gaim_connection_error(gc,
       _("Login not attempted - The account name is unacceptable"));
    g_free(sipAccountURI);
    return;
  }
  
  sipAccountURI = buildSIPURI(account->username);

  struct simple_connection_cache *sc_cache;
  
  if (gc->proto_data == NULL)
  {
    gaim_debug(GAIM_DEBUG_INFO,"sippy","Creating new simple cache\n");  
    sc_cache = gc->proto_data = g_new0(struct simple_connection_cache,1);
  }
  else
  {
    /* I don't think this will ever happen - RjS */
    gaim_debug(GAIM_DEBUG_INFO,"sippy","Reusing old simple cache\n");  
    sc_cache = gc->proto_data;
  }

  if (sc_cache->friends != NULL)
  {
    /* Likewise, I don't think this will ever happen - RjS */
    gaim_debug(GAIM_DEBUG_INFO,"sippy","Destroying old simple friend list\n");  
    g_hash_table_destroy(sc_cache->friends);
  } 

  sc_cache->friends = g_hash_table_new_full(g_str_hash,
		                            g_str_equal,
					    g_free,
					    simple_friend_cache_free);

  if (!gaginitialized)  
  {
    gaim_debug(GAIM_DEBUG_INFO,"sippy","Initializing gag\n");  
    init_gag(gc);
  }

  if (!gaginitialized)
  {
    gaim_connection_error(gc,
       _("Unable to start the SIP engine - SIP engine not found"));
    g_free(sipAccountURI);
    return;
  }

  gaim_connection_update_progress(gc,"Connecting",0,2);

  /* send the login stuff (aor,userid,password) */
  sippy_send_command( SIMPLE_LOGIN );

  gaim_debug(GAIM_DEBUG_INFO,"sippy","Sending aor [%s]\n",sipAccountURI);
  sippy_send_string( sipAccountURI );


  /* This assumes the digest username is always equal 
   *  to the user part of the AOR */
  gaim_debug(GAIM_DEBUG_INFO,"sippy","Sending username [%s]\n",account->username);  
  sippy_send_string( account->username );

  gaim_debug(GAIM_DEBUG_INFO,"sippy","Sending password [%s]\n",account->password);  
  sippy_send_string( account->password );

  gaim_debug(GAIM_DEBUG_INFO,"sippy",
             "Sending register_with_service flag [%s]\n",
             (gaim_account_get_bool(account,"register_with_service",TRUE)
                                    ?"true":"false"));
  sippy_send_bool(gaim_account_get_bool(account,"register_with_service",TRUE));

  gaim_debug(GAIM_DEBUG_INFO,"sippy",
             "Sending publish_to_service flag [%s]\n", 
             (gaim_account_get_bool(account,"publish_to_service",TRUE)
                                    ?"true":"false"));
  sippy_send_bool(gaim_account_get_bool(account,"publish_to_service",TRUE));

  if (gc->inpa!=0)
  {
    /* I don't think this will ever happen, and it points out a huge
       issue with the current plumbing - trying to login to two SIPPY
       accounts _will_ cause the gagtogaim fd to get put on gaim's
       input queue twice, with different gc's - the callbacks are going
       to happen with very wrong contexts
     */
    gaim_debug(GAIM_DEBUG_INFO,"sippy","Removing old simple gaim_input\n");  
    gaim_input_remove(gc->inpa);
  }
  gaim_debug(GAIM_DEBUG_INFO,"sippy","Adding gag's simple gaim_input\n");  
  gc->inpa = gaim_input_add(FD_GAG_TO_GAIM,GAIM_INPUT_READ,sippy_recv_cb,gc);

  g_free(sipAccountURI);

}


static int 
sippy_send_im(GaimConnection *gc, const char *who, const char *what, GaimConvImFlags flags)
{
   const char* from = gc->account->username;
   const char* to = who;
   const char* im = what;

   assert( FD_GAIM_TO_GAG );

   sippy_send_command( SIMPLE_IM );
   sippy_send_string( to );
   sippy_send_string( from );
   sippy_send_string( im );
   
/*
   m=g_strdup_printf("I saw '%s'",what);
   serv_got_im(gc,who,m,0,time(NULL));
   g_free(m);
*/

   return 1;
}

static void 
sippy_set_presence(GaimConnection *gc, const char *state, const char *msg)
{
    if (!gaginitialized) {return;}
    if (gc->away) {
      g_free(gc->away);
      gc->away = NULL;
    }

   if (msg) { gc->away = g_strdup(msg); }

   gaim_debug(GAIM_DEBUG_INFO,"sippy","sending PRESENCE to us=%s open=%d msg=%s\n",gc->account->username, *state, msg);

   sippy_send_command( SIMPLE_PRESENCE );
   
   sippy_send_string( gc->account->username );
   sippy_send_bool( (unsigned char)(*open) );
   sippy_send_string( msg );

}

static char *
simple_status_text(GaimBuddy *b)
{
  struct simple_connection_cache *sc_cache = 
	  (struct simple_connection_cache *)b->account->gc->proto_data;
  struct simple_friend_cache *fc =
	  g_hash_table_lookup(sc_cache->friends,b->name);

  return ( fc ? g_strdup(fc->status_string) : g_strdup(_("Unknown")) );
  /* Probably want NULL instead of a copy of "Unknown" */

}

static char *
simple_tooltip_text(GaimBuddy *b)
{
  return simple_status_text(b);
}


static GaimPluginProtocolInfo prpl_info =
{
        5,			/* API version number */

				/* GaimProtocolOptions */
	OPT_PROTO_PASSWORD_OPTIONAL,

	NULL,			/* user_splits */
	NULL,			/* protocol_options */
        NO_BUDDY_ICONS,         /* icon_spec */
 	sippy_list_icon, 	/* list_icon */
	NULL, 			/* list_emblems  */
	simple_status_text,	/* status_text */
	simple_tooltip_text,	/* tooltip_text */
	NULL,			/* away_states */
	NULL,			/* blist_node_menu */
	NULL,			/* chat_info */
	sippy_login,		/* login */
	sippy_logout, 		/* close */
	sippy_send_im, 		/* message_send_im */
	NULL, 			/* set_info */
	NULL,			/* send_typing */
	NULL,			/* get_info */
	sippy_set_presence, 	/* set_away */
	NULL,			/* set_idle */
	NULL, 			/* change_passwd */ 
	sippy_add_buddy, 	/* add_buddy */
	NULL,			/* add_buddies */
	sippy_remove_buddy,	/* remove_buddy */
	NULL,			/* remove_buddies */
	NULL,			/* add_permit */
	NULL,			/* add_deny */
	NULL,			/* rem_permit */
	NULL,			/* rem_deny */
	NULL,			/* set_permit_deny */
	NULL,			/* warn */
	NULL,			/* join_chat */
        NULL,                   /* reject_chat */
	NULL,			/* chat_invite */
	NULL,			/* chat_leave */
	NULL,			/* chat_whisper */
	NULL,			/* chat_send */
	NULL,			/* keepalive */
	NULL,			/* register_user */
	NULL,			/* get_cb_info */
	NULL,			/* get_cb_away */
	NULL,			/* alias_buddy */
	NULL,			/* group_buddy */
	NULL,			/* rename_group */
	NULL,			/* buddy_free */
	NULL, 			/* convo_closed */ /* XXX: thread_ids */
	NULL, 			/* normalize */
        NULL,			/* set_buddy_icon */
        NULL,			/* remove_group */
        NULL,			/* get_cb_real_name */
        NULL,			/* set_chat_topic */
        NULL,			/* find_blist_chat */
        NULL,			/* roomlist_get_list */
        NULL,                   /* roomlist_cancel */
        NULL			/* roomlist_expand_category */
};

static GaimPluginInfo info =
{
	4,                                                /**< api_version    */
	GAIM_PLUGIN_PROTOCOL,                             /**< type           */
	NULL,                                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	GAIM_PRIORITY_DEFAULT,                            /**< priority       */

	"prpl-simple",                                    /**< id             */
	"SIP/SIMPLE",                                     /**< name           */
	VERSION,                                          /**< version        */
	                                                  /**  summary        */
	N_("SIP/SIMPLE Protocol Plugin"),
	                                                  /**  description    */
	N_("SIP/SIMPLE Protocol Plugin"),
	NULL,                                             /**< author         */
	GAIM_WEBSITE,                                     /**< homepage       */

	NULL,                                             /**< load           */
	sippy_unload_plugin,                              /**< unload         */
	NULL,                                             /**< destroy        */

	NULL,                                             /**< ui_info        */
	&prpl_info,                                       /**< extra_info     */
        NULL,						  /**< prefs_info     */
        NULL                                              /**< actions */
};

static void
init_plugin(GaimPlugin *plugin)
{
/*
	GaimAccountUserSplit *split;
*/
	GaimAccountOption *option;
/*
        split = gaim_account__split_new(_("Scheme"), "sip", ':');
        prpl_info.user_splits = g_list_prepend(prpl_info.user_splits, split);

        split = gaim_account_user_split_new(_("Server"), "iptel.org", '@');
        prpl_info.user_splits = g_list_append(prpl_info.user_splits, split);
*/

/*
        option = gaim_account_option_string_new(_("Display Name"),
                        "display_name", NULL);
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);

        option = gaim_account_option_string_new(_("Digest Username"),
                        "digest_username", NULL);
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);

        option = gaim_account_option_string_new(_("Digest Realm"),
                        "digest_realm", NULL);
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);

        option = gaim_account_option_string_new(_("Transport"),
                        "transport_protocol", "TCP");
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);

        option = gaim_account_option_int_new(_("Port"), "port", 5060);
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);
*/

        option = gaim_account_option_bool_new(_("Register with this service"),
                        "register_with_service", TRUE );
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);

        option = gaim_account_option_bool_new(_("Publish presence to this service"),
                        "publish_to_service", TRUE );
        prpl_info.protocol_options = g_list_append(prpl_info.protocol_options,
                        option);

	my_protocol = plugin;

	gaim_prefs_add_none("/plugins/prpl/simple");
}


GAIM_INIT_PLUGIN(simple, init_plugin, info);
