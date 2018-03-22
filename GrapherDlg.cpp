// GrapherDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Grapher.h"
#include "GrapherDlg.h"

#include "dev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL SampleEnd=FALSE;
/////////////////////////////////////////////////////////////////////////////
// CGrapherDlg dialog

CGrapherDlg::CGrapherDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGrapherDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//��ʼ������
	WORD wVersionRequested;   
    WSADATA wsaData;   
    int err;     
    // ����socket api   
    wVersionRequested = MAKEWORD( 2, 2 );   
    err = WSAStartup( wVersionRequested, &wsaData );   
    if ( err != 0 )   
    {   
        return;   
    }   
  
    if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 )   
    {   
            WSACleanup( );   
            return;    
    }   
	//��ʼ����Ա����
	m_list1Lock = FALSE;
	m_SampleCHNum=8;
	memset(m_szHostIP,0,30);
	m_lanhosts = new LanHostInfo[lanHostsNum];
	m_hostcount = 0;
	m_FileList = new vector<CString>;
	for(int i=0;i<8;i++)
	{
		m_DataN[i]=new unsigned char[Mega*N];
	}
	//m_ShortData
	m_ShortData=new unsigned short*[m_SampleCHNum];
	for(i=0;i<m_SampleCHNum;i++)
	{
		m_ShortData[i]=new unsigned short[Mega*N];
	}
}

CGrapherDlg::~CGrapherDlg()
{
	delete m_FileList;
	WSACleanup(); 
}

void CGrapherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGrapherDlg)
	DDX_Control(pDX, IDC_EDIT1, m_status);
	DDX_Control(pDX, IDC_SLD2TEXT, m_sld2txt);
	DDX_Control(pDX, IDC_SLD1TEXT, m_sld1txt);
	DDX_Control(pDX, IDC_COMBO4, m_combo3);
	DDX_Control(pDX, IDC_COMBO3, m_combo2);
	DDX_Control(pDX, IDC_COMBO2, m_combo1);
	DDX_Control(pDX, IDC_SLIDER2, m_slider2);
	DDX_Control(pDX, IDC_SLIDER1, m_slider1);
	DDX_Control(pDX, IDC_LIST2, m_list2);
	DDX_Control(pDX, IDC_LIST1, m_list1);
	DDX_Control(pDX, IDC_IPADDRESS1, m_IP);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGrapherDlg, CDialog)
	//{{AFX_MSG_MAP(CGrapherDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_STARTSAMPLE, OnStartsample)
	ON_BN_CLICKED(IDC_STOPSAMPLE, OnStopsample)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, OnCustomdrawSlider1)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER2, OnCustomdrawSlider2)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST2, OnDblclkList2)
	ON_BN_CLICKED(IDC_BUTTON1, OnOpenRecord)
	ON_BN_CLICKED(IDC_BUTTON2, OnDeleteRecord)
	//ON_CBN_EDITCHANGE(IDC_COMBO2, OnEditchangeCombo2)
	//ON_CBN_EDITCHANGE(IDC_COMBO3, OnEditchangeCombo3)
	//ON_CBN_EDITCHANGE(IDC_COMBO4, OnEditchangeCombo4)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGrapherDlg message handlers
