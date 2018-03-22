#define PLX_API __declspec(dllexport)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//用户读DMA内存接口
typedef void (*ReadDMA_HANDLER)(DWORD DMASize,unsigned char* MemData);
//用户写DMA内存接口
typedef void (*WriteDMA_HANDLER)(DWORD DMASize,unsigned char* MemData);

typedef struct tagPcieInfo 
{
	BOOL   PcieWriteUsed;
	double PcieWriteSize;
	double PcieWriteSpeed;
	double PcieWriteTime;
	double PcieWriteEmpty;
	double PcieWriteFull;
	double PcieWriteDDRLen;
	////////////////////////
	BOOL   PcieReadUsed;
	double PcieReadSize;
	double PcieReadSpeed;
	double PcieReadTime;
	double PcieReadEmpty;
	double PcieReadFull;
	double PcieReadDDRLen;
}PCIEINFO;


extern "C"
{
	PLX_API   long WCDPcie_ScanfDevice(DWORD VENID,DWORD DENID,unsigned long *BusIDArray,unsigned long *SlotIDArray);
	PLX_API   long WCDPcie_OpenAllDevice(DWORD VENID=0x10ee,DWORD DENID=0x0007);
	PLX_API   long WCDPcie_OpenDevice(DWORD VENID=0x10ee,DWORD DENID=0x0007,unsigned long BusID=-1,unsigned long SlotID=-1,long DeviceID=-1);   
    PLX_API   long WCDPcie_OpenDeviceByID(DWORD VENID,DWORD DENID,long DeviceID);

	PLX_API   long WCDPcie_GetDeviceInfo(long DeviceID,char *InfoStr,DWORD VendorID=0x10ee,DWORD ProductID=0x0007);
	PLX_API   long WCDPcie_GetDeviceInfoEx(long DeviceID,unsigned long *VID,unsigned long *PID,unsigned long *ChainSpeed,unsigned long *ChainWidth,unsigned long *ChainIsOK,char *ChainState);

    PLX_API   long WCDPcie_ReadCtl(long DeviceID,DWORD offset,UINT*outdata);
	PLX_API   long WCDPcie_WriteCtl(long DeviceID,DWORD offset,UINT indata);	
    PLX_API   long WCDPcie_ReadReg(long DeviceID,DWORD offset,UINT*outdata);
	PLX_API   long WCDPcie_WriteReg(long DeviceID,DWORD offset,UINT indata);
	PLX_API   long WCDPcie_SetCardSampleFrequent(long DeviceID,unsigned long Frequent,unsigned long ClockMode=0);
	PLX_API   long WCDPcie_GetCardSampleFrequent(long DeviceID);

	PLX_API   long WCDPcie_Reset_AD_DDR(long DeviceID,unsigned long AutoOrMannual=0,unsigned long Rst=0);
	PLX_API   long WCDPcie_Reset_DA_DDR(long DeviceID,unsigned long AutoOrMannual=0,unsigned long Rst=0);

	PLX_API   long WCDPcie_StartAD(long DeviceID);
	PLX_API   long WCDPcie_StartDA(long DeviceID);
	PLX_API   long WCDPcie_StopAD(long DeviceID);
	PLX_API   long WCDPcie_StopDA(long DeviceID);
	PLX_API   long WCDPcie_CloseDevice(long DeviceID);

	PLX_API   unsigned long WCDPcie_GetC0DDRDataSize(long DeviceID);
	PLX_API   unsigned long WCDPcie_GetC1DDRDataSize(long DeviceID);

	PLX_API   unsigned long WCDPcie_GetADCHNum(long DeviceID,unsigned long *C0CHNum=NULL,unsigned long *C1CHNum=NULL);
	PLX_API   unsigned long WCDPcie_GetDACHNum(long DeviceID,unsigned long *C0CHNum=NULL,unsigned long *C1CHNum=NULL);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PLX_API   long WCDPcie_InitReadDMA(long DeviceID,UINT DMAID,UINT BlockNum,UINT BlockSize);
	PLX_API   long WCDPcie_InitReadFile(long DeviceID,UINT DMAID,UINT BlockNum,UINT BlockSize,__int64 DMASize,char *FileName,ReadDMA_HANDLER  ReadDMAFunHandle);
	PLX_API   long WCDPcie_InitReadMem(long DeviceID,UINT DMAID,UINT BlockNum,UINT BlockSize,__int64 DMASize,unsigned char *MemData,ReadDMA_HANDLER  ReadDMAFunHandle);
	PLX_API   long WCDPcie_ReceiveReadDMA(long DeviceID,__int64 DMASize,unsigned char *MemData);
	PLX_API   long WCDPcie_CreateReadDMAThread(long DeviceID,BOOL CreateThreadForEnd); 
	PLX_API   long WCDPcie_StopReadDMA(long DeviceID);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PLX_API   long WCDPcie_InitWriteFile(long DeviceID,UINT DMAID,UINT BlockNum,UINT BlockSize,__int64 DMASize,char *FileName,WriteDMA_HANDLER WriteDMAFunHandle);
	PLX_API   long WCDPcie_InitWriteMem(long DeviceID,UINT DMAID,UINT BlockNum,UINT BlockSize,__int64 DMASize,unsigned char *MemData);
	PLX_API   long WCDPcie_InitWriteDMA(long DeviceID,UINT DMAID,UINT BlockNum,UINT BlockSize);//x
	PLX_API   long WCDPcie_SendWriteDMA(long DeviceID,__int64 DMASize,unsigned char *MemData);//x
	PLX_API   long WCDPcie_CreateWriteDMAThread(long DeviceID,BOOL CreateThreadForEnd);
	PLX_API   long WCDPcie_StopWriteDMA(long DeviceID);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PLX_API   long WCDPcie_SetC0C1Ctl(long DeviceID,unsigned long C0_DARDEn,unsigned long C1_ADRDEn,unsigned long NowRD_C0OrC1);
	PLX_API   long WCDPcie_SetADSampleLen(long DeviceID,unsigned long ADSampleLen);
	PLX_API   long WCDPcie_SoftTrigger(long DeviceID);
	PLX_API   long WCDPcie_SetADParamter(long DeviceID,unsigned long TriggerType,unsigned long PreFifoLen,unsigned long TriggerDataMax,unsigned long TriggerDataMin,unsigned long TriggerUsedCH,unsigned long ContinueLen,unsigned long ContinueStep,unsigned long ContinueTimes);
	PLX_API   long WCDPcie_GetADStatus(long DeviceID,unsigned long &TriggerOK,unsigned long &SoftTriggerOK,unsigned long &CHTriggerOK,unsigned long &OutTriggerOK);
	PLX_API   long WCDPcie_GetADIsWork(long DeviceID,unsigned long &IsSampleNow);//long DeviceID,unsigned long &IsWaitSmaple,unsigned long &IsSampleNow,unsigned long &IsSampleEnd, unsigned long &IsTriggerOK
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PLX_API   long WCDPcieDA_InitDAParameter(long DeviceID,unsigned long DAWorkMode=1,unsigned long StartPlayDDRSize=1500000000,unsigned __int64 RealPlaySize=0);//Max=2G

	PLX_API   PVOID* WCDPcieDA_GetWrbuffer(long DeviceID);
	PLX_API   long  WCDPcieDA_GetCurrentWrPoint(long DeviceID);
	PLX_API   long  WCDPcieDA_GetPcieInfo(long DeviceID,double *PcieWriteSpeed,double *PcieWriteSize,double* PcieWriteTime);
	PLX_API   long  WCDPcieDA_GetDAMemInfo(long DeviceID,unsigned long *WriteEmptyError,unsigned long *WriteFullError,double *WriteDDRLen);

	PLX_API   long  WCDPcieAD_GetPcieInfo(long DeviceID,double *PcieReadSpeed,double *PcieReadSize,double* PcieReadTime);
	PLX_API   long  WCDPcieAD_GetDAMemInfo(long DeviceID,unsigned long *ReadEmptyError,unsigned long *ReadFullError,double *ReadDDRLen);


 	PLX_API   long  WCDPcieDA_WriteSPI(long DeviceID,unsigned int ChipID,unsigned int  Address,unsigned long Data);
	PLX_API   unsigned long  WCDPcieDA_ReadSPI(long DeviceID,unsigned int  ChipID,unsigned int  Address);
 	PLX_API   long  WCDPcieDA_NCOSet(long DeviceID,unsigned int ChipID,long NCOEn,unsigned long  NCOFrequent,unsigned long HLSideImage,long IQEnable,long InverseSinCEn);//0x27
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 	PLX_API   long  WCDPcieDA_SetDAMode0_FpgaDDSFreq(long DeviceID,long DAMode,double CH0Feq,double CH1Feq,double CH2Feq,double CH3Feq,double CH4Feq,double CH5Feq,double CH6Feq,double CH7Feq);
	PLX_API   long  WCDPcieDA_SetDAMode0_FpgaDDSAmp(long DeviceID,double CH0Amp,double CH1Amp,double CH2Amp,double CH3Amp,double CH4Amp,double CH5Amp,double CH6Amp,double CH7Amp);
 	PLX_API   long  WCDPcieDA_SetDAMode0_FpgaDDSOffset(long DeviceID,double CH0Offset,double CH1Offset,double CH2Offset,double CH3Offset,double CH4Offset,double CH5Offset,double CH6Offset,double CH7Offset);

 	PLX_API   long  WCDPcieDA_SetDAMode1_PCData(long DeviceID,long DAMode);
 	PLX_API   long  WCDPcieDA_SetDAMode2_FpgaConstant(long DeviceID,long DAMode,double CH0Volt,double CH1Volt,double CH2Volt,double CH3Volt,double CH4Volt,double CH5Volt,double CH6Volt,double CH7Volt);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 	PLX_API   long  WCDPcieDA_ConfigureBegin(long DeviceID,unsigned int ChipID);
	PLX_API   long  WCDPcieDA_ConfigureParameter(long DeviceID,unsigned int ChipID);
 	PLX_API   long  WCDPcieDA_ConfigureEnd(long DeviceID,unsigned int ChipID);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PLX_API   PVOID* WCDPcieAD_GetRdbuffer(long DeviceID);
	PLX_API   long   WCDPcieAD_GetCurrentRdPoint(long DeviceID);

	PLX_API   long WCDPcie_GetPcieInfo(long DeviceID,PCIEINFO *PcieInfo);


}
//#endif
