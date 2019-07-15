#include "pch.h"
#include "UsbDevice.h"
#include "NoloDevUtils.h"

using namespace std;


OpenedDevice::OpenedDevice()
{
	buffer = new UCHAR[UsbPackSize];
	bufferToDev = new UCHAR[UsbPackSize];
}

OpenedDevice::~OpenedDevice()
{
	SafeDelete(buffer);
	SafeDelete(bufferToDev);
	if (mTransfer)
	{
		libusb_free_transfer(mTransfer);
	}
	if (mTransfer)
	{
		libusb_free_transfer(mTransferToDev);
	}
	if (mTransfer)
	{
		libusb_close(mHandle);
	}
}


UsbDevice::UsbDevice()
{
}


UsbDevice::~UsbDevice()
{
	ReleaseLibUsb();
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

void UsbDevice::ReleaseLibUsb()
{
	//清空所有已经打开的设备
	CloseAllHandles();
	//结束
	libusb_exit(nullptr);
}


void UsbDevice::StartNoloDevice()
{

}

void UsbDevice::TrigerHapiticPause()
{
	UCHAR buffer[64] = { 0xAA, 0x66, 0x54, 0x45, 0x00 };
	for (int i = 0; i < 100; i++)
	{
		SendData(buffer, 64);
		Sleep(10);
	}
}

bool UsbDevice::SendData(UCHAR * pData,int len)
{
	auto it = m_DeviceList.begin();
	if (it == m_DeviceList.end())
	{
		return false;
	}
	libusb_device_handle *pHandle = (*it)->mHandle;
	if (pHandle == nullptr)
	{
		return false;
	}

	if (len > UsbPackSize)
	{
		return false;
	}

	UCHAR *Buffer = (*it)->bufferToDev;
	memcpy(Buffer, pData, len);
	libusb_submit_transfer((*it)->mTransferToDev);

	return true;
}


void UsbDevice::SyncReadData()
{
	auto it = m_DeviceList.begin();
	if (it != m_DeviceList.end())
	{
		//InitTransfer(*it, ETransferType::eBulkRead);
		libusb_device_handle *TempHandle = (*it)->mHandle;
		int ReadCount = 0;
		//int Res = libusb_interrupt_transfer(TempHandle,1, (*it)->buffer, UsbPackSize, &ReadCount,100);
		//int Res = libusb_bulk_transfer(TempHandle,1, (*it)->buffer, UsbPackSize, &ReadCount,100);
		int Res = libusb_interrupt_transfer(TempHandle, 0x82, (*it)->buffer, UsbPackSize, &ReadCount,100);
		if (Res == LIBUSB_SUCCESS)
		{
			printf("SyncRead Data OK:%X\n", (*it)->buffer[0]);
		}
		else
		{
			printf("SyncRead Data Fail:%d\n", Res);
		}
	}
}

void UsbDevice::UsbDevWorks()
{
	while (m_bRunning)
	{
		if (m_bDevConnected)
		{
			int Res = libusb_handle_events(NULL);
			//LibUsb Engin loop
			if (Res != LIBUSB_SUCCESS)
			{
				printf("Engin loop");
				break;
			}
		}
		else
		{
			//Searching
			SerchingDevice();
			this_thread::sleep_for(chrono::microseconds(50));
		}
		
	}
}

void UsbDevice::SerchingDevice()
{
	CloseAllHandles();
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
			//LIBUSB_SPEED_FULL == 2
			//int speed = libusb_get_device_speed(TempDev);
			//printf("Usb speed is:%d\n", speed);
			//Descriptor
			libusb_config_descriptor *ConfigDes = nullptr;
			Res = libusb_get_active_config_descriptor(TempDev, &ConfigDes);
			if (Res != LIBUSB_SUCCESS)
			{
				printf("Get ConfigError:%d\n", Res);
				goto SearchEnd;
			}
			//打印InterFaces
			int numberInterFace = ConfigDes->bNumInterfaces;
			for (int i = 0; i < numberInterFace; i++)
			{
				const libusb_interface &tempInterFace = ConfigDes->interface[i];
				NoloUtils::printInterFace(tempInterFace);
			}
			libusb_free_config_descriptor(ConfigDes);

			shared_ptr<OpenedDevice> OpenDev = make_shared<OpenedDevice>();
			if (   pDestciptor->idProduct == (uint16_t)0x028A
				|| pDestciptor->idProduct == (uint16_t)0x0301
				|| pDestciptor->idProduct == (uint16_t)0x0302
				|| pDestciptor->idProduct == (uint16_t)0x0303
				)
			{
				Res = libusb_open(TempDev, &OpenDev->mHandle);
				if (Res == LIBUSB_SUCCESS)
				{
					printf("Add Dev PID:%X\n", pDestciptor->idProduct);
					m_DeviceList.push_back(OpenDev);
				}
				else
				{
					printf("Can not Open Device:%X Error:%d\n", pDestciptor->idProduct, Res);
				}
			}
		}
	}
