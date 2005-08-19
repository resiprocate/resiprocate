#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "GagMessage.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

// Yeah, this is kind of ugly, but I can't think of
// a good pattern for this off the top of my head.

GagMessage *
GagMessage::getMessage(int in_fd)
{
  int type;

  if (GagMessage::readAll(in_fd,(char *)&type, sizeof(type)) < 0 )
  { 
    return NULL; 
  }

  DebugLog ( << "Reading message of type " << type);

  switch (type)
  {
    case IM:
        return new GagImMessage(in_fd);
      break;

    case PRESENCE:
        return new GagPresenceMessage(in_fd);
      break;

    case HELLO:
        return new GagHelloMessage(in_fd);
      break;

    case LOGIN:
        return new GagLoginMessage(in_fd);
      break;

    case LOGOUT:
        return new GagLogoutMessage(in_fd);
      break;

    case ADD_BUDDY:
        return new GagAddBuddyMessage(in_fd);
      break;

    case REMOVE_BUDDY:
        return new GagRemoveBuddyMessage(in_fd);
      break;

   case SHUTDOWN:
        return new GagShutdownMessage(in_fd);
      break;

    // Technically, we shouldn't get these. But, oh well.
    case GAG_ERROR:
        return new GagErrorMessage(in_fd);
      break;

    case LOGIN_STATUS:
        return new GagLoginStatusMessage(in_fd);
      break;

    default:
        // Something is HORRIBLY WRONG.
        // This will be logged in the main loop; we just
        // return NULL here to let him know something is awry.
        return 0;
      break;
  }
}

// Spit out header
ostream &
GagMessage::serialize(ostream &os) const
{
    int t = static_cast<int>(messageType);
    os.write(reinterpret_cast<char *>(&t), sizeof(t));
    return os;
}

// Helper functions for I/O

void
GagMessage::serialize(ostream &os, const Data& data)
{
  int size = int(data.size());
  os.write(reinterpret_cast<char *>(&size), sizeof(int));
  os.write(data.data(), int(data.size()) );
}

void
GagMessage::serialize(ostream &os, const Uri& uri)
{
  serialize(os, Data::from(uri));
}

void
GagMessage::serialize(ostream &os, const bool& flag)
{
  int size = 1;
  os.write(reinterpret_cast<char *>(&size), sizeof(int));
  os.put(flag?1:0);
}

void
GagMessage::serialize(ostream &os, const int& value)
{
  int size = sizeof(int);
  os.write(reinterpret_cast<char *>(&size), sizeof(int));
  os.write(reinterpret_cast<const char *>(&value), sizeof(int));
}

bool
GagMessage::parse(int in_fd, Data &data)
{
  int size;
  char *temp;

  if ( GagMessage::readAll(in_fd,(char *)&size, sizeof(size)) < 0 )
  { 
    return (false); 
  }
  temp=(char *)malloc(size);
  if (!temp) return (false);
  if ( GagMessage::readAll(in_fd,temp, size) < 0 )
  {
    return (false);
  }
  data = Data(temp, size);
  free(temp);
  return true;
}

bool
GagMessage::parse(int in_fd, Uri &uri)
{
  int size;
  char *temp;

  if (GagMessage::readAll(in_fd,(char *)&size, sizeof(size)) < 0 )
  {
    return (false);
  }
  temp=(char *)malloc(size);
  if (!temp) return (false);
  if (GagMessage::readAll(in_fd,temp, size) < 0 )
  {
    return (false);
  }
  DebugLog ( << "Parsing URI from GAIM: " << Data(temp,size));
  uri = Uri(Data(temp,size));
  free(temp);
  return true;
}

bool
GagMessage::parse(int in_fd, bool &flag)
{
  int size;
  char *temp;

  if (GagMessage::readAll(in_fd,(char *)&size, sizeof(size)) < 0 )
  {
     return (false);
  }
  temp=(char *)malloc(size);
  if (!temp) return (false);
  if ( GagMessage::readAll(in_fd,temp, size) < 0 )
  {
    return (false);
  }
  flag = (temp[0]?true:false);
  free(temp);
  return true;
}

bool
GagMessage::parse(int in_fd, int &value)
{
  int size;
  char *temp;

  if ( GagMessage::readAll(in_fd,(char *)&size, sizeof(size)) < 0 )
  {
    return (false);
  }
  temp=(char *)malloc(size);
  if (!temp) return (false);
  if ( GagMessage::readAll(in_fd,temp, size) < 0 )
  {
    return (false);
  }
  value = *(reinterpret_cast<int *>(temp));
  free(temp);
  return true;
}


ssize_t
GagMessage::readAll(int fd, char* target, size_t length)
{

  DebugLog ( << "Attempting to read " << length << " bytes from peer");

  size_t remaining = length;
  size_t offset = 0;
  while (remaining)
  {
    size_t readSize = read(fd,target+offset,remaining);

    DebugLog ( << "  Read " << length << " bytes");

    if ( readSize == -1 )
    {
       DebugLog ( << "  Which is, of course, erroneous");
       return -1;
    }
    remaining -= readSize;
    offset += readSize;
  }
  return length;
}