//�ļ������߳�
UINT RecvFileThread(LPVOID lpara)
{
	CGrapherDlg *pPCIeDlg=(CGrapherDlg*)lpara;
	void * pCtx = NULL;
    void * pSock = NULL;
    const char * pAddr = "tcp://*:7766";

    //����context��zmq��socket ��Ҫ��context�Ͻ��д��� 
    if((pCtx = zmq_ctx_new()) == NULL)
    {
        return 0;
    }
    //����zmq socket ��socketĿǰ��6������ ������ʹ��dealer��ʽ
    //����ʹ�÷�ʽ��ο�zmq�ٷ��ĵ���zmq�ֲᣩ 
    if((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
    {
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    int iRcvTimeout = 5000;// millsecond
    //����zmq�Ľ��ճ�ʱʱ��Ϊ5�� 
    if(zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    //�󶨵�ַ tcp://*:7766 
    //Ҳ����ʹ��tcpЭ�����ͨ�ţ�ʹ������˿� 7766
    if(zmq_bind(pSock, pAddr) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }
	char szHeader[80]={0};
	unsigned char *szMsg = new unsigned char[N*Mega];
	//char szMsg[N*Mega]={0};
	char szCharacter[10]={0},szCardNum[5]={0},szChannel[5]={0},szSplitPK[5]={0},szPKID[5]={0},szPKLength[5]={0};
	char szSrcIP[30]={0}; 
	//int iListItemCount = pPCIeDlg->m_list1.GetItemCount();
	int iSpliPK,iPKID,iCardID;
	FILE *fp;
	while(TRUE)
	{
		//���հ�ͷ
		long lRet = zmq_recv(pSock, szHeader, 80,0);		
		if(lRet == 80)//�յ�����
		{
			memcpy(szCharacter,szHeader,10);
			memcpy(szSrcIP,szHeader+10,30);//��ԴIP
			//memcpy(szCardNum,szHeader+40,5);//�ڼ��ſ�
			iCardID = atoi(szHeader+40);
			memcpy(szChannel,szHeader+45,5);//�ڼ���ͨ��
			//memcpy(szSplitPK,szHeader+50,5);
			iSpliPK = atoi(szHeader+50);
			iPKID = atoi(szHeader + 55);
			//memcpy(szPKID,szHeader+55,5);//�ڼ�����
			memcpy(szPKLength,szHeader+60,20);//����
			int iPKLength = atoi(szPKLength);
			if(strcmp(szCharacter,"CardData")==0)
			{						
				//���հ���
				for(int n=0;n<3;n++) 
				{
					lRet = zmq_recv(pSock, szMsg, iPKLength,0);
					if(lRet == iPKLength)
					{
#ifdef DBGXU
						if(atoi(szChannel)==0)
						{
							FILE *fpt=fopen("ch1","wb");
							fwrite(szMsg,iPKLength,1,fpt);
							fclose(fpt);
						}
#endif
						break;
					}
					Sleep(100);
				}
				if(lRet == iPKLength)//������ȷ
				{
					if (iSpliPK == 1)//�ְ�
					{
						if(iPKID==0)//�ÿ���ͨ���ĵ�һ����
						{
							int iFrontEmpty = 0;
							for(int i=0;i<pPCIeDlg->m_hostcount;i++)//����ͨ���ļ�
							{
								if(pPCIeDlg->m_lanhosts[i].cardnum == 0)
								{
									iFrontEmpty++;
									continue;
								}
								else if ((strcmp(szSrcIP,pPCIeDlg->m_lanhosts[i].ip) == 0)&&(pPCIeDlg->m_lanhosts[i].cardnum > 0))
								{
									//�����ļ�
									CString strChFileName;
									strChFileName.Format("%s\\CH%d",pPCIeDlg->m_strSampleSavePath,(i-iFrontEmpty)*8+atoi(szCardNum)*8+atoi(szChannel)+1);
									fp = fopen(strChFileName,"w");
									break;
								}
							}
						}
						else
						{
							int iFrontEmpty = 0;
							for(int i=0;i<pPCIeDlg->m_hostcount;i++)
							{
								if(pPCIeDlg->m_lanhosts[i].cardnum == 0)
								{
									iFrontEmpty++;
									continue;
								}
								else if ((strcmp(szSrcIP,pPCIeDlg->m_lanhosts[i].ip) == 0)&&(pPCIeDlg->m_lanhosts[i].cardnum > 0))
								{
									//д�ļ�
									CString strChFileName;
									strChFileName.Format("%s\\CH%d",pPCIeDlg->m_strSampleSavePath,(i-iFrontEmpty)*8+atoi(szCardNum)*8+atoi(szChannel)+1);
									fp = fopen(strChFileName,"a+");
									break;
								}
							}

						}
						//fprintf(fp, "%s\n", szMsg);
						//fwrite(szMsg,iPKLength,1,fp);
						for(long j=0;j<iPKLength/2;j++)
						{
							pPCIeDlg->m_ShortData[iCardID][j]=szMsg[(j+iCardID)*2]+(szMsg[(j+iCardID)*2+1]<<8);
							fprintf(fp,"%6d\n",pPCIeDlg->m_ShortData[iCardID][j]);
						}
						fclose(fp);
									
					}
					else//���ְ����ð�����������ͨ���ļ�
					{
						int iFrontEmpty = 0;
						for(int i=0;i<pPCIeDlg->m_hostcount;i++)//����ͨ���ļ�
						{
							if(pPCIeDlg->m_lanhosts[i].cardnum == 0)
							{
								iFrontEmpty++;
								continue;
							}
							else if ((strcmp(szSrcIP,pPCIeDlg->m_lanhosts[i].ip) == 0)&&(pPCIeDlg->m_lanhosts[i].cardnum > 0))
							{
								//�����ļ�
								CString strChFileName;
								strChFileName.Format("%s\\CH%d",pPCIeDlg->m_strSampleSavePath,(i-iFrontEmpty)*8+atoi(szCardNum)*8+atoi(szChannel)+1);
								fp = fopen(strChFileName,"w");
								break;
							}
						}
						long j;
						for(j=0;j<iPKLength/2;j++)
						{
							//int tt = szMsg[(j*8+iCardID)*2+1]<<8;
							pPCIeDlg->m_ShortData[iCardID][j]=szMsg[(j+iCardID)*2]+(szMsg[(j+iCardID)*2+1]<<8);
							fprintf(fp,"%6d\n",pPCIeDlg->m_ShortData[iCardID][j]);
						}
						fclose(fp);
					}
				}
				
			}				

		}
	}
	delete szMsg;
	return 0;
}
//�����߳�
UINT HBThread(LPVOID lpara)
{
	CGrapherDlg *pPCIeDlg=(CGrapherDlg*)lpara;
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return -1;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return -2;  
	}  
	while(true)
	{
		if(pPCIeDlg->m_list1Lock)
			continue;
		//pPCIeDlg->m_status.SetWindowText("");
		//LAN�������ɼ�����������				
        pPCIeDlg->getLanHosts();	
		pPCIeDlg->m_list1.DeleteAllItems();
		int hostCount = pPCIeDlg->m_hostcount;
		for(int i =0;i<pPCIeDlg->m_hostcount;i++)
		{		
			//��ÿ��HOST����"�鿴�ɼ�������"����	
			//AfxMessageBox(pPCIeDlg->m_lanhosts[i].ip);			
			sin.sin_addr.S_un.S_addr = inet_addr(pPCIeDlg->m_lanhosts[i].ip);//IP
			int len = sizeof(sin);    
			memset(sendData,0,100);
			memcpy(sendData,"cardnum",8);
			memcpy(sendData+10,pPCIeDlg->m_szHostIP,30);
			retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);		
			//��ÿ��host����"��ѯ�ɼ���״̬"����
			memset(sendData,0,100);
			memcpy(sendData,"status",7);
			retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);
		}		
		//list2ʵʱ����		
		pPCIeDlg->InsertHisRecord();
		Sleep(1000);
	}
	return 0;
}
//�����߳�
UINT ListenThread(LPVOID lpara)
{	
	CGrapherDlg *pPCIeDlg=(CGrapherDlg*)lpara;	
	//���������
	fd_set rfd;     // ����������   
	timeval timeout;
	int retVal=0,imode=1; 	
	char chNum[10] = {0};	
	char szHeader[20]={0};					
	char szSrcIP[30]={0};
	// ���ó�ʱʱ��Ϊ20s
	timeout.tv_sec = 20;	
	timeout.tv_usec = 0;    
	char recvData[512]={0};  
	SOCKET serSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 	
    if(serSocket == INVALID_SOCKET)
    {        
        return -1;
    }
    sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(ListenPort);
    serAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	int nAddrLen = sizeof(serAddr); 
	//����Ϊ������ģʽ      
	retVal=ioctlsocket(serSocket,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  		
		closesocket(serSocket);  		
		return -2;  
	}  
    if(bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {        
        closesocket(serSocket);
        return -3;
    }   	
    while (true)
    {				
		//��ʼʹ��select
		FD_ZERO(&rfd);
		FD_SET(serSocket, &rfd); // ��serSocket����Ҫ���Ե�����������		
		retVal = select(0, &rfd, NULL, NULL, &timeout);// ����Ƿ����׽ӿ��Ƿ�ɶ�
		if (retVal == SOCKET_ERROR)	
		{
			//int err = GetLastError();			
			return -4;
		}
		else if (retVal == 0)// ��ʱ
		{									
			continue;
		}
		else// ��⵽���׽ӿڿɶ�
		{
			if (FD_ISSET(serSocket, &rfd))	// serSocket�ɶ�
			{
				memset(recvData,0,512);
				int ret = recvfrom(serSocket, recvData, 512, 0, (sockaddr *)&serAddr,&nAddrLen);
				//int err = GetLastError();
				if (ret > 0)
				{		
					memset(szHeader,0,10);
					memset(szSrcIP,0,30);
					memcpy(szHeader,recvData,10);
					memcpy(szSrcIP,recvData+10,30);//����ԴIP
					if(strcmp(szHeader,"numret")==0)//����(��ȡ�ɼ�������)�����
					{
						memset(chNum,0,10);							
						memcpy(chNum,recvData+40,4);
						int iNum = atoi(chNum);
						//��m_lanhosts����
						if(pPCIeDlg->m_hostcount==0)
						{
							strcpy(pPCIeDlg->m_lanhosts[pPCIeDlg->m_hostcount].ip,szSrcIP);
							pPCIeDlg->m_lanhosts[pPCIeDlg->m_hostcount].cardnum = iNum;
							pPCIeDlg->m_hostcount++;
						}
						else
						{
							bool IsRepeat = false;
							for(int i =0;i<pPCIeDlg->m_hostcount;i++)
							{							
								if(strcmp(pPCIeDlg->m_lanhosts[i].ip,szSrcIP)==0)//�����д��ڸ�ip	
								{
									pPCIeDlg->m_lanhosts[i].cardnum = iNum;
									IsRepeat = true;
								}
							}	
							if(!IsRepeat)//������û�и�ip
							{
								strcpy(pPCIeDlg->m_lanhosts[pPCIeDlg->m_hostcount].ip,szSrcIP);
								pPCIeDlg->m_lanhosts[pPCIeDlg->m_hostcount].cardnum = iNum;
								pPCIeDlg->m_hostcount++;
							}
						}
						//����List1						
						pPCIeDlg->m_list1.DeleteAllItems();
						for(int i=0;i<pPCIeDlg->m_hostcount;i++)
						{	
							char strIP[30]={0};
							strcpy(strIP,pPCIeDlg->m_lanhosts[i].ip);
							iNum = pPCIeDlg->m_lanhosts[i].cardnum;
							if(iNum>0)//�ɼ�����������0�Ĳ���ʾ
							{
								int iRow = pPCIeDlg->m_list1.InsertItem(i,pPCIeDlg->m_lanhosts[i].ip);	
								sprintf(chNum,"%d",pPCIeDlg->m_lanhosts[i].cardnum);
								pPCIeDlg->m_list1.SetItemText(iRow,1,chNum);	
							}
						}
					}							
					else if(strcmp(szHeader,"hisrec")==0)//��ʷ��¼�ļ�
					{		
						strcpy(pPCIeDlg->m_chFileIP,szSrcIP);
						CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RecvFileThread,pPCIeDlg,0,NULL);
						//����"�ļ�������׼����"
						char sendData[100];
						memset(sendData,0,100);
						memcpy(sendData,"revready",9);								
						memcpy(sendData+10,pPCIeDlg->m_szHostIP,30); 								
						sockaddr_in cliAddr;
						cliAddr.sin_family = AF_INET;
						cliAddr.sin_port = htons(ACKPort);
						cliAddr.sin_addr.S_un.S_addr = inet_addr(szSrcIP);
						sendto(serSocket, sendData, 100, 0, (sockaddr *)&cliAddr, nAddrLen); 					
					}								
					else if(strcmp(szHeader,"stret")==0)//�ɼ���״̬�����
					{					
						pPCIeDlg->m_status.SetWindowText(recvData+40);//һ����ʾһ��
					}
					else if(strcmp(szHeader,"hisreccop")==0)//�������������
					{
						pPCIeDlg->m_list1Lock = FALSE;	
					}
					else if (strcmp(szHeader,"statustxt")==0)
					{
						pPCIeDlg->m_status.SetWindowText(recvData+40);
					}
				}	
			}
		}        	
    }
    closesocket(serSocket); 
    return 0;	
}

BOOL CGrapherDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon	
	//����IP�ؼ�	
	strcpy(m_szHostIP,getHostIP()); 
	DWORD dwIP=ntohl(inet_addr(m_szHostIP)); 
	m_IP.SetAddress(dwIP);
	//list�ؼ���ͷ��ʼ��
	m_list1.InsertColumn(0,_T("����IP"),LVCFMT_CENTER,95);
	m_list1.InsertColumn(1,_T("�ɼ�������"),LVCFMT_CENTER,134);
	m_list1.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);

	m_list2.InsertColumn(0,_T("�ļ�������"),LVCFMT_CENTER,106);
	m_list2.InsertColumn(1,_T("ͨ����"),LVCFMT_CENTER,103);
	m_list2.InsertColumn(2,_T("��������"),LVCFMT_CENTER,103);
	m_list2.InsertColumn(3,_T("������"),LVCFMT_CENTER,103);
	m_list2.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	//��������
	GetDlgItem(IDC_LIST2)->SetFocus();
	//���鷶Χ����
	m_slider1.SetRange(0,5000);
	m_slider2.SetRange(0,5000);	
	//��Ͽ������б�
	m_combo1.InsertString(0,"62914560");
	m_combo1.InsertString(1,"41943040");
	m_combo1.InsertString(2,"10485760");
	m_combo1.InsertString(3,"2621440");
	m_combo1.InsertString(4,"655360");
	m_combo1.InsertString(5,"163840");
	m_combo1.InsertString(6,"40960");
	m_combo1.InsertString(7,"10240");
	m_combo1.SetCurSel(0);

	m_combo2.InsertString(0,"10000000");
	m_combo2.InsertString(1,"50000000");
	m_combo2.InsertString(2,"100000000");
	m_combo2.SetCurSel(0);

	m_combo3.AddString("���ϴ���");
	m_combo3.AddString("��������");	
	m_combo3.SetCurSel(0);	
	//list1	
