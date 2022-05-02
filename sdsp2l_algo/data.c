#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "data.h"
#include "p2l.h"
#include <malloc.h>
#include <sys/types.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>


int* pDataPayload;	                // payload buffer
int* pBlkErCntTbl[D_MAX_CH_CNT];					// erase count table with slc mode bit
int* pRdCntTbl[D_MAX_CH_CNT];						// die block read count table
int* pIdlTimeTbl[D_MAX_CH_CNT];						// block idle time table
long chIoBurstCnt[D_MAX_CH_CNT];						// io burst count for each channel

//const char* Dir_base = "C:\\Users\\kenzw_lin\\source\\repos\\nes-3.0-simulator\\nes-3.0-simulator\\sdsp2l_algo\\sdsp2l_algo\\";

const int five_lbn_size = 20480;	//16384+2208=18592 16k page size + 2k parity, 1LBN = 4k, 1 entry = 5 LBN
const int page_size = 18432;

t_file_mgr file_mgr;

// write 16384+2208=18592 bytes to storage
int iWritePageData(int lbn, int* pPayload, int page_num, int plane_num) {

	//int fd_write;
	errno_t err;
	int blk_lbn_base = ((lbn - (lbn_mgr.lbnEntryCnt / 1024)) / (5 * dev_mgr.pageCnt * dev_mgr.planeCnt)) * (5 * dev_mgr.pageCnt * dev_mgr.planeCnt);	//((lbn-0x2000)/0x5000)*0x5000
	int blk_lbn_per_plane = (lbn_mgr.lbnEntryCnt / 1024) + blk_lbn_base + (plane_num * (5 * dev_mgr.pageCnt));
	char* Data_file = (char*)malloc(128 * sizeof(char));
	char* Data_dir = (char*)malloc(128 * sizeof(char));

	if (blk_lbn_per_plane != file_mgr.blk_lbn_per_plane_wr) {

		if (file_mgr.fd_write != -1) {
			_close(file_mgr.fd_write);
			file_mgr.fd_write = -1;
		}

		memset(Data_dir, -1, 128);
		if (Data_file != NULL) {
			memset(Data_file, -1, 128);
		}
		else {
			return 0;
		}

		if ((Data_dir = _getcwd(NULL, 128)) == NULL) {	//get current dir
			free(Data_dir);
			free(Data_file);
		}
		sprintf_s(Data_file, 128 * sizeof(char), "%s\\test_%d.bin", Data_dir, blk_lbn_per_plane);	//Accroding to diff block, create a file

		if (_access(Data_file, 0) != 0) {	//0 means check this file is exist or not
			err = _sopen_s(&file_mgr.fd_write, Data_file, _O_WRONLY | _O_CREAT | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);	//create a new file
		}
		else {
			err = _sopen_s(&file_mgr.fd_write, Data_file, _O_WRONLY | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
		}
	}


	if (file_mgr.fd_write != -1)
	{
		if (_lseek(file_mgr.fd_write, page_num * five_lbn_size * sizeof(char), SEEK_SET) != -1) {	//shift to diff page

			int index = page_num * five_lbn_size * sizeof(char);
			_write(file_mgr.fd_write, pPayload, page_size * sizeof(char));
		}
	}
	else {
		printf("Fail, File '%s' wasn't opened\n", Data_file);
	}
	//pDataPayload[(lbn - (lbn_mgr.lbnEntryCnt / 1024))/5] = *pPayload;



	free(Data_file);
	free(Data_dir);
	Data_file = NULL;

	file_mgr.blk_lbn_per_plane_wr = blk_lbn_per_plane;
	
	return 0;
}

// read 18592 bytes from storage
int iReadPageData(int lbn, int* pPayload, int page_num, int plane_num) {

	errno_t err;
	//int fd_read;
	char* Data_file = (char*)malloc(128 * sizeof(char));
	char* Data_dir = (char*)malloc(128 * sizeof(char));

	int blk_lbn_base = ((lbn - (lbn_mgr.lbnEntryCnt / 1024)) / (5 * dev_mgr.pageCnt * dev_mgr.planeCnt)) * (5 * dev_mgr.pageCnt * dev_mgr.planeCnt);	//((lbn-0x2000)/0x5000)*0x5000
	int blk_lbn_per_plane = (lbn_mgr.lbnEntryCnt / 1024) + blk_lbn_base + (plane_num * (5 * dev_mgr.pageCnt));

	memset(Data_dir, -1, 128);
	if (Data_file != NULL) {
		memset(Data_file, -1, 128);
	}
	else {
		return 0;
	}

	if ((Data_dir = _getcwd(NULL, 128)) == NULL) {
		free(Data_dir);
		free(Data_file);
	}

	sprintf_s(Data_file, 128 * sizeof(char), "%s\\test_%d.bin", Data_dir, blk_lbn_per_plane);

	if ((err = _sopen_s(&file_mgr.fd_read, Data_file, _O_RDONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE)) == 0) {
		printf("File opened, exec read\n");
		if (_lseek(file_mgr.fd_read, page_num * five_lbn_size, SEEK_SET) != -1) {
			_read(file_mgr.fd_read, pPayload, page_size);
		}
	}
	else {
		printf("Fail, File '%s' wasn't opened\n", Data_file);
	}

	//*pPayload = pDataPayload[(lbn - (lbn_mgr.lbnEntryCnt / 1024))/5];

	if (file_mgr.fd_read != -1) {
		_close(file_mgr.fd_read);
		file_mgr.fd_read = -1;
	}
	free(Data_dir);
	free(Data_file);
	//free(buffer);
	return 0;
}