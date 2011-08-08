#ifndef __MFS_MFS_H__
#define __MFS_MFS_H__

void init_mfs(void);

int createMemFile(char* fileName, unsigned int recordLength, unsigned int recordNum);

int openMemFile(char* fileName, char accessMode);



#endif