/*	getLanHosts();
	char chNum[10]={0};
	for(int i=0;i<m_hostcount;i++)
	{					
		if(m_lanhosts[i].cardnum>0)//�ɼ�����������0�Ĳ���ʾ
		{
			memset(chNum,0,10);
			m_list1.InsertItem(i,m_lanhosts[i].ip);	
			sprintf(chNum,"%d",0);
			m_list1.SetItemText(i,1,chNum);	
		}
	}*/
	//ѡ����ʷ�ļ���	
	int ret = OnBrowseforfolder("��ѡ����ʷ��¼�ļ���:",true);	
	if(ret == 1)//ȡ������·������ȷ����û����ʷ��¼
	{
		AfxMessageBox("δѡ��·��������ʷ��¼Ϊ�ջ���·������ȷ!������ʾ��ʷ��¼");
		//return false;
	}
	//������ʷ��¼��list2	
	InsertHisRecord();
	UpdateData(false);
	//��ȡ�����ɼ�������
	//m_CardNum=WCDPcie_ScanfDevice(0x10ee,0x0007,m_BusIDArray,m_SlotIDArray);	
	//���������߳�
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ListenThread,this,0,NULL);
	//���������߳�	
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)HBThread,this,0,NULL);
	return FALSE;  // return TRUE  unless you set the focus to a control
}


void CGrapherDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		
		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{

		CDialog::OnPaint();
	}
}

HCURSOR CGrapherDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
void CGrapherDlg::InsertHisRecord()
{	
	//���list2
	m_list2.DeleteAllItems();
	if(!m_strSampleSavePath.IsEmpty())
	{
		//����setup.inf�ļ�
		CFile file;
		CString FileName;	
		FileName.Format("%s\\Setup.inf",m_strSampleSavePath);	
		if(!file.Open(FileName, CFile::modeCreate | CFile::modeWrite|CFile::typeBinary))
			return;
		CString csBuf;
		//  0.�����
		csBuf.Format("RECORDER SETTING FILE\r\n");
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	1.ͨ����
		long lListItemCount = m_list1.GetItemCount();
		csBuf.Format("%d\r\n",lListItemCount*8);
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	2.����		������ Ĭ��10��
		csBuf.Format("10\r\n");
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	3.��������
		int nIndex  = m_combo1.GetCurSel();			
		m_combo1.GetLBText(nIndex,csBuf);
		csBuf += "\r\n";
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	4.��������
		nIndex  = m_combo2.GetCurSel();			
		m_combo2.GetLBText(nIndex,csBuf);
		csBuf += "\r\n";
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	5.������ʽ��1���������� 4��ͨ��������
		nIndex  = m_combo3.GetCurSel();
		csBuf.Format("%d\r\n",nIndex);
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//pPCIeDlg->m_combo3.GetLBText(nIndex,csBuf);
		//	6.����ͨ�������� Ĭ��0��
		csBuf.Format("0\r\n");
		file.Write(csBuf,csBuf.GetLength());		
		//	7.������ƽ������ Ĭ��0��
		//csBuf.Format("0\n");
		file.Write(csBuf,csBuf.GetLength());		
		//	8.������ʱ������ Ĭ��0��
		//csBuf.Format("0\n");
		file.Write(csBuf,csBuf.GetLength());
		file.Close();
	}
	//��ȡ��ʷ��¼����ʾ��list2�ؼ�		 
	 if(!m_strHisFilePath.IsEmpty())
	 {			 
		 int filecount = 0;
		 int i=0;
		 //��ȡ���ļ��������е���ʷ�ļ���
		 getHisFile(m_strHisFilePath,m_FileList,filecount);		 
		 if(!m_FileList->empty())
		 {
			 //int size = m_FileList->size();
			 vector<CString>::iterator it;
			 for(it=m_FileList->begin();it!=m_FileList->end();it++)
			 {
				//��ȡ�ļ���
				int nCount = (*it).ReverseFind('\\');
				CString FolderName = (*it).Mid(nCount+1);
				m_list2.InsertItem(i,FolderName);
				//��ȡ�ļ�ָ���е�����
				FolderName = (*it) + "\\Setup.inf";
				char* tds = ReadSpeacialLine(FolderName.GetBuffer(256),2);
				char* cycd = ReadSpeacialLine(FolderName.GetBuffer(256),4);
				char* cyl = ReadSpeacialLine(FolderName.GetBuffer(256),5);
				FolderName.ReleaseBuffer();
				m_list2.SetItemText(i,1,tds);
				m_list2.SetItemText(i,2,cycd);
				m_list2.SetItemText(i,3,cyl);
				i++;
			 }
		 }		 		 			
	}	
}
void CGrapherDlg::getPathFromCfg(bool IsOpenHis)
{
	//��ȡ�����ļ�·�� 	
	CString CfgPath;
	GetModuleFileName(NULL,CfgPath.GetBuffer(MAX_PATH),MAX_PATH);
	CfgPath.ReleaseBuffer();
	int nPos  = CfgPath.ReverseFind('\\');
	CfgPath = CfgPath.Left(nPos);		
	CfgPath += "\\Grapher.ini";
	//��ȡ�����ļ�����
	FILE *fp = fopen(CfgPath,"a+");	
	if(fp)
	{	
		if(IsOpenHis)
		{
			int err = GetPrivateProfileString("Configure","HisPath","",m_strHisFilePath.GetBuffer(MAX_PATH),MAX_PATH,CfgPath);
			m_strHisFilePath.ReleaseBuffer();			
			if(m_strHisFilePath.IsEmpty())//��������ļ�Ϊ��
			{
				GetModuleFileName(NULL,m_strHisFilePath.GetBuffer(MAX_PATH),MAX_PATH);
				m_strHisFilePath.ReleaseBuffer();
				int nPos  = m_strHisFilePath.ReverseFind('\\');
				m_strHisFilePath = m_strHisFilePath.Left(nPos);	
			}
		}
		else
		{
			int err = GetPrivateProfileString("Configure","SavePath","",m_strSampleSavePath.GetBuffer(MAX_PATH),MAX_PATH,CfgPath);
			m_strSampleSavePath.ReleaseBuffer();			
			if(m_strSampleSavePath.IsEmpty())//��������ļ�Ϊ��
			{
				GetModuleFileName(NULL,m_strSampleSavePath.GetBuffer(MAX_PATH),MAX_PATH);
				m_strSampleSavePath.ReleaseBuffer();
				int nPos  = m_strSampleSavePath.ReverseFind('\\');
				m_strSampleSavePath = m_strSampleSavePath.Left(nPos);	
			}
		}
	}
	fclose(fp);
}
char* CGrapherDlg::getHostIP()  
{   
    char szHost[MAX_PATH]={0};  
    gethostname(szHost,MAX_PATH);  
    hostent *pHost=gethostbyname(szHost);  
    in_addr addr;  
    char *p=pHost->h_addr_list[0];  
    memcpy(&addr.S_un.S_addr,p,pHost->h_length);  
    char *szIp=inet_ntoa(addr);     
    return szIp;    
}  

