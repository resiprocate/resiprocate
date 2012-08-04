
#ifndef __XMLHELPERS_H
#define __XMLHELPERS_H

#include <xercesc/dom/DOMCharacterData.hpp>
#include <xercesc/util/XMLString.hpp>

#include <rutil/Data.hxx>

XERCES_CPP_NAMESPACE_USE

class StrX
{
public :
    // -----------------------------------------------------------------------
    //  Constructors and Destructor
    // -----------------------------------------------------------------------
    StrX(const XMLCh* const toTranscode)
    {
        // Call the private transcoding method
        fLocalForm = XMLString::transcode(toTranscode);
    }

    ~StrX()
    {
        XMLString::release(&fLocalForm);
    }
    // -----------------------------------------------------------------------
    //  Getter methods
    // -----------------------------------------------------------------------
    const char* localForm() const
    {
        return fLocalForm;
    }

private :
    // -----------------------------------------------------------------------
    //  Private data members
    //
    //  fLocalForm
    //      This is the local code page form of the string.
    // -----------------------------------------------------------------------
    char*   fLocalForm;
};



inline resip::Data NodeToData(DOMCharacterData *node) {
   if(node == NULL)
      return resip::Data("");
   return resip::Data(StrX(node->getData()).localForm());
}



#endif

