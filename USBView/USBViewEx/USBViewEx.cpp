// USBViewEx.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
using namespace std;

#include "usbioctl.h"


const int MAX_HCD_COUNT = 10;

BOOL g_bDoConfigDesc = TRUE;
int g_nTotalHubs = 0;
ULONG g_ulTotalDevicesConnected = 0;

void EnumerateHubPorts(HANDLE hHubDevice, ULONG NumPorts, int nLevel);

std::string WideStrToMultiStr(LPCWSTR WideStr)
{
	// Get the length of the converted string
	//
	ULONG nBytes = WideCharToMultiByte(CP_ACP, 0, WideStr, -1, NULL, 0, NULL, NULL);
	if (nBytes == 0)
	{
		return NULL;
	}

	// Allocate space to hold the converted string
	//
	PTSTR MultiStr = (PTSTR)malloc(nBytes);
	if (MultiStr == NULL)
	{
		return NULL;
	}

	// Convert the string
	//
	nBytes = WideCharToMultiByte(CP_ACP, 0, WideStr, -1, MultiStr, nBytes, NULL, NULL);
	if (nBytes == 0)
	{
		free(MultiStr);
		return NULL;
	}

	std::string strMulti = MultiStr;
	free(MultiStr);

	return strMulti;
}

const std::string GetDriverKeyName(HANDLE hDev, ULONG ConnectionIndex)
{
	USB_NODE_CONNECTION_DRIVERKEY_NAME  driverKeyName;
	PUSB_NODE_CONNECTION_DRIVERKEY_NAME driverKeyNameW = NULL;

	// Get the length of the name of the driver key of the device attached to
	// the specified port.
	//
	driverKeyName.ConnectionIndex = ConnectionIndex;
	
	ULONG nBytes = 0;
	BOOL success = DeviceIoControl(hDev,
		IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
		&driverKeyName,
		sizeof(driverKeyName),
		&driverKeyName,
		sizeof(driverKeyName),
		&nBytes,
		NULL);
	if (!success)
	{
		return "";
	}

	// Allocate space to hold the driver key name
	//
	nBytes = driverKeyName.ActualLength;
	if (nBytes <= sizeof(driverKeyName))
	{
		return "";
	}

	driverKeyNameW = (PUSB_NODE_CONNECTION_DRIVERKEY_NAME)new BYTE[nBytes];

	if (driverKeyNameW == NULL)
	{
		SAFE_DELETE_ARRAY(driverKeyNameW);
		return "";
	}

	// Get the name of the driver key of the device attached to
	// the specified port.
	//
	driverKeyNameW->ConnectionIndex = ConnectionIndex;

	success = DeviceIoControl(hDev,
		IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
		driverKeyNameW,
		nBytes,
		driverKeyNameW,
		nBytes,
		&nBytes,
		NULL);

	if (!success)
	{
		SAFE_DELETE_ARRAY(driverKeyNameW);
		return "";
	}

	// Convert the driver key name
	//
	std::string driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName);
	SAFE_DELETE_ARRAY(driverKeyNameW);

	return driverKeyNameA;
}


const std::string GetHCDDriverKeyName(HANDLE hDev)
{
	USB_HCD_DRIVERKEY_NAME  driverKeyName;
	PUSB_HCD_DRIVERKEY_NAME driverKeyNameW = NULL;

	// Get the length of the name of the driver key of the device attached to
	// the specified port.
	//

	ULONG nBytes = 0;
	BOOL success = DeviceIoControl(hDev,
		IOCTL_GET_HCD_DRIVERKEY_NAME,
		&driverKeyName,
		sizeof(driverKeyName),
		&driverKeyName,
		sizeof(driverKeyName),
		&nBytes,
		NULL);
	if (!success)
	{
		return "";
	}

	// Allocate space to hold the driver key name
	//
	nBytes = driverKeyName.ActualLength;
	if (nBytes <= sizeof(driverKeyName))
	{
		return "";
	}

	driverKeyNameW = (PUSB_HCD_DRIVERKEY_NAME)new BYTE[nBytes];

	if (driverKeyNameW == NULL)
	{
		SAFE_DELETE_ARRAY(driverKeyNameW);
		return "";
	}

	// Get the name of the driver key of the device attached to
	// the specified port.
	//
	success = DeviceIoControl(hDev,
		IOCTL_GET_HCD_DRIVERKEY_NAME,
		driverKeyNameW,
		nBytes,
		driverKeyNameW,
		nBytes,
		&nBytes,
		NULL);

	if (!success)
	{
		SAFE_DELETE_ARRAY(driverKeyNameW);
		return "";
	}

	// Convert the driver key name
	//
	std::string driverKeyNameA = WideStrToMultiStr(driverKeyNameW->DriverKeyName);
	SAFE_DELETE_ARRAY(driverKeyNameW);

	return driverKeyNameA;
}