int CGrapherDlg::getLanHosts()
{       	
	struct hostent *host;
	struct in_addr *ptr;    // ���IP��ַ     
	DWORD dwScope = RESOURCE_CONTEXT;
	NETRESOURCE *NetResource = NULL;
	HANDLE hEnum;
	WNetOpenEnum( dwScope,NULL,NULL,NULL,&hEnum);	
	//WSADATA   wsaData;
	//WSAStartup(MAKEWORD(1, 1), &wsaData);
	//��ʼö��������Դ	
	if ( hEnum )     //��������Ч
	{
		DWORD Count = 0xFFFFFFFF;
		DWORD BufferSize = 2048;
		LPVOID Buffer = new char[2048];
		//memset(Buffer,0,2048);
		// ����WSAStartup�����WNetEnumResource����һ����ö�ٹ���
		WNetEnumResource(hEnum,&Count,Buffer,&BufferSize);
		//AfxMessageBox("1");
		NetResource = (NETRESOURCE*)Buffer;
		char szHostName[lanHostsNum]={0};
		m_hostcount = 0;
		for ( unsigned int i = 0; i < BufferSize/sizeof(NETRESOURCE); i++, NetResource++ )        
		{            
			if ( NetResource->dwUsage == RESOURCEUSAGE_CONTAINER && NetResource->dwType == RESOURCETYPE_ANY )                
			{
				//AfxMessageBox("2");
				if (NetResource->lpRemoteName)
				{         
					//AfxMessageBox("3");
					char* strFullName = NetResource->lpRemoteName;
					size_t namelen = strlen(strFullName)+1;
					size_t converted = 0;				
					//ȥ��ͷ����"//"    
					char* pstr = (char*)malloc((namelen)*sizeof(char));
					if ((strFullName[0]=='\\')&&(strFullName[1]=='\\'))                                                
					{
						strcpy(pstr,strFullName+2);
					}
					//���������
					gethostname( szHostName, strlen( szHostName ) );
					//����������ø�����Ӧ��������Ϣ
					host = gethostbyname(pstr);
					if(host == NULL) continue; 
					ptr = (struct in_addr *) host->h_addr_list[0];                    
					// ��ȡIP��ַ��Ϣ����ַ��ʽ���£� 211.40.35.76                 
					int a = ptr->S_un.S_un_b.s_b1;  // 211                
					int b = ptr->S_un.S_un_b.s_b2;  // 40
					int c = ptr->S_un.S_un_b.s_b3;  // 35
					int d = ptr->S_un.S_un_b.s_b4;  // 76					
					sprintf(m_lanhosts[m_hostcount].name,"%s",pstr);                    
					sprintf(m_lanhosts[m_hostcount].ip,"%d.%d.%d.%d",a,b,c,d);						
					m_lanhosts[m_hostcount].cardnum = 0;
					free(pstr);
					m_hostcount++;
				}
			}
		}
		delete Buffer;
		// ����ö�ٹ���
		WNetCloseEnum( hEnum );
	}		
/*	sprintf(m_lanhosts[0].name,"%s","123");                    
	sprintf(m_lanhosts[0].ip,"%d.%d.%d.%d",192,168,1,102);	
	sprintf(m_lanhosts[1].name,"%s","234");                    
	sprintf(m_lanhosts[1].ip,"%d.%d.%d.%d",192,168,1,68);
	sprintf(m_lanhosts[2].name,"%s","567");                    
	sprintf(m_lanhosts[2].ip,"%d.%d.%d.%d",192,168,2,33);
	m_hostcount = 3;	*/
    //unsigned int x=ntohl(inet_addr(ip.c_str()));    
	//�Խṹ�����鰴��IP��ַ��������
	if(m_hostcount>0)
	{		
		int min;
		for(int i=0;i<m_hostcount-1;i++)
		{
			min = i;			
			for(int j=i+1;j<m_hostcount;j++)
			{
				if(ntohl(inet_addr(m_lanhosts[min].ip)) > ntohl(inet_addr(m_lanhosts[j].ip)))
				{
					min = j;
				}
			}
			if(min != i)
			{
				
				swap(m_lanhosts[min],m_lanhosts[i]);			
			}
		
		}
	}	
	return 0;
}
//��ȡ�ļ�������ʷ��¼�ļ���
void CGrapherDlg::getHisFile(CString path,vector<CString> *m_FileList, int& count)
{	
	CFileFind finder;
	BOOL working = finder.FindFile(path + "\\*.*");	
	m_FileList->clear();	
	while (working)
	{		
		working = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (finder.IsDirectory())
		{
			//getHisFile(finder.GetFilePath(), filenames, count);
			CString filename = finder.GetFileName();
			//����
			//CString temp = filename.Left(2);
			//if(temp.CompareNoCase("Ch") == 0)
			m_FileList->push_back(path+"\\"+filename);		
		} 
		else 
		{
		/*	CString filename = finder.GetFileName();
			//����
			CString temp = filename.Left(2);
			if(temp.CompareNoCase("Ch") == 0)
				m_FileList->push_back(path+"\\"+filename);
			//filenames[count++] = filename;*/
		}
	}	
	//����
	if(!m_FileList->empty())
	{		
		int min;
		for(int i=0;i<m_FileList->size()-1;i++)
		{				
			CString pfolderName = m_FileList->at(i);		
			int nCount = m_FileList->at(i).ReverseFind('\\');		
			pfolderName = pfolderName.Mid(nCount+1);		
			min = i;	
			for(int j=i+1;j < m_FileList->size();j++)
			{			
				pfolderName = m_FileList->at(min);
				nCount = m_FileList->at(min).ReverseFind('\\');
				pfolderName = pfolderName.Mid(nCount+1);

				CString qfolderName = m_FileList->at(j);
				nCount = m_FileList->at(j).ReverseFind('\\');
				qfolderName = qfolderName.Mid(nCount+1);			
				if(atoi(pfolderName.GetBuffer(256)) > atoi(qfolderName.GetBuffer(256)))
				{				
					pfolderName.ReleaseBuffer();
					qfolderName.ReleaseBuffer();
					min = j;				
				}
			}
			if(min != i)
			{				
				swap(m_FileList->at(i),m_FileList->at(min));
			}
		}
	}
}

