#pragma once

#include <libusb.h>

#define UsbPackSize 64

enum ETransferType
{
	eUsbRead = 0,
	eUsbWrite,
	eBulkReadWrite,
};

class OpenedDevice
{
public:
	OpenedDevice();
	~OpenedDevice();
public:
	ETransferType m_TransferType = ETransferType::eUsbRead;
	libusb_device_handle *mHandle = nullptr;
	//Receive
	libusb_transfer *mTransfer = nullptr;
	UCHAR *buffer = nullptr;
	//Send to Device
	libusb_transfer *mTransferToDev = nullptr;
	UCHAR *bufferToDev = nullptr;

};


enum ENoloDevVidPidType
{
	eAllNoloDevType = 0,
	eNoloIDHead,
	eNoloIDBase,
	eNoloIDControl,
	eOhterType
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
	void TrigerHapiticPause();
	bool SendData(UCHAR *pData, int len);
private:
	void UsbDevWorks();
	void SerchingDevice();
	void CloseAllHandles();
	//void ListenHandle(libusb_device_handle *DevHandle);
	int InitTransfer(std::shared_ptr<OpenedDevice> pOpenedDev, ETransferType type);
	void OnNewData(struct libusb_transfer *transfer);
	static void StaticOnNewCallBack(struct libusb_transfer *transfer);
	static void StaticSendCallBack(struct libusb_transfer *transfer);

private:
	void SyncReadData();

private:
	//libusb_hotplug_callback_handle m_Plug_CBHandle;
	//libusb_transfer *m_Rtransfer;
	bool m_bRunning = false;
	bool m_bDevConnected = false;
	std::vector<std::shared_ptr<OpenedDevice>> m_DeviceList;
};

