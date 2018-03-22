// GrapherServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GrapherServer.h"
#include "GrapherServerDlg.h"

#include "../dev.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif





/////////////////////////////////////////////////////////////////////////////
// CGrapherServerDlg dialog

CGrapherServerDlg::CGrapherServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGrapherServerDlg::IDD, pParent)
{
	
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//m_SendBuf = new char[N*Mega+50];
	//��ʼ����Ա����
	m_bRecvReady = FALSE;
	m_sucnum = 0;
	m_flag = 0;
	memset(m_szHostIP,0,30);
	memcpy(m_szHostIP,getHostIP(),30);
	m_saved = false;
	long i;
	m_DeviceNum=0;

	m_SampleCHNum=8;
	m_Data=new unsigned char[Mega*N];

	for(i=0;i<8;i++)
	{
		m_DataN[i]=new unsigned char[Mega*N];
	}

	m_DoubleData=new float*[m_SampleCHNum*8];
	for(i=0;i<m_SampleCHNum*8;i++)
	{
		m_DoubleData[i]=new float[Mega*N/m_SampleCHNum/2];
	}
	//m_ShortData
	m_ShortData=new unsigned short*[m_SampleCHNum*8];
	for(i=0;i<m_SampleCHNum*8;i++)
	{
		m_ShortData[i]=new unsigned short[Mega*N/m_SampleCHNum/2];
	}
	m_SelDevice=0;
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
	
	LOG("started");
}

CGrapherServerDlg::~CGrapherServerDlg()
{
	WSACleanup( );
	//delete m_SendBuf;
	long i;
	for(i=0;i<m_SampleCHNum*8;i++)
	{
		delete []m_DoubleData[i];
	}
	delete []m_DoubleData;
	for(i=0;i<m_SampleCHNum*8;i++)
	{
		delete []m_ShortData[i];
	}
	delete []m_ShortData;
	delete []m_Data;
	
	for(i=0;i<8;i++)
	{
		delete []m_DataN[i];
	}
}

void CGrapherServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGrapherServerDlg)
	DDX_Control(pDX, IDC_BUTTON1, m_start);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGrapherServerDlg, CDialog)
	//{{AFX_MSG_MAP(CGrapherServerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnStart)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGrapherServerDlg message handlers

BOOL CGrapherServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	m_bIsRuning=false;
	// TODO: Add extra initialization here
	m_DeviceNum = WCDPcie_ScanfDevice(0x10ee,0x0007,m_BusIDArray,m_SlotIDArray);	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGrapherServerDlg::OnPaint() 
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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGrapherServerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CGrapherServerDlg::OnPreSample() 
{	
	LOG("starting presample");
	CString temp;
	long HaveDevice;
	unsigned long TriggerType = m_tt;//0�������� 15���ϴ���
	unsigned long PreFifoLen;
	unsigned long PreTriggerLen=0;
	unsigned long TriggerVoltMax = m_tul*10000;//��������
	TriggerVoltMax = (int)((m_tul+5)/10.f*65535);//��������
	unsigned long TriggerVoltMin = m_tll*10000;//��������
	TriggerVoltMin = (int)((m_tll+5)/10.f*65535);//��������
	unsigned long ContinueLen=2000;
	unsigned long ContinueStep=2000;
	unsigned long ContinueTimes=40000;
	unsigned long m_CHSampleLength = m_sl;//��������
	/*//0�������� 15���ϴ���
	if(m_combo3.GetCurSel()==0)
		TriggerType = 0;
	else
		TriggerType = 15;
	//��������
	m_combo1.GetWindowText(temp);
	_stscanf(temp, _T("%ld"), &m_CHSampleLength);
	//������
	TriggerVoltMax = m_slider1.GetPos()*10000;
	TriggerVoltMin = m_slider2.GetPos()*10000;*/
	if(TriggerVoltMax < TriggerVoltMin)
	{
		//m_status.SetWindowText("�����������ô���!����������");
		return;
	}
	else if((TriggerVoltMax == 0) && (TriggerVoltMin == 0))
	{
		//m_status.SetWindowText("δ���ô���������,��ʹ��Ĭ������!");
		TriggerVoltMax = 50000;
		TriggerVoltMin = 10000;
	}
	////////////////////////////////
	long i;
	long j;
	unsigned long C0DDRSize;
	unsigned long C1DDRSize;
	unsigned short Data1;
	HaveDevice=	WCDPcie_OpenAllDevice(0x10ee,0x0007);
	if(HaveDevice==0)
	{
		return;
	}

	//HaveDevice=	WCDPcie_OpenDevice(0x10ee,0x0007,m_BusIDArray[m_SelDevice],m_SlotIDArray[m_SelDevice]);
	for( i=0;i<m_DeviceNum;i++)
	{
	//	WCDPcie_OpenDevice(0x10ee,0x0007,m_BusIDArray[i],m_SlotIDArray[i]);
		Sleep(3);
		WCDPcie_StopAD(i);
		//��������
		/*
		int nIndex  = m_combo2.GetCurSel();
		if(nIndex == 0)
			WCDPcie_SetCardSampleFrequent(i,10);//100000000
		else if(nIndex == 1)
			WCDPcie_SetCardSampleFrequent(i,50);
		else if(nIndex == 2)
			WCDPcie_SetCardSampleFrequent(i,100);*/
		WCDPcie_SetCardSampleFrequent(i,m_sf);
		WCDPcie_SetADParamter(i,TriggerType,PreTriggerLen,TriggerVoltMax,TriggerVoltMin,0xFF,ContinueLen,ContinueStep,ContinueTimes);
		WCDPcie_SetADSampleLen(i,m_CHSampleLength);
		WCDPcie_SetC0C1Ctl(i,0,0,1);
		WCDPcie_Reset_AD_DDR(i);	
		Sleep(10);
		WCDPcie_SetC0C1Ctl(i,1,1,1);
		//////////////////////////////////////////////////////
		WCDPcie_InitReadMem(i,0,2,1024*1024,0,NULL,NULL);	
	}
	LOG("presample ended");
}

