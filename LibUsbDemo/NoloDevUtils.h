#pragma once


#define SafeDelete(p) if(p!=nullptr){delete p;p=nullptr;}
namespace NoloUtils
{

void printInterFace(const libusb_interface &UsbInterFace)
{
	printf("Interface  InterfaceNum:%X\n", UsbInterFace.altsetting->bInterfaceNumber);
	int EndPortNumber = UsbInterFace.altsetting->bNumEndpoints;
	for (int j = 0; j < EndPortNumber; j++)
	{
		//LIBUSB_ENDPOINT_OUT
		const libusb_endpoint_descriptor &Endpoint = UsbInterFace.altsetting->endpoint[j];
		printf("   Endpoint:%d Addr:%X,Attr:%X,PackSize:%d,Int:%d\n", j, Endpoint.bEndpointAddress
			, Endpoint.bmAttributes, Endpoint.wMaxPacketSize, Endpoint.bInterval);
	}
	printf("\n");
}

}

