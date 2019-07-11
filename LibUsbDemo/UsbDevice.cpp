#include "pch.h"
#include "UsbDevice.h"

using namespace std;


OpenedDevice::OpenedDevice()
{
	buffer = new UCHAR[UsbPackSize];
}

OpenedDevice::~OpenedDevice()
{
	delete buffer;
}


UsbDevice::UsbDevice()
{
}


UsbDevice::~UsbDevice()
{
	ReleaseLibUsb();
}


static int countUsb = 0;
int hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
	libusb_hotplug_event event, void *user_data) {
	static libusb_device_handle *dev_handle = NULL;
	struct libusb_device_descriptor desc;
	int rc;
	(void)libusb_get_device_descriptor(dev, &desc);
	if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
		rc = libusb_open(dev, &dev_handle);
		if (LIBUSB_SUCCESS != rc) {
			printf("Could not open USB device\n");
		}
	}
	else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
		if (dev_handle) {
			libusb_close(dev_handle);
			dev_handle = NULL;
		}
	}
	else {
		printf("Unhandled event %d\n", event);
	}
	countUsb++;
	return 0;
}

bool UsbDevice::InitLibUsb()
{	
	//获取库版本号
	const libusb_version *LibVersion = libusb_get_version();
	string strDesc = LibVersion->describe;
	//初始化
	int Res = 0;
	Res = libusb_init(nullptr);
	if (Res != 0)
	{
		//初始化USB失败
		printf("Can not Init LibUsb\n");
		return false;
	}

	Res = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);
	if (Res == 0)
	{
		printf("Not Suport Hotplug\n");
	}
	else
	{
		printf("Hotplug is Suported\n");
	}
	//m_Rtransfer = libusb_alloc_transfer(0);

	m_bRunning = true;
	thread SearchingThread = thread(&UsbDevice::UsbDevWorks,this);
	SearchingThread.detach();

	//libusb_hotplug_event eventType = libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT);
	//rc = libusb_hotplug_register_callback(nullptr, eventType, LIBUSB_HOTPLUG_NO_FLAGS
	//	, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY
	//	, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, nullptr, &m_Plug_CBHandle);

	//if (LIBUSB_SUCCESS != rc) {
	//	printf("Error creating a hotplug callback\n");
	//	libusb_exit(NULL);
	//	return EXIT_FAILURE;
	//}
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

void UsbDevice::UsbDevWorks()
{
	while (m_bRunning)
	{
		if (m_bDevConnected)
		{
			//Reset ReadWrite
			printf("Pridict I am ReadWrite Data\n");
			this_thread::sleep_for(chrono::seconds(1));
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


			if (pDestciptor->idVendor == (uint16_t)0x28E9)
			{
				libusb_config_descriptor *ConfigDes = nullptr;
				libusb_get_active_config_descriptor(TempDev, &ConfigDes);
				shared_ptr<OpenedDevice> OpenDev = make_shared<OpenedDevice>();
				if (pDestciptor->idProduct == (uint16_t)0x028A)
				{
					Res = libusb_open(TempDev, &OpenDev->mHandle);
					if (Res == 0)
					{
						m_OpenedDevHandles.push_back(OpenDev);
					}
					else
					{
						printf("Can not Open (uint16_t)0x028A\n");
					}
				}
			}
			else
			{
				//Other Device
				continue;
			}
		}
	}
	size_t OpenCount = m_OpenedDevHandles.size();
	if (OpenCount > 0)
	{
		printf("SerchingDevice Opened %d Device\n", OpenCount);
		m_bDevConnected = true;
	}
	auto it = m_OpenedDevHandles.begin();
	if (it != m_OpenedDevHandles.end())
	{
		InitTransfer(*it,ETransferType::eBulkRead);
	}
}

void UsbDevice::CloseAllHandles()
{
	//for(auto Devive : m_OpenedDevHandles)
	//{
	//	libusb_close(Devive);
	//}
	m_OpenedDevHandles.clear();
}

void UsbDevice::InitTransfer(shared_ptr<OpenedDevice> pOpenedDev, ETransferType type)
{
	if (!pOpenedDev || pOpenedDev->mHandle == nullptr)
	{
		return;
	}
	pOpenedDev->mTransfer = libusb_alloc_transfer(0);
	int Res = libusb_claim_interface(pOpenedDev->mHandle,0);
	if (Res == 0)
	{
		printf("libusb_claim_interface OK\n");
	}
	else
	{
		printf("libusb_claim_interface Fail:%d\n", Res);
	}

	libusb_fill_interrupt_transfer(pOpenedDev->mTransfer, pOpenedDev->mHandle,(UCHAR)1,
		pOpenedDev->buffer,64, &UsbDevice::OnNewUsbData,nullptr,0);
	Res = libusb_submit_transfer(pOpenedDev->mTransfer);
	if (Res == LIBUSB_SUCCESS)
	{
		printf("SubmitTransfer OK\n");
	}
	else
	{
		printf("SubmitTransfer Fail:%d\n", Res);
	}
}

void UsbDevice::OnNewUsbData(libusb_transfer * transfer)
{
	printf("New Usb data Arrived\n");
}


