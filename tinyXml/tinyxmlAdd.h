/********************************************************************
	author:		wjy
	
	purpose:	create this file to fast use tinyxml
				all the function return data must be with the operater << or >>
*********************************************************************/

#ifndef _TINY_XML_FAST_USE_H_
#define _TINY_XML_FAST_USE_H_

#include "tinyxml.h"
#include <sstream>
#include <assert.h>

template<typename T>
void LinkTextElementToEndChild(TiXmlElement *elementLinkTo,const char *keyName,T value)
{
	assert(elementLinkTo);
	assert(keyName);
	
	TiXmlElement *eleToBeLink = new TiXmlElement(keyName);
	assert(eleToBeLink);
	elementLinkTo->LinkEndChild(eleToBeLink);

	std::stringstream bufferstring;
	bufferstring << value;

	TiXmlText *text = new TiXmlText(bufferstring.str().c_str());
	assert(text);
	eleToBeLink->LinkEndChild(text);

}


template<typename T>
bool GetTextElementValue(TiXmlElement *elementToGet,const char *keyName,T &value)
{
	assert(elementToGet);
	assert(keyName);

	TiXmlElement *ele = elementToGet->FirstChildElement(keyName);
	if (ele==NULL)	{		return false;	}

	const char *pText = ele->GetText();
	if (pText==NULL){		return false;	}

	std::stringstream bufferstring(pText);
	bufferstring >> value;

	return true;
}

bool GetTextElementToBuffer(TiXmlElement *elementToGet,const char *keyName,char *buffer,unsigned int length);

template<typename T>
void SetAttribute(TiXmlElement *elementToSet,const char *attributeName,T value)
{
	assert(elementToSet);
	assert(attributeName);


	std::stringstream bufferstring;
	bufferstring << value;

	elementToSet->SetAttribute(attributeName,bufferstring.str().c_str());

}


template<typename T>
bool GetAttribute(TiXmlElement *elementToSet,const char *attributeName,T value)
{
	assert(elementToSet);
	assert(attributeName);

	const char *pText = elementToSet->Attribute(attributeName);
	if (pText==NULL)
	{
		return false;
	}
	std::stringstream bufferstring(pText);
	bufferstring >> value;

	return true;
}


#endif




