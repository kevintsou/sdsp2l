//#include "WorkItem.h"

#include <windows.h>
#include <basetyps.h>
//#include "dConfig.h"
#include <winioctl.h>
#include <setupapi.h>

#include <string.h>
//#include "usbioctl.h"
#include "usbioctl.h"//"c:\\WINDDK\\2600.1106\\inc\wxp\\usbioctl.h"
//#include <usbioctl.h>
//#include <usbview.h>
#include <cfgmgr32.h>
//#include "MpCommon.h"
//#include "usbdesc.h"
//#include "PortInof.h"
#define WM_MYMESSAGE_ADD (WM_USER + 0x01)
//#define NUM_HCS_TO_CHECK 10
#define ICTYPE_DEF_PS2231	0x0E			// [454:0x1c6]
#include <vector>
using namespace std;

//typedef struct PORTINFO
//{
//    BYTE PortNumber;
//	int Partition ;
//	char DriveLetter[10];
//	wchar_t Manufacture[256];
//	wchar_t Product[256];
//	wchar_t SerialNumber[256];
//	char DriveKeyName[10][256];
//
//} PORTINFO, *PPORTINFO;
/*
typedef struct _THREAD_CONTEXT
{
   CWorkItem* pWorkItem;
   int ID;
   HWND hwnd;
} THREAD_CONTEXT,*PTHREAD_CONTEXT;

*/
/*

typedef struct _STRING_DESCRIPTOR_NODE_
{
    struct _STRING_DESCRIPTOR_NODE *Next;
    UCHAR                           DescriptorIndex;
    USHORT                          LanguageID;
    USB_STRING_DESCRIPTOR           StringDescriptor[0];
} STRING_DESCRIPTOR_NODE, *PSTRING_DESCRIPTOR_NODE;

//
////
//// Structures assocated with TreeView items through the lParam.  When an item
//// is selected, the lParam is retrieved and the structure it which it points
//// is used to display information in the edit control.
////

typedef struct _USBDEVICEINFO
{
    PUSB_NODE_INFORMATION               HubInfo;        // NULL if not a HUB
    PCHAR                               HubName;        // NULL if not a HUB
    PUSB_NODE_CONNECTION_INFORMATION    ConnectionInfo; // NULL if root HUB
    PUSB_DESCRIPTOR_REQUEST             ConfigDesc;     // NULL if root HUB
    PSTRING_DESCRIPTOR_NODE             StringDescs;

} USBDEVICEINFO, *PUSBDEVICEINFO;
*/
//#define ALLOC(dwBytes) MyAlloc(__FILE__, __LINE__, (dwBytes))
//
//HGLOBAL
//MyAlloc (
//    PCHAR   File,
//    ULONG   Line,
//    DWORD   dwBytes
//);
