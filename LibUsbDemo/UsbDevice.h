#pragma once

#include <libusb.h>

class UsbDevice
{
public:
	UsbDevice();
	~UsbDevice();
	bool InitLibUsb();
public:
	void StartNoloDevice();

private:
	libusb_hotplug_callback_handle m_Plug_CBHandle;
};