UINT SendFileThread(LPVOID pParam)
{
	CGrapherServerDlg* pDlg = (CGrapherServerDlg*)pParam;	
	char szHeader[80]={0};
	char *SendBuf=new char[N*Mega];
	//char *correctSendBuf=new char[N*Mega];
	void * pCtx = NULL;
    void * pSock = NULL;	
	memcpy(szHeader,"CardData",10);
	memcpy(szHeader+10,pDlg->m_szHostIP,30);
    //ʹ��tcpЭ�����ͨ��,ͨ��ʹ�õ�����˿�Ϊ7766 
    char pAddr[256];
	sprintf(pAddr,"tcp://%s:7766",pDlg->m_startSrcIP);	
    //����context 
    if((pCtx = zmq_ctx_new()) == NULL)
    {
        return 0;
    }
    //����socket 
    if((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
    {
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    int iSndTimeout = 5000;// millsecond
    //���ý��ճ�ʱ 
    if(zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iSndTimeout, sizeof(iSndTimeout)) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    //����Ŀ��IP���˿�7766 
    if(zmq_connect(pSock, pAddr) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    } 
	long lRet;
	long lTotleLength = pDlg->m_sl* 2 * pDlg->m_SampleCHNum;
	char chFileName[MAX_PATH]={0};
	for (int i=0;i<pDlg->m_DeviceNum;i++)
	{	
		//AfxMessageBox("11111");
		sprintf(szHeader+40,"%d",i);
		memset(chFileName,0,MAX_PATH);
		sprintf(chFileName,".//Card%dData",i+1);
		FILE *fp = fopen(chFileName,"rb+");
		if (!fp)
		{
			return -1;
		}
		long lSent = lTotleLength/pDlg->m_SampleCHNum;//һ��ͨ�������ݴ�С		
		for (int j=0;j<pDlg->m_SampleCHNum;j++)//ÿ�η���һ��ͨ��������
		{	
			sprintf(szHeader+45,"%d",j);
			if (lSent < N*Mega)//ÿ��ͨ��������С��10MB��ֱ�ӷ�
			{		
				//�ȷ���ͷ
				sprintf(szHeader+50,"%d",0);//���ְ�
				sprintf(szHeader+60,"%ld",lSent);
				for (int m=0;m<3;m++)
				{
					lRet = zmq_send(pSock, szHeader, 80, 0);
					if( lRet == 80)
					{
						//�ٷ������ļ�����												
						fread(SendBuf,lSent,1,fp);//�������(ȷʵ��(����)������-------------------------
						//----
#ifdef DBGXU
						if(j==0)
						{
							FILE *fpt1=fopen("ch1sent","wb");
							fwrite(SendBuf,lSent,1,fpt1);
							fclose(fpt1);
						}
#endif
						//----
						lRet = zmq_send(pSock, SendBuf, lSent, 0);
						break;
					}   											
				}
				Sleep(100);
			}
			else//ÿ��ͨ�������ݴ���40MB���ְ���,ÿ��10MB
			{		
				//�ȷ���ͷ
				sprintf(szHeader+50,"%d",1);//�ְ�
				sprintf(szHeader+60,"%ld",N*Mega);//����
				int iPktCount = ceil(lSent/(N*Mega));//���ĸ���
				for (int k=0;k<iPktCount;k++)
				{					
					sprintf(szHeader+55,"%d",k);//��k��	
					for (int m=0;m<3;m++)
					{
						lRet = zmq_send(pSock, szHeader, 80,0);//i:�ڼ��Ųɼ���,j:�ÿ��ڼ���ͨ��,k:��ͨ���ڼ�����
						if( lRet == 80)
						{										
							//�ٷ������ļ�����							
							fread(SendBuf,N*Mega,1,fp);
							lRet = zmq_send(pSock, SendBuf, N*Mega,0);//i:�ڼ��Ųɼ���,j:�ÿ��ڼ���ͨ��,k:��ͨ���ڼ�����								
							break;
						}
					}
					Sleep(100);
				}
			}	
		}				
    }  
	delete SendBuf;
	return 0;
}
UINT ServerThread(LPVOID pParam)
{
	CGrapherServerDlg* pDlg = (CGrapherServerDlg*)pParam;	
	//���������
	fd_set rfd;// ����������   
	timeval timeout;
	int retVal=0,imode=1; 	
	// ���ó�ʱʱ��Ϊ2s
	timeout.tv_sec = 3;	
	timeout.tv_usec = 0;
   
	char recvData[100]={0};  
	char sendData[512]={0};
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
		//WSACleanup();  
		return -2;  
	}
    if(bind(serSocket, (sockaddr *)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
    {        
        closesocket(serSocket);
        return -3;
    }   	
    while (pDlg->m_flag)
    {
		LOG("running");
		//���ɼ���״̬
		if(pDlg->m_bIsRuning)
		{
			LOG("get status");
			pDlg->getStatus(serSocket);
		}
		//��ʼʹ��select
		FD_ZERO(&rfd);
		FD_SET(serSocket, &rfd); // ��serSocket����Ҫ���Ե�����������		
		retVal = select(0, &rfd, NULL, NULL, &timeout);// ����Ƿ����׽ӿ��Ƿ�ɶ�
		if (retVal == SOCKET_ERROR)	
		{
			int err = GetLastError();	
			closesocket(serSocket);		
			return -4;
		}
		else if (retVal == 0)// ��ʱ
		{			
			//closesocket(serSocket);			
			continue;
		}
		else//��⵽���׽ӿڿɶ�
		{
			if (FD_ISSET(serSocket, &rfd))	// serSocket�ɶ�
			{
				//AfxMessageBox("FD_ISSET");
				memset(recvData,0,100);
				int ret = recvfrom(serSocket, recvData, 100, 0, (sockaddr *)&serAddr,&nAddrLen);
				//int err = GetLastError();
				if (ret > 0)
				{		
					char szHeader[10]={0};
					memcpy(szHeader,recvData,10);
					char szSrcIP[30]={0};
					memcpy(szSrcIP,recvData+10,30);//����ԴIP				
					if(strcmp(szHeader,"cardnum")==0)//����(��ȡ�ɼ�������)�����
					{	
						//�ظ����
						memset(sendData,0,512);
						memcpy(sendData,"numret",7);								
						memcpy(sendData+10,pDlg->m_szHostIP,30); 
						sprintf(sendData+40,"%d",pDlg->m_DeviceNum);						
						sockaddr_in cliAddr;
						cliAddr.sin_family = AF_INET;
						cliAddr.sin_port = htons(ACKPort);
						cliAddr.sin_addr.S_un.S_addr = inet_addr(szSrcIP);
						sendto(serSocket, sendData, 100, 0, (sockaddr *)&cliAddr, nAddrLen);   				
					}
					else if(strcmp(szHeader,"status")==0)
					{	
						if(pDlg->m_bIsRuning)
						{
							pDlg->getStatus(serSocket);						
							//״̬�ذ�stret
							memset(sendData,0,512);
							memcpy(sendData,"numret",7);								
							memcpy(sendData+10,pDlg->m_szHostIP,30); 
							sprintf(sendData+40,"%s",pDlg->m_statustxt);						
							sockaddr_in cliAddr;
							cliAddr.sin_family = AF_INET;
							cliAddr.sin_port = htons(ACKPort);
							cliAddr.sin_addr.S_un.S_addr = inet_addr(szSrcIP);
							sendto(serSocket, sendData, 512, 0, (sockaddr *)&cliAddr, nAddrLen); 
						}				
					}
					else if(strcmp(szHeader,"start")==0)
					{						
						pDlg->m_startSrcIP=szSrcIP;
//						AfxMessageBox(recvData);
						pDlg->m_saved = false;
						//׼���ɼ�
						pDlg->OnPreSample();
						//��ʼ�ɼ�
						if(pDlg->m_DeviceNum > 0)
						{						  					  						
							for(long i=0;i<pDlg->m_DeviceNum;i++)
							{
								//�����ɼ�
								WCDPcie_StartAD(i);
							}
						}
						pDlg->m_bIsRuning = true;
					}
					else if(strcmp(szHeader,"stop")==0)
					{						
						//AfxMessageBox("stop");
						for(long i=0;i<pDlg->m_DeviceNum;i++)
						{
							WCDPcie_StopAD(i);
						}
						pDlg->m_bIsRuning = false;
					}
					else if(strcmp(szHeader,"setsf")==0)
					{
//						AfxMessageBox(recvData);
						LOG("setsf");
						pDlg->m_sf = atoi(recvData+40);
					}
					else if(strcmp(szHeader,"setsl")==0)
					{
						//AfxMessageBox(recvData);
						LOG("setsl");
						pDlg->m_sl = atoi(recvData+40);
					}
					else if(strcmp(szHeader,"settt")==0)
					{
//						AfxMessageBox(recvData);
						//AfxMessageBox("settt");
						pDlg->m_tt = atoi(recvData+40);
					}
					else if(strcmp(szHeader,"settul")==0)
					{
//						AfxMessageBox(recvData);
						
						pDlg->m_tul = atof(recvData+40);
					}
					else if(strcmp(szHeader,"settll")==0)
					{
//						AfxMessageBox(recvData);
	//					AfxMessageBox("settll");
						pDlg->m_tll = atoi(recvData+40);
					}
					else if(strcmp(szHeader,"revready")==0)
					{
						pDlg->m_startSrcIP=szSrcIP;
						pDlg->m_bRecvReady = TRUE;//�ͻ���׼���ý�����						
					}
				}
			}
		}        	
    }
    closesocket(serSocket); 	
	return 0;
}



void CGrapherServerDlg::OnStart() 
{
	char btnText[10]={0};
	m_start.GetWindowText(btnText,10);
	if(strcmp(btnText,"�� ʼ") == 0)
	{
		m_flag = 1;
		AfxBeginThread((AFX_THREADPROC)ServerThread,this,THREAD_PRIORITY_IDLE);
		m_start.SetWindowText("ͣ ֹ");
	}
	else
	{
		m_flag = 0;
		m_start.SetWindowText("�� ʼ");
	}
}
char* CGrapherServerDlg::getHostIP()
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
void CGrapherServerDlg::getStatus(SOCKET serSock)
{
	m_sucnum=0;
	m_statustxt = _T("1111");
	//�����ɼ���״̬
	CString str0;
	CString str1;
	CString str2;
	CString str3;
	m_statustxt.Format("%sδ��⵽�ɼ���!",m_szHostIP);
	unsigned long TriggerOK;
	unsigned long SoftTriggerOK;
	unsigned long CHTriggerOK;
	unsigned long OutTriggerOK;
	unsigned long RealSampleFrequent;
	unsigned long DDRCHLenthC0;
	unsigned long DDRCHLenthC1;
	long N1=1;
	//long SampleOKNum = 0;//�ɼ��ɹ��Ĳɼ�����
	if((m_DeviceNum > 0 ))
	{
		m_statustxt.Empty();	
		m_statustxt.Format("%s",m_szHostIP);			
		for(int i=0;i<m_DeviceNum;i++)
		{			
			str0.Format("�豸%d:",i+1);
			//////////////////////////////////////////////////////////////
			RealSampleFrequent=WCDPcie_GetCardSampleFrequent(i);
			str1.Format("Ƶ�ʣ�%dMHz;",RealSampleFrequent);
			////////////////////////////////////////////////////////////////
			WCDPcie_GetADStatus(i,TriggerOK,SoftTriggerOK,CHTriggerOK,OutTriggerOK);
			str2.Format("��%d;��%d;ͨ%d��%d;",SoftTriggerOK,OutTriggerOK,CHTriggerOK,TriggerOK);
			/////////////////////////////////////////////////////////////////
			
			DDRCHLenthC0=WCDPcie_GetC0DDRDataSize(i);
			
			while(!TriggerOK)
			{
				Sleep(10);//20171212
			}
			for(long j=0;j<m_DeviceNum;j++)
			{
				WCDPcie_StopAD(i);//20171212
			}
			//while((DDRCHLenthC1=WCDPcie_GetC1DDRDataSize(i))<(m_sl/64))
			{
				Sleep(100);
			}
			str3.Format("�ڴ泤��C1:%u",DDRCHLenthC1*64);
			//AfxMessageBox(str0+str1+str2+str3);
			m_statustxt=m_statustxt+str0+str1+str2+str3+" \n";			
			//�����������ļ�
			//AfxMessageBox("123456");
			//if(TriggerOK == 1 && m_saved == false)
			{	
				//memset(m_DataN[i],i,N1*Mega);
				//SampleOKNum++;			
				FILE *fp;
				char chFileName[MAX_PATH]={0};
				sprintf(chFileName,".//Card%dData",i+1);
				fp = fopen(chFileName,"wb");
				if (!fp)
				{
					return;
				}
				long lWrote = 0;//��д
				long lTotleLength = m_sl*2*m_SampleCHNum;//�����ܳ���
				//while(lWrote < lTotleLength)
				{
					//if (lTotleLength < N*Mega)
					{
						unsigned char *dataByChannel=new unsigned char[lTotleLength];
						lWrote = lTotleLength;
						CString mtul;

						WCDPcie_ReceiveReadDMA(i,lTotleLength,dataByChannel);
						TRACE("1");
						//mtul.Format("%.3lf",m_tul);
						//AfxMessageBox(mtul);
						//AfxMessageBox("12345");
						for(int chn=0;chn<8;chn++)
						{
							for(int idx=0;idx<lTotleLength/8/2;idx++)
							{
								fwrite(dataByChannel+2*(idx*8+chn),2,1,fp);
							}
						}
						fclose(fp);
						delete []dataByChannel;
						//fwrite(dataByChannel,lTotleLength,1,fp);	
						//fprintf(fp,"%6d\n",m_DataN[i]);
					}
					/*
					else if (lTotleLength - lWrote < N*Mega)//ʣ���С��N*Mega
					{
						lWrote = lTotleLength;
						WCDPcie_ReceiveReadDMA(i,lTotleLength - lWrote,m_DataN[i]);					
						fwrite(m_DataN[i],lTotleLength - lWrote,1,fp);	
						//fprintf(fp,"%6d\n",m_DataN[i]);
					}
					else
					{
						lWrote += N*Mega;
						WCDPcie_ReceiveReadDMA(i,N*Mega,m_DataN[i]);					
						fwrite(m_DataN[i],N*Mega,1,fp);	
						//fprintf(fp,"%6d\n",m_DataN[i]);
					}
					*/	
				}
				//fclose(fp);
				
				//
				
				m_sucnum++;
				if (m_sucnum == m_DeviceNum)//ȫ�������������ļ���
				{					
					m_saved = true;
				}
			}
		}
	}
	//m_bIsRuning = FALSE;//xym
	//����m_statustxt���ͻ���
	sockaddr_in sndAddr;
	sndAddr.sin_family = AF_INET;
	sndAddr.sin_port = htons(ACKPort);	
	sndAddr.sin_addr.S_un.S_addr = inet_addr(m_startSrcIP.GetBuffer(30));
	m_startSrcIP.ReleaseBuffer();
	int nAddrLen = sizeof(sndAddr); 	
	char sendData[512]={0};
	//UDP��֪ͨ�ͻ��ˣ����俪���ļ������߳�
	memcpy(sendData,"statustxt",10);								
	memcpy(sendData+10,m_szHostIP,30); 
	int iLen = m_statustxt.GetLength();
	memcpy(sendData+40,m_statustxt.GetBuffer(iLen),iLen);
	m_statustxt.ReleaseBuffer();
	sendto(serSock, sendData, 512, 0, (sockaddr *)&sndAddr, nAddrLen);  
	//���вɼ������Ѿ������������ļ���,�Ϳ��������ļ��̰߳Ѳɼ����ݷ����ͻ���	
	if(m_saved &&(!m_startSrcIP.IsEmpty()))
	{
		char sendData[100]={0};
		//UDP��֪ͨ�ͻ��ˣ����俪���ļ������߳�
		memcpy(sendData,"hisrec",7);								
		memcpy(sendData+10,m_szHostIP,30); 
		sendto(serSock, sendData, 100, 0, (sockaddr *)&sndAddr, nAddrLen);  
		//����TCP�����ļ��߳�
		if(m_bRecvReady)
		{		
			AfxBeginThread((AFX_THREADPROC)SendFileThread,this,THREAD_PRIORITY_IDLE);
			m_bIsRuning = FALSE;
		//	m_saved = true;
		}		
	}
}


void CGrapherServerDlg::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnTimer(nIDEvent);
}
