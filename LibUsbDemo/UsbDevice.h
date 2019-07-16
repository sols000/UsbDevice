#pragma once

#include <libusb.h>
#include <HighPrecisionTimer.h>

#define UsbPackSize 64
#define ReadPointSize 3

enum ETransferType
{
	eUsbRead = 0,
	eUsbWrite,
	eBulkReadWrite,
};


class UsbDevice
{
public:
	UsbDevice();
	~UsbDevice();
	bool InitLibUsb();
public:
	void StartNoloDevice();
	void TrigerHapiticPause();
	bool SendData(UCHAR *pData, int len);

private:
	void CloseUsbHandle();
	void ResetTransfer();
	void UsbDevWorks();
	void SerchingDevice();
	//void ListenHandle(libusb_device_handle *DevHandle);
	int InitTransfer();
	void OnNewData(struct libusb_transfer *transfer);
	static void StaticOnNewCallBack(struct libusb_transfer *transfer);
	static void StaticSendCallBack(struct libusb_transfer *transfer);

private:
	//void SyncReadData();

private:
	//libusb_hotplug_callback_handle m_Plug_CBHandle;
	//libusb_transfer *m_Rtransfer;
	bool m_bRunning = false;
	bool m_bDevConnected = false;
	//OpenedDevice 
	libusb_device_handle *m_OpenedHandle = nullptr;
	uint16_t m_DevPID = 0;
	//Receive
	libusb_transfer *mReadTransfer[3] = { nullptr };
	UCHAR *m_ReadBuffer[3] = { nullptr };
	//Send to Device
	libusb_transfer *m_WriteTransfer = nullptr;
	UCHAR *m_WriteBuffer = nullptr;


};