SearchEnd:
	libusb_free_device_list(DevList,1);
	size_t OpenCount = m_DeviceList.size();
	if (OpenCount > 0)
	{
		printf("SerchingDevice Opened %zd Device\n", OpenCount);
	}
	auto it = m_DeviceList.begin();
	if (it != m_DeviceList.end())
	{
		int Res = InitTransfer(*it,ETransferType::eUsbRead);
		if (Res == LIBUSB_SUCCESS)
		{
			m_bDevConnected = true;
		}
	}
}

void UsbDevice::CloseAllHandles()
{
	m_DeviceList.clear();
}

int UsbDevice::InitTransfer(shared_ptr<OpenedDevice> pOpenedDev, ETransferType type)
{
	if (!pOpenedDev || pOpenedDev->mHandle == nullptr)
	{
		return LIBUSB_ERROR_INVALID_PARAM;
	}
	pOpenedDev->mTransfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(pOpenedDev->mTransfer, pOpenedDev->mHandle,0x82,
		pOpenedDev->buffer, UsbPackSize, &UsbDevice::StaticOnNewCallBack,this,100);

	pOpenedDev->mTransferToDev = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(pOpenedDev->mTransferToDev, pOpenedDev->mHandle, 0x01,
		pOpenedDev->bufferToDev, UsbPackSize, &UsbDevice::StaticSendCallBack, this, 100);

	int Res = libusb_claim_interface(pOpenedDev->mHandle,0);
	if (Res == LIBUSB_SUCCESS)
	{
		printf("libusb_claim_interface OK\n");
	}
	else
	{
		printf("libusb_claim_interface Fail:%d\n", Res);
		return Res;
	}
	Res = libusb_submit_transfer(pOpenedDev->mTransfer);
	if (Res == LIBUSB_SUCCESS)
	{
		printf("First SubmitTransfer OK\n");
	}
	else
	{
		printf("Frist SubmitTransfer Fail:%d\n", Res);
		return Res;
	}
	return LIBUSB_SUCCESS;
}

void UsbDevice::OnNewData(libusb_transfer * transfer)
{
	UCHAR *tempBuffer = nullptr;
	static int PrintCount = 0;
	switch (transfer->status)
	{
	case LIBUSB_TRANSFER_COMPLETED:
		// Success here, data transfered are inside
		tempBuffer = transfer->buffer;
		if (PrintCount++ % 61 == 0)
		{
			printf("SyncRead Data OK:%X\n", transfer->buffer[0]);
		}
		libusb_submit_transfer(transfer);
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		//超时则 重新请求
		libusb_submit_transfer(transfer);
		break;
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_OVERFLOW:
		printf("OnNewUsbData Read_ERROR:%d\n", transfer->status);
		m_bDevConnected = false;
		m_DeviceList.clear();
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
		printf("Send Data OK:%X\n", transfer->buffer[0]);
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



