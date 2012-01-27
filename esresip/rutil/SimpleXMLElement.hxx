/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef __SIMPLEXMLELEMENT__
#define __SIMPLEXMLELEMENT__
#include "rutil/compat.hxx"
#include <string>

namespace resip{
/**
    @brief A simple element is an XML element with no child elements, only
    attributes and a text body. 
	
	The text of the body and values are
    mutated to avoid illegal XML body characters. The name and and
    names of the attributes are NOT validated.
*/
class SimpleXMLElement
{
    protected:
        std::string mattributes; ///< The attributes (stored pre-rendered)
        std::string mbody; ///< The body (stored pre-rendered)
        std::string mname; ///< The name of the tag
    public:
        /** @brief Renders a new attribute/value pair and adds it to the attribute string */
        void addAttribute(std::string const &name, std::string const &value);
        /** @brief Renders the body of the element using the given character data*/
        void setBody(std::string const &text);
        /** @brief Sets the name of the element */
        void setName(std::string const &name);
        /** @brief Returns a string representation of the element */
        std::string * toString();
        /** @brief Constructor that sets the name of the element */
        SimpleXMLElement(std::string const &name);
        /** @brief Constructor that sets the name of the element and renders its body */
        SimpleXMLElement(std::string const &name, std::string const &body);
};
}
#endif

/* Copyright 2007 Estacado Systems */

