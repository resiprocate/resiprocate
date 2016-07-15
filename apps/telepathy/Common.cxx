/*
    Copyright (C) 2015 Niels Ole Salscheider <niels_ole@salscheider-online.de>
    Copyright (C) 2015 Daniel Pocock http://danielpocock.com

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
    LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
    OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "Common.hxx"

using namespace tr;

Tp::SimpleStatusSpecMap Common::getSimpleStatusSpecMap()
{
    Tp::SimpleStatusSpec spOffline;
    spOffline.type = Tp::ConnectionPresenceTypeOffline;
    spOffline.maySetOnSelf = true;
    spOffline.canHaveMessage = false;

    Tp::SimpleStatusSpec spAvailable;
    spAvailable.type = Tp::ConnectionPresenceTypeAvailable;
    spAvailable.maySetOnSelf = true;
    spAvailable.canHaveMessage = true;

    Tp::SimpleStatusSpec spAway;
    spAway.type = Tp::ConnectionPresenceTypeAway;
    spAway.maySetOnSelf = true;
    spAway.canHaveMessage = true;

    Tp::SimpleStatusSpec spXa;
    spXa.type = Tp::ConnectionPresenceTypeExtendedAway;
    spXa.maySetOnSelf = true;
    spXa.canHaveMessage = true;

    Tp::SimpleStatusSpec spDnd;
    spDnd.type = Tp::ConnectionPresenceTypeBusy;
    spDnd.maySetOnSelf = true;
    spDnd.canHaveMessage = true;

    Tp::SimpleStatusSpec spChat;
    spChat.type = Tp::ConnectionPresenceTypeAvailable;
    spChat.maySetOnSelf = true;
    spChat.canHaveMessage = true;

    Tp::SimpleStatusSpec spHidden;
    spHidden.type = Tp::ConnectionPresenceTypeHidden;
    spHidden.maySetOnSelf = true;
    spHidden.canHaveMessage = true;

    Tp::SimpleStatusSpec spUnknown;
    spUnknown.type = Tp::ConnectionPresenceTypeUnknown;
    spUnknown.maySetOnSelf = false;
    spUnknown.canHaveMessage = false;

    Tp::SimpleStatusSpecMap specs;
    specs.insert(QLatin1String("offline"), spOffline);
    specs.insert(QLatin1String("available"), spAvailable);
    specs.insert(QLatin1String("away"), spAway);
    specs.insert(QLatin1String("xa"), spXa);
    specs.insert(QLatin1String("dnd"), spDnd);
    specs.insert(QLatin1String("chat"), spChat);
    specs.insert(QLatin1String("hidden"), spHidden);
    specs.insert(QLatin1String("unknown"), spUnknown);
    return specs;
}

Tp::AvatarSpec Common::getAvatarSpec()
{
    return Tp::AvatarSpec(QStringList() << QLatin1String("image/png") << QLatin1String("image/jpeg"),
                          0 /* minHeight */,
                          512 /* maxHeight */,
                          256 /* recommendedHeight */,
                          0 /* minWidth */,
                          512 /* maxWidth */,
                          256 /* recommendedWidth */,
                          1024 * 1024 /* maxBytes */);
}


