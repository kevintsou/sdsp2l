#ifndef DEF_DATA_H
#define DEF_DATA_H
#ifdef __cplusplus
extern "C" {
#endif
extern int writePageData(int lba, int *pPayload);
extern int readPageData(int lba, int *pPayload);

#ifdef __cplusplus
}
#endif
#endif