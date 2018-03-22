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
	//初始化成员变量
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
	//初始化网络
	WORD wVersionRequested;   
    WSADATA wsaData;   
    int err;     
    // 启动socket api   
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
	unsigned long TriggerType = m_tt;//0立即触发 15联合触发
	unsigned long PreFifoLen;
	unsigned long PreTriggerLen=0;
	unsigned long TriggerVoltMax = m_tul*10000;//触发上限
	TriggerVoltMax = (int)((m_tul+5)/10.f*65535);//触发上限
	unsigned long TriggerVoltMin = m_tll*10000;//触发下限
	TriggerVoltMin = (int)((m_tll+5)/10.f*65535);//触发下限
	unsigned long ContinueLen=2000;
	unsigned long ContinueStep=2000;
	unsigned long ContinueTimes=40000;
	unsigned long m_CHSampleLength = m_sl;//采样长度
	/*//0立即触发 15联合触发
	if(m_combo3.GetCurSel()==0)
		TriggerType = 0;
	else
		TriggerType = 15;
	//采样长度
	m_combo1.GetWindowText(temp);
	_stscanf(temp, _T("%ld"), &m_CHSampleLength);
	//上下限
	TriggerVoltMax = m_slider1.GetPos()*10000;
	TriggerVoltMin = m_slider2.GetPos()*10000;*/
	if(TriggerVoltMax < TriggerVoltMin)
	{
		//m_status.SetWindowText("触发上限设置错误!请重新设置");
		return;
	}
	else if((TriggerVoltMax == 0) && (TriggerVoltMin == 0))
	{
		//m_status.SetWindowText("未设置触发上下限,将使用默认设置!");
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
		//采样速率
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
    //使用tcp协议进行通信,通信使用的网络端口为7766 
    char pAddr[256];
	sprintf(pAddr,"tcp://%s:7766",pDlg->m_startSrcIP);	
    //创建context 
    if((pCtx = zmq_ctx_new()) == NULL)
    {
        return 0;
    }
    //创建socket 
    if((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
    {
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    int iSndTimeout = 5000;// millsecond
    //设置接收超时 
    if(zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iSndTimeout, sizeof(iSndTimeout)) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    //连接目标IP，端口7766 
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
		long lSent = lTotleLength/pDlg->m_SampleCHNum;//一个通道的数据大小		
		for (int j=0;j<pDlg->m_SampleCHNum;j++)//每次发送一个通道的数据
		{	
			sprintf(szHeader+45,"%d",j);
			if (lSent < N*Mega)//每个通道的数据小于10MB，直接发
			{		
				//先发包头
				sprintf(szHeader+50,"%d",0);//不分包
				sprintf(szHeader+60,"%ld",lSent);
				for (int m=0;m<3;m++)
				{
					lRet = zmq_send(pSock, szHeader, 80, 0);
					if( lRet == 80)
					{
						//再发本地文件数据												
						fread(SendBuf,lSent,1,fp);//这里好像(确实）(曾经)有问题-------------------------
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
			else//每个通道的数据大于40MB，分包发,每包10MB
			{		
				//先发包头
				sprintf(szHeader+50,"%d",1);//分包
				sprintf(szHeader+60,"%ld",N*Mega);//包长
				int iPktCount = ceil(lSent/(N*Mega));//包的个数
				for (int k=0;k<iPktCount;k++)
				{					
					sprintf(szHeader+55,"%d",k);//第k包	
					for (int m=0;m<3;m++)
					{
						lRet = zmq_send(pSock, szHeader, 80,0);//i:第几张采集卡,j:该卡第几个通道,k:该通道第几个包
						if( lRet == 80)
						{										
							//再发本地文件数据							
							fread(SendBuf,N*Mega,1,fp);
							lRet = zmq_send(pSock, SendBuf, N*Mega,0);//i:第几张采集卡,j:该卡第几个通道,k:该通道第几个包								
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
	//服务器相关
	fd_set rfd;// 读描述符集   
	timeval timeout;
	int retVal=0,imode=1; 	
	// 设置超时时间为2s
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
	//设置为非阻塞模式      
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
		//检查采集卡状态
		if(pDlg->m_bIsRuning)
		{
			LOG("get status");
			pDlg->getStatus(serSocket);
		}
		//开始使用select
		FD_ZERO(&rfd);
		FD_SET(serSocket, &rfd); // 把serSocket放入要测试的描述符集中		
		retVal = select(0, &rfd, NULL, NULL, &timeout);// 检测是否有套接口是否可读
		if (retVal == SOCKET_ERROR)	
		{
			int err = GetLastError();	
			closesocket(serSocket);		
			return -4;
		}
		else if (retVal == 0)// 超时
		{			
			//closesocket(serSocket);			
			continue;
		}
		else//检测到有套接口可读
		{
			if (FD_ISSET(serSocket, &rfd))	// serSocket可读
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
					memcpy(szSrcIP,recvData+10,30);//包来源IP				
					if(strcmp(szHeader,"cardnum")==0)//处理(获取采集卡数量)请求包
					{	
						//回复结果
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
							//状态回包stret
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
						//准备采集
						pDlg->OnPreSample();
						//开始采集
						if(pDlg->m_DeviceNum > 0)
						{						  					  						
							for(long i=0;i<pDlg->m_DeviceNum;i++)
							{
								//启动采集
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
						pDlg->m_bRecvReady = TRUE;//客户端准备好接收了						
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
	if(strcmp(btnText,"开 始") == 0)
	{
		m_flag = 1;
		AfxBeginThread((AFX_THREADPROC)ServerThread,this,THREAD_PRIORITY_IDLE);
		m_start.SetWindowText("停 止");
	}
	else
	{
		m_flag = 0;
		m_start.SetWindowText("开 始");
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
	//本机采集卡状态
	CString str0;
	CString str1;
	CString str2;
	CString str3;
	m_statustxt.Format("%s未检测到采集卡!",m_szHostIP);
	unsigned long TriggerOK;
	unsigned long SoftTriggerOK;
	unsigned long CHTriggerOK;
	unsigned long OutTriggerOK;
	unsigned long RealSampleFrequent;
	unsigned long DDRCHLenthC0;
	unsigned long DDRCHLenthC1;
	long N1=1;
	//long SampleOKNum = 0;//采集成功的采集卡数
	if((m_DeviceNum > 0 ))
	{
		m_statustxt.Empty();	
		m_statustxt.Format("%s",m_szHostIP);			
		for(int i=0;i<m_DeviceNum;i++)
		{			
			str0.Format("设备%d:",i+1);
			//////////////////////////////////////////////////////////////
			RealSampleFrequent=WCDPcie_GetCardSampleFrequent(i);
			str1.Format("频率：%dMHz;",RealSampleFrequent);
			////////////////////////////////////////////////////////////////
			WCDPcie_GetADStatus(i,TriggerOK,SoftTriggerOK,CHTriggerOK,OutTriggerOK);
			str2.Format("软%d;外%d;通%d总%d;",SoftTriggerOK,OutTriggerOK,CHTriggerOK,TriggerOK);
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
			str3.Format("内存长度C1:%u",DDRCHLenthC1*64);
			//AfxMessageBox(str0+str1+str2+str3);
			m_statustxt=m_statustxt+str0+str1+str2+str3+" \n";			
			//保存至本地文件
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
				long lWrote = 0;//已写
				long lTotleLength = m_sl*2*m_SampleCHNum;//数据总长度
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
					else if (lTotleLength - lWrote < N*Mega)//剩余的小于N*Mega
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
				if (m_sucnum == m_DeviceNum)//全部保存至本地文件了
				{					
					m_saved = true;
				}
			}
		}
	}
	//m_bIsRuning = FALSE;//xym
	//发送m_statustxt给客户端
	sockaddr_in sndAddr;
	sndAddr.sin_family = AF_INET;
	sndAddr.sin_port = htons(ACKPort);	
	sndAddr.sin_addr.S_un.S_addr = inet_addr(m_startSrcIP.GetBuffer(30));
	m_startSrcIP.ReleaseBuffer();
	int nAddrLen = sizeof(sndAddr); 	
	char sendData[512]={0};
	//UDP先通知客户端，让其开启文件接收线程
	memcpy(sendData,"statustxt",10);								
	memcpy(sendData+10,m_szHostIP,30); 
	int iLen = m_statustxt.GetLength();
	memcpy(sendData+40,m_statustxt.GetBuffer(iLen),iLen);
	m_statustxt.ReleaseBuffer();
	sendto(serSock, sendData, 512, 0, (sockaddr *)&sndAddr, nAddrLen);  
	//所有采集卡都已经保存至本地文件了,就开启发送文件线程把采集数据发给客户端	
	if(m_saved &&(!m_startSrcIP.IsEmpty()))
	{
		char sendData[100]={0};
		//UDP先通知客户端，让其开启文件接收线程
		memcpy(sendData,"hisrec",7);								
		memcpy(sendData+10,m_szHostIP,30); 
		sendto(serSock, sendData, 100, 0, (sockaddr *)&sndAddr, nAddrLen);  
		//开启TCP发送文件线程
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
