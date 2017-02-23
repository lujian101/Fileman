
// Fileman.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CFilemanApp:
// See Fileman.cpp for the implementation of this class
//

class CFilemanApp : public CWinApp
{
public:
	CFilemanApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual BOOL ExitInstance();
// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CFilemanApp theApp;