// Grapher.h : main header file for the GRAPHER application
//

#if !defined(AFX_GRAPHER_H__BAF12605_CBF0_40EC_BEFD_353A212C4D8B__INCLUDED_)
#define AFX_GRAPHER_H__BAF12605_CBF0_40EC_BEFD_353A212C4D8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CGrapherApp:
// See Grapher.cpp for the implementation of this class
//

class CGrapherApp : public CWinApp
{
public:
	CGrapherApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGrapherApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CGrapherApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHER_H__BAF12605_CBF0_40EC_BEFD_353A212C4D8B__INCLUDED_)
