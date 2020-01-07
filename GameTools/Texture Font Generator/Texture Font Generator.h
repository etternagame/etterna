#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h" // main symbols

class CTextureFontGeneratorApp : public CWinApp
{
  public:
	CTextureFontGeneratorApp();
	BOOL ProcessMessageFilter(int code, LPMSG lpMsg);

  public:
	virtual BOOL InitInstance();
	HACCEL m_hAccelerators;

	DECLARE_MESSAGE_MAP()
};

extern CTextureFontGeneratorApp theApp;
