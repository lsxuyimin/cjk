// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__24931C76_92A6_45FB_8260_34782329F0EB__INCLUDED_)
#define AFX_STDAFX_H__24931C76_92A6_45FB_8260_34782329F0EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include "ZMQ\\zmq.h"
#include "ZMQ\\zmq_utils.h"
#include <afxsock.h>		// MFC socket extensions
#include <stdlib.h>
//#include<winsock.h>
//#pragma comment(lib,'ws2_32')
//#include<sys/socket.h>
//#include<netinet/in.h>
//#include<arpa/inet.h>
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__24931C76_92A6_45FB_8260_34782329F0EB__INCLUDED_)
