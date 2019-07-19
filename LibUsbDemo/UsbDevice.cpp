#include "pch.h"
#include "UsbDevice.h"
#include "NoloDevUtils.h"

using namespace std;


UsbDevice::UsbDevice()
{
	for (int i = 0;i< ReadPointSize; i++)
	{
		m_ReadBuffer[i] = new UCHAR[UsbPackSize];
	}
	m_WriteBuffer = new UCHAR[UsbPackSize];
}


UsbDevice::~UsbDevice()
{	
	m_bDevConnected = false;
	m_bRunning = false;
	CloseUsbHandle();
	ResetTransfer();
	//结束
	libusb_exit(nullptr);

	for (int i = 0; i < ReadPointSize; i++)
	{
		SafeDelete(m_ReadBuffer[i]);
	}
	SafeDelete(m_WriteBuffer);

}


bool UsbDevice::InitLibUsb()
{
	//获取库版本号
	const libusb_version *LibVersion = libusb_get_version();
	string strDesc = LibVersion->describe;
	//初始化
	int Res = libusb_init(nullptr);
	if (Res != 0)
	{
		//初始化USB失败
		printf("Can not Init LibUsb\n");
		return false;
	}
	m_bRunning = true;
	thread SearchingThread = thread(&UsbDevice::UsbDevWorks,this);
	SearchingThread.detach();

	return true;
}

void UsbDevice::StartNoloDevice()
{

}

void UsbDevice::TrigerHapiticPause()
{
	UCHAR buffer[64] = { 0xC0, 0x66, 0x54, 0x45, 0x02, 0x02
		,0x66, 0x54, 0x45, 0x02, 0x02
		,0x66, 0x34, 0x25, 0x01, 0x03
	};
	HighPrecisionTimer::Global()->RecordNow();
	SendData(buffer, 64);
}

bool UsbDevice::SendData(UCHAR * pData,int len)
{
	if (m_OpenedHandle == nullptr)
	{
		return false;
	}
	if (len > UsbPackSize)
	{
		return false;
	}

	//UCHAR *Buffer = m_WriteBuffer;
	memcpy(m_WriteBuffer, pData, len);
	libusb_submit_transfer(m_WriteTransfer);

	return true;
}

void UsbDevice::SetNewDataCallBack(pfn_UsbNewDta_cb callback, void * Context)
{
	m_NewDataCb = callback;
	m_CallBackContex = Context;
}


void UsbDevice::CloseUsbHandle()
{
	if (m_OpenedHandle != nullptr)
	{
		libusb_close(m_OpenedHandle);
		m_OpenedHandle = nullptr;
	}
}

void UsbDevice::ResetTransfer()
{
	for (int i = 0; i < ReadPointSize; i++)
	{
		if (mReadTransfer[i] != nullptr)
		{
			libusb_free_transfer(mReadTransfer[i]);
			mReadTransfer[i] = nullptr;
		}
	}
	if (m_WriteTransfer != nullptr)
	{
		libusb_free_transfer(m_WriteTransfer);
		m_WriteTransfer = nullptr;
	}
	m_DevPID = 0;
}


static HighPrecisionTimer TheTimer;

void UsbDevice::UsbDevWorks()
{
	UCHAR buffer[64];
	double timeMs;
	while (m_bRunning)
	{
		if (m_bDevConnected)
		{
			//LibUsb Engin loop
			if (libusb_handle_events(NULL) != LIBUSB_SUCCESS)
			{
				printf("Engin loop error");
				break;
			}

			//SyncRead
			//bool Res = SyncReadData(buffer);
			//if (Res)
			//{
			//	timeMs = TheTimer.GetTimeMsSinceLastRecord(true);
			//	//printf("SyncRead Data OK:%X Time:%.5lf\n", transfer->buffer[0], timeMs);
			//	printf("NewData: T:%.5lf \n ",  timeMs);
			//}
			//else
			//{
			//	printf("Read Fail\n");
			//}
		}
		else
		{
			//Searching
			SerchingDevice();
			this_thread::sleep_for(chrono::microseconds(50));
		}
	}
}


void UsbDevice::SendDataWork()
{
	return;
	UCHAR Sendbuffer[64] = {};
	for (int i = 0;i<64; i++)
	{
		Sendbuffer[i] = (UCHAR)i;
	}
	auto TimeNow = chrono::system_clock::now();
	static int i = 0;
	while (true)
	{
		SendData(Sendbuffer,64);
		TheTimer.RecordNow();
		this_thread::sleep_until(TimeNow + chrono::milliseconds(8 * i));
		i++;
	}
}

void UsbDevice::SerchingDevice()
{
	//如果已经打开，先关闭
	ResetTransfer();
	CloseUsbHandle();
	int Res = LIBUSB_SUCCESS;
	libusb_device **DevList = nullptr;
	shared_ptr<libusb_device_descriptor> pDestciptor = make_shared<libusb_device_descriptor>();
	ssize_t Count = libusb_get_device_list(nullptr, &DevList);
	if (Count > 1)
	{
		libusb_device *TempDev = nullptr;
		for (int i = 0; (TempDev = DevList[i]) != nullptr; i++)
		{
			int Res = libusb_get_device_descriptor(TempDev, pDestciptor.get());
			if (Res < 0)
			{
				printf("Can not get descriptor\n");
				continue;
			}
			if (pDestciptor->idVendor != (uint16_t)0x28E9)
			{
				//Not Watch other Vendor
				continue;
			}
			//shared_ptr<OpenedDevice> OpenDev = make_shared<OpenedDevice>();
			if (   pDestciptor->idProduct == (uint16_t)0x028A
				|| pDestciptor->idProduct == (uint16_t)0x0301
				|| pDestciptor->idProduct == (uint16_t)0x0302
				|| pDestciptor->idProduct == (uint16_t)0x0303
				)
			{
				Res = libusb_open(TempDev, &m_OpenedHandle);
				if (Res == LIBUSB_SUCCESS)
				{
					Res = InitTransfer();
					if (Res == LIBUSB_SUCCESS)
					{
						m_bDevConnected = true;
						printf("Serching Device:0x%X  Opened\n", (UINT)m_OpenedHandle);
						//thread theSendWork = thread(&UsbDevice::SendDataWork, this);
						//theSendWork.detach();
					}
					m_DevPID = pDestciptor->idProduct;
					break;
				}
				else
				{
					printf("Can not Open Device:%X Error:%d\n", pDestciptor->idProduct, Res);
				}
			}
		}//End of Forloop
	}
	libusb_free_device_list(DevList, 1);
}