const std::string DriverNameToDeviceDesc(std::string DriverName, BOOL DeviceId = FALSE)
{
	DEVINST     devInst = {0};
	DEVINST     devInstNext = {0};
	CONFIGRET   cr = {0};
	ULONG       walkDone = 0;
	ULONG       len = 0;
	char		buf[1024] = {0};

	// Get Root DevNode
	//
	cr = CM_Locate_DevNode(&devInst, NULL, 0);
	if (cr != CR_SUCCESS)
	{
		return "";
	}

	// Do a depth first search for the DevNode with a matching
	// DriverName value
	//
	while (!walkDone)
	{
		// Get the DriverName value
		//
		len = sizeof(buf);
		cr = CM_Get_DevNode_Registry_Property(devInst,
			CM_DRP_DRIVER,  //\Device\USBPDO-5 
			NULL,
			buf,
			&len,
			0);

		// If the DriverName value matches, return the DeviceDescription
		//
		if (cr == CR_SUCCESS && (DriverName.compare(buf) == 0))
		{
			len = sizeof(buf);

			if (DeviceId)
			{
				cr = CM_Get_Device_ID(devInst, buf, len, 0);
			}
			else
			{
				cr = CM_Get_DevNode_Registry_Property(devInst,
					CM_DRP_DEVICEDESC,  //USB Mass Storage Device 
					NULL,
					buf,
					&len,
					0);
			}

			if (cr == CR_SUCCESS)
			{
				return buf;
			}
			else
			{
				return "";
			}
		}

		// This DevNode didn't match, go down a level to the first child.
		//
		cr = CM_Get_Child(&devInstNext, devInst, 0);

		if (cr == CR_SUCCESS)
		{
			devInst = devInstNext;
			continue;
		}

		// Can't go down any further, go across to the next sibling.  If
		// there are no more siblings, go back up until there is a sibling.
		// If we can't go up any further, we're back at the root and we're
		// done.
		//
		for (;;)
		{
			cr = CM_Get_Sibling(&devInstNext,devInst,0);
			if (cr == CR_SUCCESS)
			{
				devInst = devInstNext;
				break;
			}

			cr = CM_Get_Parent(&devInstNext,devInst,0);
			if (cr == CR_SUCCESS)
			{
				devInst = devInstNext;
			}
			else
			{
				walkDone = 1;
				break;
			}
		}
	}

	return "";
}


const std::string GetRootHubName(HANDLE HostController)
{
	// Get the length of the name of the Root Hub attached to the
	// Host Controller
	//	
	USB_ROOT_HUB_NAME   rootHubName;
	ULONG nBytes = 0;
	BOOL success = DeviceIoControl(HostController,
		IOCTL_USB_GET_ROOT_HUB_NAME,
		0,
		0,
		&rootHubName,
		sizeof(rootHubName),
		&nBytes,
		NULL);
	if (!success)
	{
		return "";
	}

	// Allocate space to hold the Root Hub name
	//
	nBytes = rootHubName.ActualLength;

	PUSB_ROOT_HUB_NAME rootHubNameW = (PUSB_ROOT_HUB_NAME)new BYTE[nBytes];

	// Get the name of the Root Hub attached to the Host Controller
	//
	success = DeviceIoControl(HostController,
		IOCTL_USB_GET_ROOT_HUB_NAME,
		NULL,
		0,
		rootHubNameW,
		nBytes,
		&nBytes,
		NULL);

	if (!success)
	{
		SAFE_DELETE(rootHubNameW);
		return "";
	}

	// Convert the Root Hub name
	//
	string rootHubNameA = WideStrToMultiStr(rootHubNameW->RootHubName);

	// All done, free the uncoverted Root Hub name and return the
	// converted Root Hub name
	//
	SAFE_DELETE(rootHubNameW);

	return rootHubNameA;
}

