#include "resiprocate/sam/DialogSet.hxx"

using namespace resip;
using namespace std;

DialogSet::DialogSet(const BaseCreator *creator) : 
    mDialogs(),
    mCreator(creator)
{
}

DialogSet::DialogSet(Dialog* dialog) :
    mDialogs(),
    mCreator(NULL)
{
    mDialogs.push_back(dialog);
}

DialogSet::~DialogSet()
{
}

DialogSetId
Dialogset::getId()
{
    return mId;
}

void
DialogSet::addDialog(Dialog *dialog)
{
    mDialogs.push_back(dialog);
}

void 
DialogSet::removeDialog(const Dialog *dialog)
{
    mDialogs.remove(dialog);
}

DialogIdSet 
DialogSet::getDialogs() const
{
    /** @todo Implment this method */
}

Dialog* 
DialogSet::findDialog(const DialogId id)
{
    std::list<Dialog*>::iterator it = mDialogs.start();
    Dialog *dialog;
    while (it != mDialogs.end())
    {
        dialog  = it.next();
        if (dialog->getId() == id)
        {
            return dialog;
        }
    }
    return NULL;
}

Dialog*
Dialogset::findDialog( const Data& otherTag )
{
    /** @todo Implment this method */
    return NULL;
}

Dialog* findDialog(SipMessage& msg)
{
    /** @todo Implment this method */
    return NULL;
}

BaseCreator* 
DialogSet::getCreator()
{
    return mCreator;
}
    