int UsbDevice::InitTransfer()
{
	if (m_OpenedHandle == nullptr)
	{
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	//ResetTransfer();

	libusb_device *TheDev = libusb_get_device(m_OpenedHandle);
	//LIBUSB_SPEED_FULL == 2
	//int speed = libusb_get_device_speed(TempDev);
	//printf("Usb speed is:%d\n", speed);
	//获取端点信息
	libusb_config_descriptor *ConfigDes = nullptr;
	UCHAR SendEndPoint = 0x02;
	const UCHAR ReceivePoint = 0x82;
	int Res = libusb_get_active_config_descriptor(TheDev, &ConfigDes);
	if (Res != LIBUSB_SUCCESS)
	{
		printf("Get ConfigError:%d\n", Res);
		return Res;
	}
	//InterFaces 遍历
	int numberInterFace = ConfigDes->bNumInterfaces;
	int tempPoint = 0x81;
	for (int i = 0; i < numberInterFace; i++)
	{	
		Res = libusb_claim_interface(m_OpenedHandle, i);
		if (Res == LIBUSB_SUCCESS) {
			printf("libusb_claim_interface Index:%d Addr:%X OK\n", i,(0x81+i));
		}
		else
		{
			printf("Can not libusb_claim_interfac:%d Error:%d\n", (0x81 + i), Res);
			goto InitTransferEnd;
		}
		const libusb_interface &tempInterFace = ConfigDes->interface[i];
		mReadTransfer[i] = libusb_alloc_transfer(0);
		tempPoint = (numberInterFace == 1)? ReceivePoint : (0x81 + i);
		libusb_fill_interrupt_transfer(mReadTransfer[i], m_OpenedHandle, tempPoint,
			m_ReadBuffer[i], UsbPackSize, &UsbDevice::StaticOnNewCallBack, this, 1000);

		Res = libusb_submit_transfer(mReadTransfer[i]);
		if (Res == LIBUSB_SUCCESS)
		{
			printf("First SubmitTransfer:%d OK\n",i);
		}
		else
		{
			printf("Frist SubmitTransfer Fail:%d\n", Res);
			goto InitTransferEnd;
		}
		NoloUtils::printInterFace(tempInterFace);//打印
	}
	m_WriteTransfer = libusb_alloc_transfer(0); 
	//SendEndPoint = (numberInterFace == 1) ? 0x01 : 0x02;
	libusb_fill_interrupt_transfer(m_WriteTransfer, m_OpenedHandle, SendEndPoint,m_WriteBuffer,
		UsbPackSize, &UsbDevice::StaticSendCallBack, this, 1000);

InitTransferEnd:

	libusb_free_config_descriptor(ConfigDes);

	return Res;
}

void UsbDevice::OnNewData(libusb_transfer * transfer)
{
	UCHAR *tempBuffer = nullptr;
	static int PrintCount = 0;
	double timeMs = 0.0;
	switch (transfer->status)
	{
	case LIBUSB_TRANSFER_COMPLETED:
		// Success here, data transfered are inside
		tempBuffer = transfer->buffer;
		timeMs = TheTimer.GetTimeMsSinceLastRecord(true);
		//printf("SyncRead Data OK:%X Time:%.5lf\n", transfer->buffer[0], timeMs);
		//printf("NewData:%X T:%.5lf \n ", transfer->buffer[0], timeMs);

		if (m_NewDataCb)
		{
			m_NewDataCb(transfer->buffer, m_CallBackContex);
		}

		libusb_submit_transfer(transfer);
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		//超时则 重新请求
		libusb_submit_transfer(transfer);
		//printf("Time out Submit Data\n");
		break;
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		printf("OnNewUsbData Read_ERROR:%d\n", transfer->status);
		m_bDevConnected = false;
		// Various type of errors here
		break;
	default:
		printf("OnNewUsbData OtherDevice Status\n");
	}
}

void UsbDevice::StaticOnNewCallBack(libusb_transfer * transfer)
{
	if (transfer->user_data != nullptr)
	{
		((UsbDevice *)(transfer->user_data))->OnNewData(transfer);
	}
}

void UsbDevice::StaticSendCallBack(libusb_transfer * transfer)
{
	switch (transfer->status)
	{
	case LIBUSB_TRANSFER_COMPLETED:
		// Success here, data transfered are inside 
		//printf("NewUsbData Comming:\n");
		printf("\nSend Data OK:%X\n", transfer->buffer[0]);
		break;
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		printf("OnNewUsbData LIBUSB_TRANSFER_ERROR\n");
		// Various type of errors here
		break;
	default:
		printf("OnNewUsbData OtherDevice Status\n");
	}

}