BOOL EnumerateHub (PCTSTR HubName, PUSB_NODE_CONNECTION_INFORMATION_EX ConnectionInfo, PUSB_DESCRIPTOR_REQUEST ConfigDesc, PCTSTR DeviceDesc, int nLevel = 0)
{
	// Allocate some space for a USB_NODE_INFORMATION structure for this Hub,
	//
	PUSB_NODE_INFORMATION hubInfo = (PUSB_NODE_INFORMATION)new BYTE[sizeof(USB_NODE_INFORMATION)];
	// USB_HUB_CAPABILITIES_EX is only available in Vista and later headers
#if (_WIN32_WINNT >= 0x0600) 
	// Allocate some space for a USB_HUB_CAPABILITIES_EX structure for this Hub,
	//
	PUSB_HUB_CAPABILITIES_EX hubCapsEx = (PUSB_HUB_CAPABILITIES_EX)new BYTE[sizeof(USB_HUB_CAPABILITIES_EX)];
#endif

	// Allocate some space for a USB_HUB_CAPABILITIES structure for this Hub,
	//
	PUSB_HUB_CAPABILITIES hubCaps = (PUSB_HUB_CAPABILITIES)new BYTE[sizeof(USB_HUB_CAPABILITIES)];

	// Allocate a temp buffer for the full hub device name.
	//
	size_t deviceNameSize = _tcslen(HubName) + _tcslen(_T("\\\\.\\")) + 1;
	PTSTR deviceName = (PTSTR)new BYTE[deviceNameSize * sizeof(TCHAR)];

	// Create the full hub device name
	//
	_tcscpy_s(deviceName, deviceNameSize, _T("\\\\.\\"));
	_tcscat_s(deviceName, deviceNameSize, HubName);

	// Try to hub the open device
	//
	HANDLE hHubDevice = CreateFile(deviceName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hHubDevice == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	SAFE_DELETE(deviceName);

	ULONG nBytes = 0;
	// USB_HUB_CAPABILITIES_EX is only available in Vista and later headers
#if (_WIN32_WINNT >= 0x0600) 

	//
	// Now query USBHUB for the USB_HUB_CAPABILTIES_EX structure for this hub.
	//
	BOOL success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_HUB_CAPABILITIES_EX,
		hubCapsEx,
		sizeof(USB_HUB_CAPABILITIES_EX),
		hubCapsEx,
		sizeof(USB_HUB_CAPABILITIES_EX),
		&nBytes,
		NULL);

	// This will fail for pre-vista OS.  Ignore failures but don't try to use the data.
	if (!success)
	{
		hubCapsEx = NULL;
	}
#endif

	//
	// Now query USBHUB for the USB_HUB_CAPABILTIES structure for this hub.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_HUB_CAPABILITIES,
		hubCaps,
		sizeof(USB_HUB_CAPABILITIES),
		hubCaps,
		sizeof(USB_HUB_CAPABILITIES),
		&nBytes,
		NULL);

	if (!success)
	{
		hubCaps = NULL;
	}

	//
	// Now query USBHUB for the USB_NODE_INFORMATION structure for this hub.
	// This will tell us the number of downstream ports to enumerate, among
	// other things.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_NODE_INFORMATION,
		hubInfo,
		sizeof(USB_NODE_INFORMATION),
		hubInfo,
		sizeof(USB_NODE_INFORMATION),
		&nBytes,
		NULL);

	if (!success)
	{
		return FALSE;
	}


	// Build the leaf name from the port number and the device description
	//
	for (int i = 0; i < nLevel*3; i++)
		cout<<" ";

	if (DeviceDesc)
	{
		cout<<"Hub: "<<DeviceDesc<<endl;
	}
	else
	{
		cout<<"HubName: "<<HubName<<endl;
	}

	// Now recursively enumrate the ports of this hub.
	//
	EnumerateHubPorts(hHubDevice, hubInfo->u.HubInformation.HubDescriptor.bNumberOfPorts, ++nLevel);
	nLevel--;

	CloseHandle(hHubDevice);

	return TRUE;
}

