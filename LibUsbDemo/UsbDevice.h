#pragma once

#include <libusb.h>

#define UsbPackSize 64

class OpenedDevice
{
public:
	OpenedDevice();
	~OpenedDevice();
public:
	libusb_device_handle *mHandle = nullptr;
	libusb_transfer *mTransfer = nullptr;
	UCHAR *buffer = nullptr;
};

enum ETransferType
{
	eBulkRead = 0,
	eBulkWrite,
	eBulkReadWrite,
};

class UsbDevice
{
public:
	UsbDevice();
	~UsbDevice();
	bool InitLibUsb();
	void ReleaseLibUsb();
public:
	void StartNoloDevice();
private:
	void UsbDevWorks();
	void SerchingDevice();
	void CloseAllHandles();
	//void ListenHandle(libusb_device_handle *DevHandle);
	void InitTransfer(std::shared_ptr<OpenedDevice> pOpenedDev, ETransferType type);

	static void OnNewUsbData(struct libusb_transfer *transfer);
private:
	//libusb_hotplug_callback_handle m_Plug_CBHandle;
	//libusb_transfer *m_Rtransfer;
	bool m_bRunning = false;
	bool m_bDevConnected = false;
	std::vector<std::shared_ptr<OpenedDevice>> m_OpenedDevHandles;
};

