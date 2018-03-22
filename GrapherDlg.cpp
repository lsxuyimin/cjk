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
	//初始化成员变量
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
//文件接收线程
UINT RecvFileThread(LPVOID lpara)
{
	CGrapherDlg *pPCIeDlg=(CGrapherDlg*)lpara;
	void * pCtx = NULL;
    void * pSock = NULL;
    const char * pAddr = "tcp://*:7766";

    //创建context，zmq的socket 需要在context上进行创建 
    if((pCtx = zmq_ctx_new()) == NULL)
    {
        return 0;
    }
    //创建zmq socket ，socket目前有6中属性 ，这里使用dealer方式
    //具体使用方式请参考zmq官方文档（zmq手册） 
    if((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
    {
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    int iRcvTimeout = 5000;// millsecond
    //设置zmq的接收超时时间为5秒 
    if(zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iRcvTimeout, sizeof(iRcvTimeout)) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }
    //绑定地址 tcp://*:7766 
    //也就是使用tcp协议进行通信，使用网络端口 7766
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
		//先收包头
		long lRet = zmq_recv(pSock, szHeader, 80,0);		
		if(lRet == 80)//收到数据
		{
			memcpy(szCharacter,szHeader,10);
			memcpy(szSrcIP,szHeader+10,30);//来源IP
			//memcpy(szCardNum,szHeader+40,5);//第几张卡
			iCardID = atoi(szHeader+40);
			memcpy(szChannel,szHeader+45,5);//第几个通道
			//memcpy(szSplitPK,szHeader+50,5);
			iSpliPK = atoi(szHeader+50);
			iPKID = atoi(szHeader + 55);
			//memcpy(szPKID,szHeader+55,5);//第几个包
			memcpy(szPKLength,szHeader+60,20);//包长
			int iPKLength = atoi(szPKLength);
			if(strcmp(szCharacter,"CardData")==0)
			{						
				//再收包体
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
				if(lRet == iPKLength)//接收正确
				{
					if (iSpliPK == 1)//分包
					{
						if(iPKID==0)//该卡该通道的第一个包
						{
							int iFrontEmpty = 0;
							for(int i=0;i<pPCIeDlg->m_hostcount;i++)//创建通道文件
							{
								if(pPCIeDlg->m_lanhosts[i].cardnum == 0)
								{
									iFrontEmpty++;
									continue;
								}
								else if ((strcmp(szSrcIP,pPCIeDlg->m_lanhosts[i].ip) == 0)&&(pPCIeDlg->m_lanhosts[i].cardnum > 0))
								{
									//创建文件
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
									//写文件
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
					else//不分包，该包就是完整的通道文件
					{
						int iFrontEmpty = 0;
						for(int i=0;i<pPCIeDlg->m_hostcount;i++)//创建通道文件
						{
							if(pPCIeDlg->m_lanhosts[i].cardnum == 0)
							{
								iFrontEmpty++;
								continue;
							}
							else if ((strcmp(szSrcIP,pPCIeDlg->m_lanhosts[i].ip) == 0)&&(pPCIeDlg->m_lanhosts[i].cardnum > 0))
							{
								//创建文件
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
//心跳线程
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
	//设置为非阻塞模式  
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
		//LAN内主机采集卡数量更新				
        pPCIeDlg->getLanHosts();	
		pPCIeDlg->m_list1.DeleteAllItems();
		int hostCount = pPCIeDlg->m_hostcount;
		for(int i =0;i<pPCIeDlg->m_hostcount;i++)
		{		
			//给每个HOST发送"查看采集卡数量"请求	
			//AfxMessageBox(pPCIeDlg->m_lanhosts[i].ip);			
			sin.sin_addr.S_un.S_addr = inet_addr(pPCIeDlg->m_lanhosts[i].ip);//IP
			int len = sizeof(sin);    
			memset(sendData,0,100);
			memcpy(sendData,"cardnum",8);
			memcpy(sendData+10,pPCIeDlg->m_szHostIP,30);
			retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);		
			//给每个host发送"查询采集卡状态"请求
			memset(sendData,0,100);
			memcpy(sendData,"status",7);
			retVal = sendto(sclient, sendData, 100, 0, (sockaddr *)&sin, len);
		}		
		//list2实时更新		
		pPCIeDlg->InsertHisRecord();
		Sleep(1000);
	}
	return 0;
}
//监听线程
UINT ListenThread(LPVOID lpara)
{	
	CGrapherDlg *pPCIeDlg=(CGrapherDlg*)lpara;	
	//服务器相关
	fd_set rfd;     // 读描述符集   
	timeval timeout;
	int retVal=0,imode=1; 	
	char chNum[10] = {0};	
	char szHeader[20]={0};					
	char szSrcIP[30]={0};
	// 设置超时时间为20s
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
	//设置为非阻塞模式      
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
		//开始使用select
		FD_ZERO(&rfd);
		FD_SET(serSocket, &rfd); // 把serSocket放入要测试的描述符集中		
		retVal = select(0, &rfd, NULL, NULL, &timeout);// 检测是否有套接口是否可读
		if (retVal == SOCKET_ERROR)	
		{
			//int err = GetLastError();			
			return -4;
		}
		else if (retVal == 0)// 超时
		{									
			continue;
		}
		else// 检测到有套接口可读
		{
			if (FD_ISSET(serSocket, &rfd))	// serSocket可读
			{
				memset(recvData,0,512);
				int ret = recvfrom(serSocket, recvData, 512, 0, (sockaddr *)&serAddr,&nAddrLen);
				//int err = GetLastError();
				if (ret > 0)
				{		
					memset(szHeader,0,10);
					memset(szSrcIP,0,30);
					memcpy(szHeader,recvData,10);
					memcpy(szSrcIP,recvData+10,30);//包来源IP
					if(strcmp(szHeader,"numret")==0)//处理(获取采集卡数量)结果包
					{
						memset(chNum,0,10);							
						memcpy(chNum,recvData+40,4);
						int iNum = atoi(chNum);
						//对m_lanhosts更新
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
								if(strcmp(pPCIeDlg->m_lanhosts[i].ip,szSrcIP)==0)//数组中存在该ip	
								{
									pPCIeDlg->m_lanhosts[i].cardnum = iNum;
									IsRepeat = true;
								}
							}	
							if(!IsRepeat)//数组中没有该ip
							{
								strcpy(pPCIeDlg->m_lanhosts[pPCIeDlg->m_hostcount].ip,szSrcIP);
								pPCIeDlg->m_lanhosts[pPCIeDlg->m_hostcount].cardnum = iNum;
								pPCIeDlg->m_hostcount++;
							}
						}
						//更新List1						
						pPCIeDlg->m_list1.DeleteAllItems();
						for(int i=0;i<pPCIeDlg->m_hostcount;i++)
						{	
							char strIP[30]={0};
							strcpy(strIP,pPCIeDlg->m_lanhosts[i].ip);
							iNum = pPCIeDlg->m_lanhosts[i].cardnum;
							if(iNum>0)//采集卡数量大于0的才显示
							{
								int iRow = pPCIeDlg->m_list1.InsertItem(i,pPCIeDlg->m_lanhosts[i].ip);	
								sprintf(chNum,"%d",pPCIeDlg->m_lanhosts[i].cardnum);
								pPCIeDlg->m_list1.SetItemText(iRow,1,chNum);	
							}
						}
					}							
					else if(strcmp(szHeader,"hisrec")==0)//历史记录文件
					{		
						strcpy(pPCIeDlg->m_chFileIP,szSrcIP);
						CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RecvFileThread,pPCIeDlg,0,NULL);
						//发送"文件接收已准备好"
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
					else if(strcmp(szHeader,"stret")==0)//采集卡状态结果包
					{					
						pPCIeDlg->m_status.SetWindowText(recvData+40);//一次显示一条
					}
					else if(strcmp(szHeader,"hisreccop")==0)//服务器发送完毕
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
	//本机IP控件	
	strcpy(m_szHostIP,getHostIP()); 
	DWORD dwIP=ntohl(inet_addr(m_szHostIP)); 
	m_IP.SetAddress(dwIP);
	//list控件表头初始化
	m_list1.InsertColumn(0,_T("主机IP"),LVCFMT_CENTER,95);
	m_list1.InsertColumn(1,_T("采集卡数量"),LVCFMT_CENTER,134);
	m_list1.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);

	m_list2.InsertColumn(0,_T("文件夹名称"),LVCFMT_CENTER,106);
	m_list2.InsertColumn(1,_T("通道数"),LVCFMT_CENTER,103);
	m_list2.InsertColumn(2,_T("采样长度"),LVCFMT_CENTER,103);
	m_list2.InsertColumn(3,_T("采样率"),LVCFMT_CENTER,103);
	m_list2.SetExtendedStyle(LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
	//焦点设置
	GetDlgItem(IDC_LIST2)->SetFocus();
	//滑块范围设置
	m_slider1.SetRange(0,5000);
	m_slider2.SetRange(0,5000);	
	//组合框下拉列表
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

	m_combo3.AddString("联合触发");
	m_combo3.AddString("立即触发");	
	m_combo3.SetCurSel(0);	
	//list1	
/*	getLanHosts();
	char chNum[10]={0};
	for(int i=0;i<m_hostcount;i++)
	{					
		if(m_lanhosts[i].cardnum>0)//采集卡数量大于0的才显示
		{
			memset(chNum,0,10);
			m_list1.InsertItem(i,m_lanhosts[i].ip);	
			sprintf(chNum,"%d",0);
			m_list1.SetItemText(i,1,chNum);	
		}
	}*/
	//选择历史文件夹	
	int ret = OnBrowseforfolder("请选择历史记录文件夹:",true);	
	if(ret == 1)//取消或者路径不正确或者没有历史记录
	{
		AfxMessageBox("未选择路径或者历史记录为空或者路径不正确!将不显示历史记录");
		//return false;
	}
	//插入历史记录到list2	
	InsertHisRecord();
	UpdateData(false);
	//获取本机采集卡数量
	//m_CardNum=WCDPcie_ScanfDevice(0x10ee,0x0007,m_BusIDArray,m_SlotIDArray);	
	//开启监听线程
	CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ListenThread,this,0,NULL);
	//开启心跳线程	
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
	//清空list2
	m_list2.DeleteAllItems();
	if(!m_strSampleSavePath.IsEmpty())
	{
		//更新setup.inf文件
		CFile file;
		CString FileName;	
		FileName.Format("%s\\Setup.inf",m_strSampleSavePath);	
		if(!file.Open(FileName, CFile::modeCreate | CFile::modeWrite|CFile::typeBinary))
			return;
		CString csBuf;
		//  0.检查行
		csBuf.Format("RECORDER SETTING FILE\r\n");
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	1.通道数
		long lListItemCount = m_list1.GetItemCount();
		csBuf.Format("%d\r\n",lListItemCount*8);
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	2.量程		（保留 默认10）
		csBuf.Format("10\r\n");
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	3.采样长度
		int nIndex  = m_combo1.GetCurSel();			
		m_combo1.GetLBText(nIndex,csBuf);
		csBuf += "\r\n";
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	4.采样速率
		nIndex  = m_combo2.GetCurSel();			
		m_combo2.GetLBText(nIndex,csBuf);
		csBuf += "\r\n";
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//	5.触发方式（1：立即触发 4：通道触发）
		nIndex  = m_combo3.GetCurSel();
		csBuf.Format("%d\r\n",nIndex);
		file.Write(csBuf,csBuf.GetLength());
		csBuf.Empty();
		//pPCIeDlg->m_combo3.GetLBText(nIndex,csBuf);
		//	6.触发通道（保留 默认0）
		csBuf.Format("0\r\n");
		file.Write(csBuf,csBuf.GetLength());		
		//	7.触发电平（保留 默认0）
		//csBuf.Format("0\n");
		file.Write(csBuf,csBuf.GetLength());		
		//	8.采样延时（保留 默认0）
		//csBuf.Format("0\n");
		file.Write(csBuf,csBuf.GetLength());
		file.Close();
	}
	//读取历史记录并显示至list2控件		 
	 if(!m_strHisFilePath.IsEmpty())
	 {			 
		 int filecount = 0;
		 int i=0;
		 //获取该文件夹下所有的历史文件夹
		 getHisFile(m_strHisFilePath,m_FileList,filecount);		 
		 if(!m_FileList->empty())
		 {
			 //int size = m_FileList->size();
			 vector<CString>::iterator it;
			 for(it=m_FileList->begin();it!=m_FileList->end();it++)
			 {
				//截取文件名
				int nCount = (*it).ReverseFind('\\');
				CString FolderName = (*it).Mid(nCount+1);
				m_list2.InsertItem(i,FolderName);
				//获取文件指定行的内容
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
	//获取配置文件路径 	
	CString CfgPath;
	GetModuleFileName(NULL,CfgPath.GetBuffer(MAX_PATH),MAX_PATH);
	CfgPath.ReleaseBuffer();
	int nPos  = CfgPath.ReverseFind('\\');
	CfgPath = CfgPath.Left(nPos);		
	CfgPath += "\\Grapher.ini";
	//读取配置文件内容
	FILE *fp = fopen(CfgPath,"a+");	
	if(fp)
	{	
		if(IsOpenHis)
		{
			int err = GetPrivateProfileString("Configure","HisPath","",m_strHisFilePath.GetBuffer(MAX_PATH),MAX_PATH,CfgPath);
			m_strHisFilePath.ReleaseBuffer();			
			if(m_strHisFilePath.IsEmpty())//如果配置文件为空
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
			if(m_strSampleSavePath.IsEmpty())//如果配置文件为空
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
	struct in_addr *ptr;    // 获得IP地址     
	DWORD dwScope = RESOURCE_CONTEXT;
	NETRESOURCE *NetResource = NULL;
	HANDLE hEnum;
	WNetOpenEnum( dwScope,NULL,NULL,NULL,&hEnum);	
	//WSADATA   wsaData;
	//WSAStartup(MAKEWORD(1, 1), &wsaData);
	//开始枚举网络资源	
	if ( hEnum )     //如果句柄有效
	{
		DWORD Count = 0xFFFFFFFF;
		DWORD BufferSize = 2048;
		LPVOID Buffer = new char[2048];
		//memset(Buffer,0,2048);
		// 调用WSAStartup后调用WNetEnumResource做进一步的枚举工作
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
					//去掉头部的"//"    
					char* pstr = (char*)malloc((namelen)*sizeof(char));
					if ((strFullName[0]=='\\')&&(strFullName[1]=='\\'))                                                
					{
						strcpy(pstr,strFullName+2);
					}
					//获得主机名
					gethostname( szHostName, strlen( szHostName ) );
					//由主机名获得跟它对应的主机信息
					host = gethostbyname(pstr);
					if(host == NULL) continue; 
					ptr = (struct in_addr *) host->h_addr_list[0];                    
					// 提取IP地址信息，地址形式如下： 211.40.35.76                 
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
		// 结束枚举工作
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
	//对结构体数组按照IP地址升序排列
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
//获取文件夹下历史记录文件夹
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
			//过滤
			//CString temp = filename.Left(2);
			//if(temp.CompareNoCase("Ch") == 0)
			m_FileList->push_back(path+"\\"+filename);		
		} 
		else 
		{
		/*	CString filename = finder.GetFileName();
			//过滤
			CString temp = filename.Left(2);
			if(temp.CompareNoCase("Ch") == 0)
				m_FileList->push_back(path+"\\"+filename);
			//filenames[count++] = filename;*/
		}
	}	
	//排序
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
	while (!feof(fp))//文件未结束
	{
		memset(data, 0, LINE_SIZE);
		fgets(data, LINE_SIZE - 1, fp);		
		curLine++;
		//去掉空格
		int i = -1, j = 0;    
		while (data[++i])   
		{
			if (data[i] != ' ')            
				dst[j++] = data[i];    
		}
		dst[j] = '\0';
		if (curLine > whichLine)
		{
			reachWhichLine = 1; //已经读取到whichLine行
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
                 //BFFM_INITIALIZED表示浏览对话框已经初化结束，参数lParam为NULL
                 //设置初始选项
                 ::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,lpData);
                 //关于BFFM_SETSELECTION消息的说明
                 //wParam ：标记lParam参数包含一个ITEMIDLIST结构(PIDL)还是一个目录路径名
                 //          如果为TRUE，lParam内容为路径名；否则lParam包含一个路径PIDL。
                 //lParam ：内容为浏览对话框所选的路径。如果wParam为TRUE，lParam内容为一个
                 //         以NULL结尾的字符串的指针，否则为PIDL
                 break;
           }

      case BFFM_SELCHANGED:
           {
                 //BFFM_SELCHANGED表示选择项已经发生变化，参数lParam包含列表中最新选中项的条目ID               
                 ITEMIDLIST * pidl; 
                 char path[MAX_PATH]; 
                 //根据条目ID取路径信息
                 pidl = (ITEMIDLIST*) lParam;
                 if (SHGetPathFromIDList(pidl, path))
                 {
                      //使得“确认”按钮生效
                      //关于BFFM_ENABLEOK消息的说明
                      //wParam ：无意义，可设置为0
                      //lParam ：如果为非0，则使能确认按钮；否则失效“确认”按钮
                      ::SendMessage(hwnd,BFFM_ENABLEOK,0,TRUE);
                      //读属性
                      DWORD attributes = ::GetFileAttributes(path);
                      //命令状态行显示当前所选项的全路径名及其文件属性
                      //关于BFFM_SETSTATUSTEXT消息的说明
                      //wParam ：无意义，可设置为0
                      //lParam ：指向一个内含状态行提示信息的字符串
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
                      //使得“确认”按钮失效
                      ::SendMessage(hwnd,BFFM_ENABLEOK,0,FALSE);
                      //清状态行信息
                      ::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)(LPTSTR)(LPCTSTR)"");
                 }
                 break;			   
           }
      case BFFM_VALIDATEFAILED:
           {
                 //BFFM_VALIDATEFAILED表示用户在浏览对话框的编辑框内输入了一个非法名称
                 //该消息在用户按“确认”时送出——当然前提是编辑框内输入的名称非法
                 //lParam参数包含了非法输入内容的地址，应用程序可以使用这个消息提示用户输入非法。
                 //另外，此消息的回调函数返回0表示目录浏览对话框旋即关闭，返回其他值则允许对话框继续显示。
                 //仅当目录浏览对话框中含有编辑框并且设置了BIF_VALIDATE标记才可能出现此消息
                 //即BROWSEINFO结构中ulFlags含有BIF_EDITBOX|BIF_VALIDATE标志
                 CString strTip;
                 strTip.Format("目录%s非法!",lParam);
                 //返回0允许对话框提前关闭，SHBrowseForFolder()返回NULL
                 //AfxMessageBox(strTip);
                 //return 0;
                 //返回1对话框继续显示，因为对话框仍继续显示，可以在状态行显示出错消息
                 //注意：如果此时仍用AfxMessageBox来显示提示信息，提示信息框关闭后，要使焦点重返目录
                 //浏览对话框，需要客户手工移动鼠标激活该对话框才行，这样会使得后继操作不是很方便，所以在状态行显示提示信息比较好
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
      //定义目录浏览对话框的属主
      bi.hwndOwner = NULL;
      //确定浏览范围（根目录）
      //只有根目录及其子目录下可以浏览
      //定义为NULL表示名字空间下皆可用
      bi.pidlRoot = NULL;
      //用于接收用户所选目录的显示名
      //经测试，该项并不包含全路径名
      bi.pszDisplayName = dispname; 
      //设置目录浏览对话框的对话框标题
      bi.lpszTitle = dlgTitle;//"请选择路径:"; 
      //设置状态
      //BIF_BROWSEINCLUDEFILES
      //BIF_RETURNONLYFSDIRS表示只返回目录，
      //BIF_STATUSTEXT表示对话框中有状态行
      //BIF_EDITBOX表示对话框中有编辑框
      //BIF_VALIDATE表示客户按“确认”按钮时检查编辑框内容的合法性
	  
      bi.ulFlags = BIF_NEWDIALOGSTYLE|BIF_BROWSEINCLUDEFILES|BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT|BIF_EDITBOX|BIF_VALIDATE;
      //设置回调函数
      //如果需要设置初始选择项、显示所选项的相关信息、让系统自动校验用户输入
      //的合法性，那么应该使用回调函数；否则可将该项设置为NULL
      bi.lpfn = BrowseCallbackProc; 
      //设置回调函数的lParam参数
      //此处用来传递目录浏览对话框的初始选项
	  if(IsOpenHis)
		bi.lParam = (LPARAM)(LPCTSTR)m_strHisFilePath; 
	  else
		bi.lParam = (LPARAM)(LPCTSTR)m_strSampleSavePath; 
      //用来接收所选目录的图标（系统图像列表中的序号）
      bi.iImage = NULL; 
      //显示目录浏览对话框
      if (pidl = SHBrowseForFolder(&bi))
      { 
           //将PIDL转换为字符串
           if (SHGetPathFromIDList(pidl, path))
           {
                 //更新对话框显示，以显示用户的最新选择
				 if(IsOpenHis)
					m_strHisFilePath = path; 
				 else
					m_strSampleSavePath = path;
                 UpdateData(FALSE);
				 //获取配置文件路径 
				 CString CfgPath;
				 GetModuleFileName(NULL,CfgPath.GetBuffer(MAX_PATH),MAX_PATH);
				 CfgPath.ReleaseBuffer();
				 int nPos  = CfgPath.ReverseFind('\\');
				 CfgPath = CfgPath.Left(nPos);		
				 CfgPath += "\\Grapher.ini";
				 FILE *fp = fopen(CfgPath,"a+");
				 if(fp)
				 {
				    //保存历史记录路径至配置文件
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
		  //AfxMessageBox("未选择历史记录路径!将不显示历史记录");
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
	//选择保存文件夹和其名称
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		AfxMessageBox("当前局域网内无采集卡!不做采集!");
		return;
	}
	ret = OnBrowseforfolder("请选择采集成功记录保存路径:",false);
	if(ret == 1)
	{
		AfxMessageBox("未选择采集记录保存路径!无法开始采集!");
		return;
	}	
	//更新配置文件Setup.inf
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
	//设置为非阻塞模式  
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
		AfxMessageBox("当前局域网内无采集卡!发送指令失败!");
		return;
	}
	//发包
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
	//设置为非阻塞模式  
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
		//给每个HOST发送"停止采集"请求	
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
	//发包
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
	//设置为非阻塞模式  
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
	//发包
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
	//设置为非阻塞模式  
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
		//AfxMessageBox("当前局域网内无采集卡!不做采集!");
		return;
	}

	//发包
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
	//设置为非阻塞模式  
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
		//给每个HOST发送"查看采集卡数量"请求	
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
		//AfxMessageBox("当前局域网内无采集卡!不做采集!");
		return;
	}
	//发包
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
	//设置为非阻塞模式  
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
		AfxMessageBox("历史记录为空!");
	}
}

void CGrapherDlg::OnDeleteRecord() 
{
	//删除一个文件
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
				temp.Format("删除失败!错误码:%d",ret);
				AfxMessageBox(temp);		
			}
			//更新list2		
			InsertHisRecord();
			UpdateData(false);
		}
		else
		{
			AfxMessageBox("请选择一条记录!");
		}
	}
	else
	{
		AfxMessageBox("历史记录为空!");
	}
}

void CGrapherDlg::OnEditchangeCombo2() 
{
	int ret = m_list1.GetItemCount();
	if(ret<=0)
	{
		//AfxMessageBox("当前局域网内无采集卡!设置无效!");
		return;
	}
	int nIndex  = m_combo1.GetCurSel();	
	CString csValue;
	m_combo1.GetLBText(nIndex,csValue);
	//发包
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
	//设置为非阻塞模式  
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
		//AfxMessageBox("当前局域网内无采集卡!不做采集!");
		return;
	}
	int nIndex  = m_combo2.GetCurSel();	
	CString csValue;
	m_combo2.GetLBText(nIndex,csValue);
	//发包	
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
	//设置为非阻塞模式  
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
		//给每个HOST发送"查看采集卡数量"请求	
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
		//AfxMessageBox("当前局域网内无采集卡!不做采集!");
		return;
	}
	int nIndex  = m_combo3.GetCurSel();	
	CString csValue;
	//m_combo3.GetLBText(nIndex,csValue);
	csValue.Format("%d",nIndex);
	//发包	
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
	//设置为非阻塞模式  
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