char* CGrapherDlg::ReadSpeacialLine(char* filename, int whichLine)
{
	if (whichLine < 0 || NULL == filename)
	{
		return NULL;
	}
	FILE *fp = fopen(filename, "r");
	if (NULL == fp) 
	{
		return NULL;
	}
	int reachWhichLine = 0;
	int curLine = 1;	
	char *data = NULL;
	data = (char*) malloc(LINE_SIZE);
	char* dst = (char*) malloc(LINE_SIZE);
	while (!feof(fp))//�ļ�δ����
	{
		memset(data, 0, LINE_SIZE);
		fgets(data, LINE_SIZE - 1, fp);		
		curLine++;
		//ȥ���ո�
		int i = -1, j = 0;    
		while (data[++i])   
		{
			if (data[i] != ' ')            
				dst[j++] = data[i];    
		}
		dst[j] = '\0';
		if (curLine > whichLine)
		{
			reachWhichLine = 1; //�Ѿ���ȡ��whichLine��
			break;
		}
	}
	fclose(fp);
	return (0 == reachWhichLine ? NULL : dst);
}

int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
      switch (uMsg)
      {
      case BFFM_INITIALIZED:
           {
                 //BFFM_INITIALIZED��ʾ����Ի����Ѿ���������������lParamΪNULL
                 //���ó�ʼѡ��
                 ::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,lpData);
                 //����BFFM_SETSELECTION��Ϣ��˵��
                 //wParam �����lParam��������һ��ITEMIDLIST�ṹ(PIDL)����һ��Ŀ¼·����
                 //          ���ΪTRUE��lParam����Ϊ·����������lParam����һ��·��PIDL��
                 //lParam ������Ϊ����Ի�����ѡ��·�������wParamΪTRUE��lParam����Ϊһ��
                 //         ��NULL��β���ַ�����ָ�룬����ΪPIDL
                 break;
           }

      case BFFM_SELCHANGED:
           {
                 //BFFM_SELCHANGED��ʾѡ�����Ѿ������仯������lParam�����б�������ѡ�������ĿID               
                 ITEMIDLIST * pidl; 
                 char path[MAX_PATH]; 
                 //������ĿIDȡ·����Ϣ
                 pidl = (ITEMIDLIST*) lParam;
                 if (SHGetPathFromIDList(pidl, path))
                 {
                      //ʹ�á�ȷ�ϡ���ť��Ч
                      //����BFFM_ENABLEOK��Ϣ��˵��
                      //wParam �������壬������Ϊ0
                      //lParam �����Ϊ��0����ʹ��ȷ�ϰ�ť������ʧЧ��ȷ�ϡ���ť
                      ::SendMessage(hwnd,BFFM_ENABLEOK,0,TRUE);
                      //������
                      DWORD attributes = ::GetFileAttributes(path);
                      //����״̬����ʾ��ǰ��ѡ���ȫ·���������ļ�����
                      //����BFFM_SETSTATUSTEXT��Ϣ��˵��
                      //wParam �������壬������Ϊ0
                      //lParam ��ָ��һ���ں�״̬����ʾ��Ϣ���ַ���
                      CString strText;
                      strText.Format("%s%s%s%s",
                            path,
                            attributes & FILE_ATTRIBUTE_HIDDEN ? ",H":"",
                            attributes & FILE_ATTRIBUTE_READONLY ? ",R":"",
                            attributes & FILE_ATTRIBUTE_SYSTEM ? ",S":""
                            );
                      ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)(LPTSTR)(LPCTSTR)strText);
                 }
                 else
                 {
                      //ʹ�á�ȷ�ϡ���ťʧЧ
                      ::SendMessage(hwnd,BFFM_ENABLEOK,0,FALSE);
                      //��״̬����Ϣ
                      ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)(LPTSTR)(LPCTSTR)"");
                 }
                 break;			   
           }
      case BFFM_VALIDATEFAILED:
           {
                 //BFFM_VALIDATEFAILED��ʾ�û�������Ի���ı༭����������һ���Ƿ�����
                 //����Ϣ���û�����ȷ�ϡ�ʱ�ͳ�������Ȼǰ���Ǳ༭������������ƷǷ�
                 //lParam���������˷Ƿ��������ݵĵ�ַ��Ӧ�ó������ʹ�������Ϣ��ʾ�û�����Ƿ���
                 //���⣬����Ϣ�Ļص���������0��ʾĿ¼����Ի��������رգ���������ֵ������Ի��������ʾ��
                 //����Ŀ¼����Ի����к��б༭����������BIF_VALIDATE��ǲſ��ܳ��ִ���Ϣ
                 //��BROWSEINFO�ṹ��ulFlags����BIF_EDITBOX|BIF_VALIDATE��־
                 CString strTip;
                 strTip.Format("Ŀ¼%s�Ƿ�!",lParam);
                 //����0����Ի�����ǰ�رգ�SHBrowseForFolder()����NULL
                 //AfxMessageBox(strTip);
                 //return 0;
                 //����1�Ի��������ʾ����Ϊ�Ի����Լ�����ʾ��������״̬����ʾ������Ϣ
                 //ע�⣺�����ʱ����AfxMessageBox����ʾ��ʾ��Ϣ����ʾ��Ϣ��رպ�Ҫʹ�����ط�Ŀ¼
                 //����Ի�����Ҫ�ͻ��ֹ��ƶ���꼤��öԻ�����У�������ʹ�ú�̲������Ǻܷ��㣬������״̬����ʾ��ʾ��Ϣ�ȽϺ�
                 //::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)(LPTSTR)(LPCTSTR)strTip);
                 //return 1;
                 break;
           }
      //default:
          //break;
      }
      return 0;
}

