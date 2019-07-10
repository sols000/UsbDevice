#include "pch.h"
#include "UsbDevice.h"

using namespace std;

UsbDevice::UsbDevice()
{
}


UsbDevice::~UsbDevice()
{
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
	int rc = 0;
	libusb_init(nullptr);

	int Res = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);

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


void UsbDevice::StartNoloDevice()
{
}
