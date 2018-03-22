// GrapherDlg.h : header file
//

#if !defined(AFX_GRAPHERDLG_H__5A5E1425_CB86_4DDC_941D_6F996D90CD35__INCLUDED_)
#define AFX_GRAPHERDLG_H__5A5E1425_CB86_4DDC_941D_6F996D90CD35__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WCDPcie//WCDPcieDriver.h"
#define Mega 1024*1024
#define N    5
#define lanHostsNum 200
#define ListenPort 9607
#define ACKPort 9603

#define GET_HOST_COMMAND "GetIPAddr"
const int MAX_BUF_LEN = 255;

//#define MAX_HOST_NUM 256

#define LINE_SIZE 256

#define BIF_NEWDIALOGSTYLE 0x0040

#include <Vector>
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CGrapherDlg dialog
typedef struct tagIPInfo  
{  
	char ip[30];  
}IPInfo;  
//枚举局域网电脑相关
struct LanHostInfo
{ 
	char name[100];
	char ip[30];
	int cardnum;
};
class CGrapherDlg : public CDialog
{
// Construction
public:
	CGrapherDlg(CWnd* pParent = NULL);	// standard constructor
	~CGrapherDlg();
// Dialog Data
	//{{AFX_DATA(CGrapherDlg)
	enum { IDD = IDD_GRAPHER_DIALOG };
	CEdit	m_status;
	CStatic	m_sld2txt;
	CStatic	m_sld1txt;
	CComboBox	m_combo3;
	CComboBox	m_combo2;
	CComboBox	m_combo1;
	CSliderCtrl	m_slider2;
	CSliderCtrl	m_slider1;
	CListCtrl	m_list2;
	CListCtrl	m_list1;
	CIPAddressCtrl	m_IP;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGrapherDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL
public:
	long m_SampleCHNum;
	BOOL m_list1Lock;	
	/*unsigned char *m_DataN[8];
	unsigned char *m_Data;
	unsigned short **m_ShortData;
	float **m_DoubleData;
	long		  m_DeviceNum;
	long          m_CardNum;
	long          m_SelDevice;
	unsigned long m_BusIDArray[32];
	unsigned long m_SlotIDArray[32];*/
	unsigned char *m_DataN[8];
	unsigned short **m_ShortData;
	char m_szHostIP[30];
	CString m_strHisFilePath;//历史记录保存路径
	CString m_strSampleSavePath;//采样成功保存路径
	char m_chFileIP[30];
public:
	char* getHostIP(); 
	struct LanHostInfo* m_lanhosts;
	int m_hostcount;
	int getLanHosts();
	void getHisFile(CString path, vector<CString> *m_FileList, int& count);
	char* ReadSpeacialLine(char* filename, int whichLine);
	//static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData);
	int OnBrowseforfolder(CString dlgTitle,bool IsOpenHis);
	void getPathFromCfg(bool IsOpenHis);
	void InsertHisRecord();
	long HisFileCount()
	{
		return m_FileList->size();
	}	
	void CustomdrawSlider1();
	void CustomdrawSlider2();
// Implementation
protected:
	//void OnPreSample();
private:
	
	vector<CString> *m_FileList;
	//static CString m_filePath;
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGrapherDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStartsample();
	afx_msg void OnStopsample();		
	afx_msg void OnCustomdrawSlider1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomdrawSlider2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOpenRecord();
	afx_msg void OnDeleteRecord();
	afx_msg void OnEditchangeCombo2();
	afx_msg void OnEditchangeCombo3();
	afx_msg void OnEditchangeCombo4();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GRAPHERDLG_H__5A5E1425_CB86_4DDC_941D_6F996D90CD35__INCLUDED_)