int CGrapherDlg::OnBrowseforfolder(CString dlgTitle,bool IsOpenHis) 
{	  
	  getPathFromCfg(IsOpenHis);
	  UpdateData(TRUE);
      BROWSEINFO bi={0}; 
      char dispname[MAX_PATH], path[MAX_PATH]; 
	  memset(dispname,0,MAX_PATH);
	  memset(path,0,MAX_PATH);
      ITEMIDLIST * pidl; 
      //����Ŀ¼����Ի��������
      bi.hwndOwner = NULL;
      //ȷ�������Χ����Ŀ¼��
      //ֻ�и�Ŀ¼������Ŀ¼�¿������
      //����ΪNULL��ʾ���ֿռ��½Կ���
      bi.pidlRoot = NULL;
      //���ڽ����û���ѡĿ¼����ʾ��
      //�����ԣ����������ȫ·����
      bi.pszDisplayName = dispname; 
      //����Ŀ¼����Ի���ĶԻ������
      bi.lpszTitle = dlgTitle;//"��ѡ��·��:"; 
      //����״̬
      //BIF_BROWSEINCLUDEFILES
      //BIF_RETURNONLYFSDIRS��ʾֻ����Ŀ¼��
      //BIF_STATUSTEXT��ʾ�Ի�������״̬��
      //BIF_EDITBOX��ʾ�Ի������б༭��
      //BIF_VALIDATE��ʾ�ͻ�����ȷ�ϡ���ťʱ���༭�����ݵĺϷ���
	  
      bi.ulFlags = BIF_NEWDIALOGSTYLE|BIF_BROWSEINCLUDEFILES|BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT|BIF_EDITBOX|BIF_VALIDATE;
      //���ûص�����
      //�����Ҫ���ó�ʼѡ�����ʾ��ѡ��������Ϣ����ϵͳ�Զ�У���û�����
      //�ĺϷ��ԣ���ôӦ��ʹ�ûص�����������ɽ���������ΪNULL
      bi.lpfn = BrowseCallbackProc; 
      //���ûص�������lParam����
      //�˴���������Ŀ¼����Ի���ĳ�ʼѡ��
	  if(IsOpenHis)
		bi.lParam = (LPARAM)(LPCTSTR)m_strHisFilePath; 
	  else
		bi.lParam = (LPARAM)(LPCTSTR)m_strSampleSavePath; 
      //����������ѡĿ¼��ͼ�꣨ϵͳͼ���б��е���ţ�
      bi.iImage = NULL; 
      //��ʾĿ¼����Ի���
      if (pidl = SHBrowseForFolder(&bi))
      { 
           //��PIDLת��Ϊ�ַ���
           if (SHGetPathFromIDList(pidl, path))
           {
                 //���¶Ի�����ʾ������ʾ�û�������ѡ��
				 if(IsOpenHis)
					m_strHisFilePath = path; 
				 else
					m_strSampleSavePath = path;
                 UpdateData(FALSE);
				 //��ȡ�����ļ�·�� 
				 CString CfgPath;
				 GetModuleFileName(NULL,CfgPath.GetBuffer(MAX_PATH),MAX_PATH);
				 CfgPath.ReleaseBuffer();
				 int nPos  = CfgPath.ReverseFind('\\');
				 CfgPath = CfgPath.Left(nPos);		
				 CfgPath += "\\Grapher.ini";
				 FILE *fp = fopen(CfgPath,"a+");
				 if(fp)
				 {
				    //������ʷ��¼·���������ļ�
					if(IsOpenHis)
						WritePrivateProfileString("Configure","HisPath",m_strHisFilePath,CfgPath);	
					else
						WritePrivateProfileString("Configure","SavePath",m_strSampleSavePath,CfgPath);
				 }
				 fclose(fp);
           }
      }
	  else
	  {
		  //AfxMessageBox("δѡ����ʷ��¼·��!������ʾ��ʷ��¼");
		  if(IsOpenHis)
			m_strHisFilePath = "";
		  else
			m_strSampleSavePath = "";
		  return 1;
	  }
	return 0;
}

void CGrapherDlg::OnStartsample() 
{			
	//m_status.SetWindowText("");
	//ѡ�񱣴��ļ��к�������
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		AfxMessageBox("��ǰ���������޲ɼ���!�����ɼ�!");
		return;
	}
	ret = OnBrowseforfolder("��ѡ��ɼ��ɹ���¼����·��:",false);
	if(ret == 1)
	{
		AfxMessageBox("δѡ��ɼ���¼����·��!�޷���ʼ�ɼ�!");
		return;
	}	
	//���������ļ�Setup.inf
	InsertHisRecord();

	OnEditchangeCombo2();
	Sleep(5);
	OnEditchangeCombo3();
	Sleep(5);
	OnEditchangeCombo4();
	Sleep(5);
	CustomdrawSlider1();
	Sleep(5);
	CustomdrawSlider2();
	Sleep(10);
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{		
		if(m_lanhosts[i].cardnum > 0)
		{
			sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
			int len = sizeof(sin);    
		//	memset(sendData,0,100);
			memcpy(sendData,"start",6);
			memcpy(sendData+10,m_szHostIP,30);
			retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);		
		}
	}		
}

void CGrapherDlg::OnStopsample() 
{
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		AfxMessageBox("��ǰ���������޲ɼ���!����ָ��ʧ��!");
		return;
	}
	//����
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{		
		//��ÿ��HOST����"ֹͣ�ɼ�"����	
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
	//	memset(sendData,0,100);
		memcpy(sendData,"stop",6);
		memcpy(sendData+10,m_szHostIP,30);
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}		
}

void CGrapherDlg::OnCustomdrawSlider1(NMHDR* pNMHDR, LRESULT* pResult) 
{	
	float pos = m_slider1.GetPos();
	//m_slider1.SetPos(pos);
	if(pos<m_slider2.GetPos())
	{
		m_slider2.SetPos(pos);
		//UpdateData(false);
		//m_slider1.Invalidate();
		//pos = m_slider1.GetPos();
	}
	CString cstxt;
	cstxt.Format("%.3lf",pos/500-5);
	m_sld1txt.SetWindowText(cstxt);
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		return;
	}
	*pResult = 0;
	//����
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{			
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);
		memcpy(sendData,"settul",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,cstxt.GetBuffer(10),10);
		cstxt.ReleaseBuffer();
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}		
}
void CGrapherDlg::CustomdrawSlider1() 
{		
	float pos = m_slider1.GetPos();
	if(pos<m_slider2.GetPos())
	{
		m_slider2.SetPos(pos);
	}
	CString cstxt;
	cstxt.Format("%.3lf",pos);
	m_sld1txt.SetWindowText(cstxt);
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		return;
	}
	//����
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{			
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
	//	memset(sendData,0,100);
		memcpy(sendData,"settul",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,cstxt.GetBuffer(10),10);
		cstxt.ReleaseBuffer();
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}		
}


