/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include <string>
#include "rutil/SimpleXMLElement.hxx"
namespace resip{
SimpleXMLElement::SimpleXMLElement(std::string const &name)
    :mname(name){}

SimpleXMLElement::SimpleXMLElement(std::string const &name, std::string const &body)
    :mname(name)
{
    setBody(body);    
}

std::string * SimpleXMLElement::toString()
{
    std::string * rendered = new std::string("<");
    *rendered += mname + mattributes;
    if(mbody == ""){
        *rendered += "/>";
        return rendered;
    }
    *rendered += ">"+ mbody +"</"+mname+">";
    return rendered;
}

void SimpleXMLElement::setName(std::string const &name)
{
    mname = name;
}

void SimpleXMLElement::addAttribute(std::string const &name, std::string const &value){
    mattributes += " " + name + "=\"";
    int c = 0;
    while(value[c] != 0)
    {
        switch(value[c]){
            case('&'):{
                mattributes += "&#37;";
                break;
            }
            case('<'):{
                mattributes += "&lt;";
                break;
            }
            case('>'):{
                mattributes += "&gt;";
                break;
            }
            case('"'):{
                mattributes += "&#34;";
                break;
            }
            case('\''):{
                mattributes += "&#39;";
                break;
            }
            default:{
                mattributes += value[c];
            }
        }
        c++;
    }
    mattributes +=  "\"";
}

void SimpleXMLElement::setBody(const std::string & text){
    mbody="";
    int c = 0;
    while(text[c] != 0)
    {
        switch(text[c]){
            case('&'):{
                mbody += "&#37;";
                break;
            }
            case('<'):{
                mbody += "&lt;";
                break;
            }
            case('>'):{
                mbody += "&gt;";
                break;
            }
            default:{
                mbody += text[c];
            }
        }
        c++;
    }
}
}

/* Copyright 2007 Estacado Systems */

