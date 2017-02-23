#include "tinyxmlAdd.h"
#ifdef _MSC_VER
	#pragma warning( disable : 4996 )
#endif
bool GetTextElementToBuffer(TiXmlElement *elementToGet,const char *keyName,char *buffer,unsigned int length)
{
	assert(keyName);

	TiXmlElement *ele = elementToGet->FirstChildElement(keyName);
	if (ele==NULL)	{		return false;	}

	const char *pText = ele->GetText();
	if(pText==NULL)return false;
	if (length<strlen(pText))
	{
		return false;
	}
	strcpy(buffer,pText);
	return true;
}


// EOF
