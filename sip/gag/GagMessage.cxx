#include "GagMessage.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::APP

// Yeah, this is kind of ugly, but I can't think of
// a good pattern for this off the top of my head.

GagMessage *
GagMessage::getMessage(istream &is)
{
  int type;

  is.read((char *)&type, sizeof(type));

  DebugLog ( << "Reading message of type " << type);

  switch (type)
  {
    case IM:
        return new GagImMessage(is);
      break;

    case PRESENCE:
        return new GagPresenceMessage(is);
      break;

    case HELLO:
        return new GagHelloMessage(is);
      break;

    case LOGIN:
        return new GagLoginMessage(is);
      break;

    case LOGOUT:
        return new GagLogoutMessage(is);
      break;

    case ADD_BUDDY:
        return new GagAddBuddyMessage(is);
      break;

    case REMOVE_BUDDY:
        return new GagRemoveBuddyMessage(is);
      break;

   case SHUTDOWN:
        return new GagShutdownMessage(is);
      break;

    // Technically, we shouldn't get these. But, oh well.
    case ERROR:
        return new GagErrorMessage(is);
      break;

    case LOGIN_STATUS:
        return new GagLoginStatusMessage(is);
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
  int size = data.size();
  os.write(reinterpret_cast<char *>(&size), sizeof(int));
  os.write(data.data(), data.size());
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
GagMessage::parse(istream &is, Data &data)
{
  int size;
  char *temp;

  is.read((char *)&size, sizeof(size));
  temp=(char *)malloc(size);
  if (!temp) return (false);
  is.read(temp, size);
  data = Data(temp, size);
  free(temp);
  return true;
}

bool
GagMessage::parse(istream &is, Uri &uri)
{
  int size;
  char *temp;

  is.read((char *)&size, sizeof(size));
  temp=(char *)malloc(size);
  if (!temp) return (false);
  is.read(temp, size);
  DebugLog ( << "Parsing URI from GAIM: " << Data(temp,size));
  uri = Uri(Data(temp,size));
  free(temp);
  return true;
}

bool
GagMessage::parse(istream &is, bool &flag)
{
  int size;
  char *temp;

  is.read((char *)&size, sizeof(size));
  temp=(char *)malloc(size);
  if (!temp) return (false);
  is.read(temp, size);
  flag = (temp[0]?true:false);
  free(temp);
  return true;
}

bool
GagMessage::parse(istream &is, int &value)
{
  int size;
  char *temp;

  is.read((char *)&size, sizeof(size));
  temp=(char *)malloc(size);
  if (!temp) return (false);
  is.read(temp, size);
  value = *(reinterpret_cast<int *>(temp));
  free(temp);
  return true;
}

void
GagImMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, to)) return;
  if (!GagMessage::parse(is, from)) return;
  if (!GagMessage::parse(is, im)) return;
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
GagPresenceMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, aor)) return;
  if (!GagMessage::parse(is, available)) return;
  if (!GagMessage::parse(is, status)) return;
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
GagHelloMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, ok)) return;
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
GagLoginMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, aor)) return;
  if (!GagMessage::parse(is, userid)) return;
  if (!GagMessage::parse(is, password)) return;
  DebugLog ( << "Got login from Gaim [aor = '" << aor 
             << "' userid = '" << userid 
             << "' password = '" << password << "']");
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
GagLogoutMessage::parse(istream &is)
{
  valid = false;
  DebugLog ( << "Got logout from Gaim [aor = '" << aor << "']");
  if (!GagMessage::parse(is, aor)) return;
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
GagAddBuddyMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, us)) return;
  if (!GagMessage::parse(is, them)) return;
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
GagRemoveBuddyMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, us)) return;
  if (!GagMessage::parse(is, them)) return;
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
GagErrorMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, message)) return;
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
GagLoginStatusMessage::parse(istream &is)
{
  valid = false;
  if (!GagMessage::parse(is, success)) return;
  if (!GagMessage::parse(is, sipCode)) return;
  if (!GagMessage::parse(is, message)) return;
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
