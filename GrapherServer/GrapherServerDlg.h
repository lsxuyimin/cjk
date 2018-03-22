// GrapherServerDlg.h : header file
//

#if !defined(AFX_GRAPHERSERVERDLG_H__DEAF2BF2_F88C_45D9_9072_F16D61E10658__INCLUDED_)
#define AFX_GRAPHERSERVERDLG_H__DEAF2BF2_F88C_45D9_9072_F16D61E10658__INCLUDED_


#include "WCDPcie\\WCDPcieDriver.h"  

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define ListenPort 9603
#define ACKPort 9607
#define Mega 1024*1024//1 MB
#define N    5
/////////////////////////////////////////////////////////////////////////////
// CGrapherServerDlg dialog

class CGrapherServerDlg : public CDialog
{
// Construction
public:
	CGrapherServerDlg(CWnd* pParent = NULL);	// standard constructor
	~CGrapherServerDlg();
	char* getHostIP(); 
	void OnPreSample(); 
	void getStatus(SOCKET serSock);	
	//UINT ServerThread(LPVOID pParam);
// Dialog Data
	//{{AFX_DATA(CGrapherServerDlg)
	enum { IDD = IDD_GRAPHERSERVER_DIALOG };
	CButton	m_start;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGrapherServerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGrapherServerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStart();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
public:
	bool m_bIsRuning;
	int m_flag;
	char m_szHostIP[30];
	long m_SampleCHNum;
	unsigned char *m_DataN[8];
	unsigned char *m_Data;
	unsigned short **m_ShortData;
	float **m_DoubleData;
	long		  m_DeviceNum;
	long          m_CardNum;
	long          m_SelDevice;
	unsigned long m_BusIDArray[32];
	unsigned long m_SlotIDArray[32];
	long m_sf;//触发速率
	long m_sl;//触发长度
	long m_tt;//触发方式
	float m_tul;//触发上限
	float m_tll;//触发下限
	CString m_statustxt;
	bool m_saved;
	long m_sucnum;
	CString m_startSrcIP;
	BOOL m_bRecvReady;
	char* m_SendBuf;
	
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHERSERVERDLG_H__DEAF2BF2_F88C_45D9_9072_F16D61E10658__INCLUDED_)