PUSB_DESCRIPTOR_REQUEST GetConfigDescriptor(HANDLE hHubDevice, ULONG ConnectionIndex, UCHAR DescriptorIndex)
{
	BOOL    success;
	ULONG   nBytes;
	ULONG   nBytesReturned;

	UCHAR   configDescReqBuf[sizeof(USB_DESCRIPTOR_REQUEST) + sizeof(USB_CONFIGURATION_DESCRIPTOR)];

	PUSB_DESCRIPTOR_REQUEST         configDescReq;
	PUSB_CONFIGURATION_DESCRIPTOR   configDesc;


	// Request the Configuration Descriptor the first time using our
	// local buffer, which is just big enough for the Cofiguration
	// Descriptor itself.
	//
	nBytes = sizeof(configDescReqBuf);

	configDescReq = (PUSB_DESCRIPTOR_REQUEST)configDescReqBuf;
	configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq+1);

	// Zero fill the entire request structure
	//
	memset(configDescReq, 0, nBytes);

	// Indicate the port from which the descriptor will be requested
	//
	configDescReq->ConnectionIndex = ConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8)
		| DescriptorIndex;

	configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		configDescReq,
		nBytes,
		configDescReq,
		nBytes,
		&nBytesReturned,
		NULL);

	if (!success)
	{
		return NULL;
	}

	if (nBytes != nBytesReturned)
	{
		return NULL;
	}

	if (configDesc->wTotalLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
	{
		return NULL;
	}

	// Now request the entire Configuration Descriptor using a dynamically
	// allocated buffer which is sized big enough to hold the entire descriptor
	//
	nBytes = sizeof(USB_DESCRIPTOR_REQUEST) + configDesc->wTotalLength;

	configDescReq = (PUSB_DESCRIPTOR_REQUEST)new BYTE[nBytes];

	configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)(configDescReq+1);

	// Indicate the port from which the descriptor will be requested
	//
	configDescReq->ConnectionIndex = ConnectionIndex;

	//
	// USBHUB uses URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE to process this
	// IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION request.
	//
	// USBD will automatically initialize these fields:
	//     bmRequest = 0x80
	//     bRequest  = 0x06
	//
	// We must inititialize these fields:
	//     wValue    = Descriptor Type (high) and Descriptor Index (low byte)
	//     wIndex    = Zero (or Language ID for String Descriptors)
	//     wLength   = Length of descriptor buffer
	//
	configDescReq->SetupPacket.wValue = (USB_CONFIGURATION_DESCRIPTOR_TYPE << 8) | DescriptorIndex;

	configDescReq->SetupPacket.wLength = (USHORT)(nBytes - sizeof(USB_DESCRIPTOR_REQUEST));

	// Now issue the get descriptor request.
	//
	success = DeviceIoControl(hHubDevice,
		IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
		configDescReq,
		nBytes,
		configDescReq,
		nBytes,
		&nBytesReturned,
		NULL);

	if (!success)
	{
		return NULL;
	}

	if (nBytes != nBytesReturned)
	{
		return NULL;
	}

	if (configDesc->wTotalLength != (nBytes - sizeof(USB_DESCRIPTOR_REQUEST)))
	{
		return NULL;
	}

	return configDescReq;
}


