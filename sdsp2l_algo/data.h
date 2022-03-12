#ifndef DEF_DATA_H
#define DEF_DATA_H
#ifdef __cplusplus
extern "C" {
#endif



extern int iWritePageData(int lba, int *pPayload);
extern int iReadPageData(int lba, int *pPayload);
extern int* pDataPayload;	                

#ifdef __cplusplus
}
#endif
#endif