void CGrapherDlg::OnCustomdrawSlider2(NMHDR* pNMHDR, LRESULT* pResult) 
{	
	// TODO: Add your control notification handler code here
	float pos = m_slider2.GetPos();
	if(m_slider1.GetPos() < pos)
	{
		m_slider1.SetPos(m_slider2.GetPos());
		//pos = m_slider2.GetPos();
	}
	CString cstxt;
	cstxt.Format("%.3lf",pos/500-5);
	m_sld2txt.SetWindowText(cstxt);
	*pResult = 0;
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		//AfxMessageBox("��ǰ���������޲ɼ���!�����ɼ�!");
		return;
	}

	//����
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{		
		//��ÿ��HOST����"�鿴�ɼ�������"����	
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
	//	memset(sendData,0,100);
		memcpy(sendData,"settll",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,cstxt.GetBuffer(10),10);
		cstxt.ReleaseBuffer();
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}		
}
void CGrapherDlg::CustomdrawSlider2() 
{
	// TODO: Add your control notification handler code here	
	float pos = m_slider2.GetPos();
	if(m_slider1.GetPos() < pos)
	{
		m_slider1.SetPos(m_slider2.GetPos());
		//pos = m_slider2.GetPos();
	}
	CString cstxt;
	cstxt.Format("%.3lf",pos);
	m_sld2txt.SetWindowText(cstxt);
//	*pResult = 0;
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		//AfxMessageBox("��ǰ���������޲ɼ���!�����ɼ�!");
		return;
	}
	//����
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        //printf("socket error !");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{			
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
		memcpy(sendData,"settll",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,cstxt.GetBuffer(10),10);
		cstxt.ReleaseBuffer();
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}		
}


void CGrapherDlg::OnDblclkList2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	if(!m_FileList->empty())
	{
		int sel = m_list2.GetNextItem(-1,LVIS_SELECTED); 
		CString temp;
		temp.Format("%s",m_FileList->at(sel));	
		ShellExecute(NULL,_T("open"),temp,NULL,NULL,SW_SHOWNORMAL);
		//WinExec(temp,SW_SHOW);		
	}
	*pResult = 0;
}

void CGrapherDlg::OnOpenRecord() 
{
	if(!m_FileList->empty())
	{
		int sel = m_list2.GetNextItem(-1,LVIS_SELECTED); 
		CString temp;
		temp.Format("%s",m_FileList->at(sel));			
		ShellExecute(NULL,_T("open"),temp,NULL,NULL,SW_SHOWNORMAL);
		//WinExec(temp,SW_SHOWNORMAL);		
	}
	else
	{
		AfxMessageBox("��ʷ��¼Ϊ��!");
	}
}

void CGrapherDlg::OnDeleteRecord() 
{
	//ɾ��һ���ļ�
	if(!m_FileList->empty())
	{
		int sel = m_list2.GetNextItem(-1,LVIS_SELECTED); 
		if(sel>0)
		{
			CString temp;
			temp.Format("%s",m_FileList->at(sel));
			int ret = RemoveDirectory(temp.GetBuffer(256));
			temp.ReleaseBuffer();
			//ShellExecute(NULL,_T("open"),temp,NULL,NULL,SW_SHOWNORMAL);
			//WinExec(temp,SW_SHOW);	
			if(ret == 0)
			{
				ret = GetLastError();
				temp.Format("ɾ��ʧ��!������:%d",ret);
				AfxMessageBox(temp);		
			}
			//����list2		
			InsertHisRecord();
			UpdateData(false);
		}
		else
		{
			AfxMessageBox("��ѡ��һ����¼!");
		}
	}
	else
	{
		AfxMessageBox("��ʷ��¼Ϊ��!");
	}
}

void CGrapherDlg::OnEditchangeCombo2() 
{
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		//AfxMessageBox("��ǰ���������޲ɼ���!������Ч!");
		return;
	}
	int nIndex  = m_combo1.GetCurSel();	
	CString csValue;
	m_combo1.GetLBText(nIndex,csValue);
	//����
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        AfxMessageBox("socket error! @ set sl");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{			
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
	//	memset(sendData,0,100);
		memcpy(sendData,"setsl",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,csValue,10);
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}
}

void CGrapherDlg::OnEditchangeCombo3() 
{
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		//AfxMessageBox("��ǰ���������޲ɼ���!�����ɼ�!");
		return;
	}
	int nIndex  = m_combo2.GetCurSel();	
	CString csValue;
	m_combo2.GetLBText(nIndex,csValue);
	//����	
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        AfxMessageBox("socket error! @ set sf");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{		
		//��ÿ��HOST����"�鿴�ɼ�������"����	
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
	//	memset(sendData,0,100);
		memcpy(sendData,"setsf",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,csValue,10);
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}			
}

void CGrapherDlg::OnEditchangeCombo4() 
{
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		//AfxMessageBox("��ǰ���������޲ɼ���!�����ɼ�!");
		return;
	}
	int nIndex  = m_combo3.GetCurSel();	
	CString csValue;
	//m_combo3.GetLBText(nIndex,csValue);
	csValue.Format("%d",nIndex);
	//����	
	int retVal=0,imode=1; 
	SOCKET sclient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  
	if(sclient == INVALID_SOCKET)
    {
        AfxMessageBox("socket error! @ set tt");
        return;
    }	
	char sendData[100]={0};
	sockaddr_in sin;
	sin.sin_family = AF_INET;  
	sin.sin_port = htons(ACKPort);//PORT		
	//����Ϊ������ģʽ  
	retVal=ioctlsocket(sclient,FIONBIO,(u_long *)&imode);  
	if(retVal == SOCKET_ERROR)  
	{  
		//printf("ioctlsocket failed!");  
		closesocket(sclient);  
		//WSACleanup();  
		return;  
	}  
	for(int i =0;i<m_hostcount;i++)
	{		 	
		sin.sin_addr.S_un.S_addr = inet_addr(m_lanhosts[i].ip);//IP
		int len = sizeof(sin);    
	//	memset(sendData,0,100);
		memcpy(sendData,"settt",6);
		memcpy(sendData+10,m_szHostIP,30);
		memcpy(sendData+40,csValue,10);
		retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);			
	}			
}