void
GagImMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, to)) return;
  if (!GagMessage::parse(in_fd, from)) return;
  if (!GagMessage::parse(in_fd, im)) return;
  DebugLog ( << "Got IM from Gaim [from = '" << from 
             << "' to = '" << to << "' im = '" << im << "']");
  valid = true;
}

ostream &
GagImMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending IM to Gaim [from = '" << from 
             << "' to = '" << to << "' im = '" << im << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, to);
  GagMessage::serialize(os, from);
  GagMessage::serialize(os, im);
  os.flush();

  return os;
}

void
GagPresenceMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, aor)) return;
  if (!GagMessage::parse(in_fd, available)) return;
  if (!GagMessage::parse(in_fd, status)) return;
  DebugLog ( << "Got presence from Gaim [aor = '" << aor 
             << "' available = '" << (available ? "true" : "false")
             << "' status = '" << status << "']");
  valid = true;
}

ostream &
GagPresenceMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending presence to Gaim [aor = '" << aor 
             << "' available = '" << (available ? "true" : "false")
             << "' status = '" << status << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, aor);
  GagMessage::serialize(os, available);
  GagMessage::serialize(os, status);
  os.flush();

  return os;
}

void
GagHelloMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, ok)) return;
  DebugLog ( << "Got hello from Gaim [ ok = '" << (ok?"true":"false") << "']");
  valid = true;
}

ostream &
GagHelloMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending hello to Gaim [ ok = '"
             << (ok?"true":"false") << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, ok);
  os.flush();

  return os;
}

void
GagLoginMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, aor)) return;
  if (!GagMessage::parse(in_fd, userid)) return;
  if (!GagMessage::parse(in_fd, password)) return;
  if (!GagMessage::parse(in_fd, register_with_service)) return;
  if (!GagMessage::parse(in_fd, publish_to_service)) return;
  DebugLog ( << "Got login from Gaim [aor = '" << aor 
             << "' userid = '" << userid 
             << "' password = '" << password
             << "' register_with_service = '" << register_with_service
             << "' publish_to_service = '" << publish_to_service
             << "']");
  valid = true;
}

ostream &
GagLoginMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending login to Gaim [aor = '" << aor 
             << "' userid = '" << userid 
             << "' password = '" << password << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, aor);
  GagMessage::serialize(os, userid);
  GagMessage::serialize(os, password);
  os.flush();

  return os;
}

void
GagLogoutMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, aor)) return;
  DebugLog ( << "Got logout from Gaim [aor = '" << aor << "']");
  valid = true;
}

ostream &
GagLogoutMessage::serialize(ostream &os) const
{
  GagMessage::serialize(os);
  GagMessage::serialize(os, aor);
  DebugLog ( << "Sending logout to Gaim [aor = '" << aor << "']");
  os.flush();
  return os;
}

void
GagAddBuddyMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, us)) return;
  if (!GagMessage::parse(in_fd, them)) return;
  DebugLog ( << "Got addbuddy from Gaim [them = '" << them 
             << "' us = '" << us << "']");
  valid = true;
}

ostream &
GagAddBuddyMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending addbuddy to Gaim [them = '" << them 
             << "' us = '" << us << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, us);
  GagMessage::serialize(os, them);
  os.flush();
  return os;
}


void
GagRemoveBuddyMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, us)) return;
  if (!GagMessage::parse(in_fd, them)) return;
  DebugLog ( << "Got removebuddy from Gaim [them = '" << them 
             << "' us = '" << us << "']");
  valid = true;
}

ostream &
GagRemoveBuddyMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending removebuddy to Gaim [them = '" << them 
             << "' us = '" << us << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, us);
  GagMessage::serialize(os, them);
  os.flush();
  return os;
}

void
GagErrorMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, message)) return;
  DebugLog ( << "Got error from Gaim [message = '" << message << "']");
  valid = true;
}

ostream &
GagErrorMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending error to Gaim [message = '" << message << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, message);
  os.flush();
  return os;
}

void
GagLoginStatusMessage::parse(int in_fd)
{
  valid = false;
  if (!GagMessage::parse(in_fd, success)) return;
  if (!GagMessage::parse(in_fd, sipCode)) return;
  if (!GagMessage::parse(in_fd, message)) return;
  DebugLog ( << "Got loginstatus from Gaim [success = '" 
             << (success ? "true":"false")
             << "' sipcode = '" << sipCode
             << "' message = '" << message << "']");
  valid = true;
}

ostream &
GagLoginStatusMessage::serialize(ostream &os) const
{
  DebugLog ( << "Sending loginstatus to Gaim [success = '" 
             << (success ? "true":"false")
             << "' sipcode = '" << sipCode
             << "' message = '" << message << "']");
  GagMessage::serialize(os);
  GagMessage::serialize(os, success);
  GagMessage::serialize(os, sipCode);
  GagMessage::serialize(os, message);
  os.flush();
  return os;
}
