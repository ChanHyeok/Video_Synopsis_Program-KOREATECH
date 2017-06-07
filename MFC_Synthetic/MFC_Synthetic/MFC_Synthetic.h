
// MFC_Synthetic.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "io.h"		// access ÇÔ¼ö


#define RESULT_TEXT_FILENAME  "frameInfo_"
#define RESULT_FOLDER_NAME "segment"

// CMFC_SyntheticApp:
// See MFC_Synthetic.cpp for the implementation of this class
//

class CMFC_SyntheticApp : public CWinApp
{
public:
	CMFC_SyntheticApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CMFC_SyntheticApp theApp;