//#ifndef _PY_ERROR_CODE_
//#define _PY_ERROR_CODE_

#include <windows.h>



#define ERR_UNKNOWN						0x01		// unknown ERR
#define ERR_MISC						0x02		// misc
#define ERR_SCSI						0x03		// SCSI Fail
#define ERR_DEVICE						0x04		// Not Support Device
#define ERR_SW_ALLOCATE					0x05		// 
#define ERR_SYS_OPEN_FILE				0x06		// 
#define ERR_SYS_READ_FILE				0x07		// 
#define ERR_SYS_WRITE_FILE				0x08		// 
#define ERR_INVALID_HANDLE				0x09		//
#define ERR_INITIAL						0x0A		//


#define ERR_BRIDGERESET					0x10		//Bridge Reset Fail 
#define ERR_IDENTIFY_NVMe				0x11		//Identify NVMe Fail
#define ERR_FLASHID						0x12		//Flash ID not support in CSV
#define ERR_LOADBURNER					0x13		//Load Burner Fail
#define ERR_SETFEATURE					0x14		//Set Feature Fail