const std::string GetExternalHubName (HANDLE  Hub,ULONG   ConnectionIndex)
{
	BOOL                        success;
	ULONG                       nBytes;
	USB_NODE_CONNECTION_NAME	extHubName;
	PUSB_NODE_CONNECTION_NAME   extHubNameW = NULL;

	// Get the length of the name of the external hub attached to the
	// specified port.
	//
	extHubName.ConnectionIndex = ConnectionIndex;

	success = DeviceIoControl(Hub,
		IOCTL_USB_GET_NODE_CONNECTION_NAME,
		&extHubName,
		sizeof(extHubName),
		&extHubName,
		sizeof(extHubName),
		&nBytes,
		NULL);

	if (!success)
	{
		return NULL;
	}

	// Allocate space to hold the external hub name
	//
	nBytes = extHubName.ActualLength;

	if (nBytes <= sizeof(extHubName))
	{
		return NULL;
	}

	extHubNameW = (PUSB_NODE_CONNECTION_NAME) new BYTE[nBytes];

	// Get the name of the external hub attached to the specified port
	//
	extHubNameW->ConnectionIndex = ConnectionIndex;

	success = DeviceIoControl(Hub,
		IOCTL_USB_GET_NODE_CONNECTION_NAME,
		extHubNameW,
		nBytes,
		extHubNameW,
		nBytes,
		&nBytes,
		NULL);

	if (!success)
	{
		SAFE_DELETE_ARRAY(extHubNameW);
		return NULL;
	}

	// Convert the External Hub name
	//
	std::string extHubNameA = WideStrToMultiStr(extHubNameW->NodeName);
	SAFE_DELETE_ARRAY(extHubNameW);

	return extHubNameA;
}

