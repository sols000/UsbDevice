// LibUsbDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <libusb.h>
#include "UsbDevice.h"

#pragma comment(lib,"libusb-1.0.lib")

using namespace std;

static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0, j = 0;
	uint8_t path[8];

	libusb_device_descriptor *pDestciptor = new libusb_device_descriptor;
	libusb_config_descriptor *PConifig = new libusb_config_descriptor;
	while ((dev = devs[i++]) != NULL) {
		

		//struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, pDestciptor);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}
		r = libusb_get_active_config_descriptor(dev, &PConifig);

		printf("%04x:%04x (bus %d, device %d)",
			pDestciptor->idVendor, pDestciptor->idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));

		r = libusb_get_port_numbers(dev, path, sizeof(path));
		if (r > 0) {
			printf(" path: %d", path[0]);
			for (j = 1; j < r; j++)
				printf(".%d", path[j]);
		}
		printf("\n");
	}
}

int main()
{
    std::cout << "Hello libusb!\n";

	shared_ptr<UsbDevice> TheDev = make_shared<UsbDevice>();

	TheDev->InitLibUsb();

	//libusb_device **devs;
	//int r;
	//ssize_t cnt;
	//r = libusb_init(NULL);
	//if (r < 0)
	//{
	//	return r;
	//}
	//cnt = libusb_get_device_list(NULL, &devs);
	//if (cnt < 0)
	//{
	//	return (int)cnt;
	//}
	//print_devs(devs);
	//libusb_free_device_list(devs, 1);

	//libusb_exit(NULL);

	getchar();
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
