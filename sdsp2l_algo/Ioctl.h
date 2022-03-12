#ifndef _IOCTL_H_
#define _IOCTL_H_

#define USB2SER_IOCTL_INDEX	0x0800

// For GP0
#define GP0_OUTPUT_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN,\
				         USB2SER_IOCTL_INDEX+20,\
				         METHOD_BUFFERED, \
				         FILE_ANY_ACCESS)					

#define GP1_OUTPUT_ENABLE	CTL_CODE(FILE_DEVICE_UNKNOWN,\
				         USB2SER_IOCTL_INDEX+21,\
				         METHOD_BUFFERED,\
				         FILE_ANY_ACCESS)

#define GP0_SET_VALUE		CTL_CODE(FILE_DEVICE_UNKNOWN,\
				         USB2SER_IOCTL_INDEX+22,\
				         METHOD_BUFFERED,\
				         FILE_ANY_ACCESS)

// For GP1
#define GP1_SET_VALUE		CTL_CODE(FILE_DEVICE_UNKNOWN,\
				         USB2SER_IOCTL_INDEX+23,\
				         METHOD_BUFFERED,\
				         FILE_ANY_ACCESS)

#define GP0_GET_VALUE		CTL_CODE(FILE_DEVICE_UNKNOWN,\
				         USB2SER_IOCTL_INDEX+24,\
				         METHOD_BUFFERED,\
				         FILE_ANY_ACCESS)

#define GP1_GET_VALUE		CTL_CODE(FILE_DEVICE_UNKNOWN,\
				         USB2SER_IOCTL_INDEX+25,\
				         METHOD_BUFFERED,\
				         FILE_ANY_ACCESS)



#define GP2_OUTPUT_ENABLE	         CTL_CODE(FILE_DEVICE_UNKNOWN,\
				                     USB2SER_IOCTL_INDEX+45,\
				                     METHOD_BUFFERED, \
				                     FILE_ANY_ACCESS)	
#define GP3_OUTPUT_ENABLE	         CTL_CODE(FILE_DEVICE_UNKNOWN,\
				                     USB2SER_IOCTL_INDEX+46,\
				                     METHOD_BUFFERED, \
				                     FILE_ANY_ACCESS)	
#define GP2_SET_VALUE		         CTL_CODE(FILE_DEVICE_UNKNOWN,\
				                     USB2SER_IOCTL_INDEX+47,\
				                     METHOD_BUFFERED, \
				                     FILE_ANY_ACCESS)	

#define GP3_SET_VALUE	            CTL_CODE(FILE_DEVICE_UNKNOWN,\
				                     USB2SER_IOCTL_INDEX+48,\
				                     METHOD_BUFFERED, \
				                     FILE_ANY_ACCESS)	

#define GP2_GET_VALUE		         CTL_CODE(FILE_DEVICE_UNKNOWN,\
				                     USB2SER_IOCTL_INDEX+49,\
				                     METHOD_BUFFERED, \
				                     FILE_ANY_ACCESS)	

#define GP3_GET_VALUE	            CTL_CODE(FILE_DEVICE_UNKNOWN,\
				                     USB2SER_IOCTL_INDEX+50,\
				                     METHOD_BUFFERED, \
				                     FILE_ANY_ACCESS)	

#endif