void EnumerateHubPorts(HANDLE hHubDevice, ULONG NumPorts, int nLevel)
{
	ULONG       index;
	BOOL        success;

	PUSB_DESCRIPTOR_REQUEST             configDesc;

	// Loop over all ports of the hub.
	//
	// Port indices are 1 based, not 0 based.
	//
	for (index=1; index <= NumPorts; index++)
	{
		ULONG nBytesEx;

		// Allocate space to hold the connection info for this port.
		// For now, allocate it big enough to hold info for 30 pipes.
		//
		// Endpoint numbers are 0-15.  Endpoint number 0 is the standard
		// control endpoint which is not explicitly listed in the Configuration
		// Descriptor.  There can be an IN endpoint and an OUT endpoint at
		// endpoint numbers 1-15 so there can be a maximum of 30 endpoints
		// per device configuration.
		//
		// Should probably size this dynamically at some point.
		//
		nBytesEx = sizeof(USB_NODE_CONNECTION_INFORMATION_EX) + sizeof(USB_PIPE_INFO) * 30;

		PUSB_NODE_CONNECTION_INFORMATION_EX connectionInfoEx = (PUSB_NODE_CONNECTION_INFORMATION_EX)new BYTE[nBytesEx];

		//
		// Now query USBHUB for the USB_NODE_CONNECTION_INFORMATION_EX structure
		// for this port.  This will tell us if a device is attached to this
		// port, among other things.
		//
		connectionInfoEx->ConnectionIndex = index;

		success = DeviceIoControl(hHubDevice,
			IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX,
			connectionInfoEx,
			nBytesEx,
			connectionInfoEx,
			nBytesEx,
			&nBytesEx,
			NULL);

		if (!success)
		{
			PUSB_NODE_CONNECTION_INFORMATION    connectionInfo;
			ULONG                               nBytes;

			// Try using IOCTL_USB_GET_NODE_CONNECTION_INFORMATION
			// instead of IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX
			//
			nBytes = sizeof(USB_NODE_CONNECTION_INFORMATION) + sizeof(USB_PIPE_INFO) * 30;

			connectionInfo = (PUSB_NODE_CONNECTION_INFORMATION)new BYTE[nBytes];

			connectionInfo->ConnectionIndex = index;

			success = DeviceIoControl(hHubDevice,
				IOCTL_USB_GET_NODE_CONNECTION_INFORMATION,
				connectionInfo,
				nBytes,
				connectionInfo,
				nBytes,
				&nBytes,
				NULL);

			if (!success)
			{
				continue;
			}

			// Copy IOCTL_USB_GET_NODE_CONNECTION_INFORMATION into
			// IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX structure.
			//
			connectionInfoEx->ConnectionIndex = connectionInfo->ConnectionIndex;

			connectionInfoEx->DeviceDescriptor = connectionInfo->DeviceDescriptor;

			connectionInfoEx->CurrentConfigurationValue = connectionInfo->CurrentConfigurationValue;

			connectionInfoEx->Speed = connectionInfo->LowSpeed ? UsbLowSpeed : UsbFullSpeed;

			connectionInfoEx->DeviceIsHub = connectionInfo->DeviceIsHub;

			connectionInfoEx->DeviceAddress = connectionInfo->DeviceAddress;

			connectionInfoEx->NumberOfOpenPipes = connectionInfo->NumberOfOpenPipes;

			connectionInfoEx->ConnectionStatus = connectionInfo->ConnectionStatus;

			memcpy(&connectionInfoEx->PipeList[0], &connectionInfo->PipeList[0], sizeof(USB_PIPE_INFO) * 30);

			SAFE_DELETE(connectionInfo);
		}

		// Update the count of connected devices
		//
		if (connectionInfoEx->ConnectionStatus == DeviceConnected)
		{
			g_ulTotalDevicesConnected++;
		}

		if (connectionInfoEx->DeviceIsHub)
		{
			g_nTotalHubs++;
		}

		// If there is a device connected, get the Device Description
		//
		string deviceDesc;
		if (connectionInfoEx->ConnectionStatus != NoDeviceConnected)
		{
			string driverKeyName = GetDriverKeyName(hHubDevice, index);

			if (!driverKeyName.empty())
			{
				deviceDesc = DriverNameToDeviceDesc(driverKeyName, FALSE);
			}
		}

		// If there is a device connected to the port, try to retrieve the
		// Configuration Descriptor from the device.
		//
		if (g_bDoConfigDesc && connectionInfoEx->ConnectionStatus == DeviceConnected)
		{
			configDesc = GetConfigDescriptor(hHubDevice, index, 0);
		}
		else
		{
			configDesc = NULL;
		}

		// If the device connected to the port is an external hub, get the
		// name of the external hub and recursively enumerate it.
		//
		if (connectionInfoEx->DeviceIsHub)
		{
			string extHubName = GetExternalHubName(hHubDevice,index);
			if (!extHubName.empty())
			{
				nLevel--;
				EnumerateHub(extHubName.c_str(), connectionInfoEx, configDesc, deviceDesc.c_str(), ++nLevel);
			}
		}
		else
		{
			for (int i = 0; i < nLevel*3; i++)
				cout<<" ";

			cout<<"[Port"<<index<<"]";
			if (deviceDesc.empty())
			{
				cout<<" NoDeviceConnected"<<endl;
			}
			else
			{
				cout<<" DeviceConnected: "<<deviceDesc.c_str()<<endl;
			}
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	char hcdName[16] = {0};
	for (int nHCDNo = 0; nHCDNo < MAX_HCD_COUNT; nHCDNo++)
	{
		sprintf_s(hcdName, "\\\\.\\HCD%d", nHCDNo);
		HANDLE hHCDev = CreateFile(hcdName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hHCDev == INVALID_HANDLE_VALUE)
			continue;
		
		string strHCDDriveKeyName = GetHCDDriverKeyName(hHCDev);
		if (strHCDDriveKeyName.empty())
			continue;

		string strDevDesc = DriverNameToDeviceDesc(strHCDDriveKeyName, TRUE);
		if (strDevDesc.empty())
			continue;

		string strRootHubName = GetRootHubName(hHCDev);
		if (strRootHubName.empty())
			continue;

		EnumerateHub(strRootHubName.c_str(),
			NULL,      // ConnectionInfo
			NULL,      // ConfigDesc
			_T("RootHub"),
			0);
// 		// 设备路径必须是usbstor
// 		if ((NULL == strstr(strDevDesc.c_str(), "usb")) || (NULL == strstr(strDevDesc.c_str(), "USB")))
// 			continue;


	}

	return 0;
}

