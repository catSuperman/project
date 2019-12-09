#ifndef __RECORD_FILE_H_
#define __RECORD_FILE_H_

#include "DataTypes.h"
#include <string.h>


boolean EnumRecordFile(char* pFileName,char* pFilePathName);
boolean OpenUpRecordFile(char* FileName,unsigned int* dwFileSize,unsigned int* dwBlockSize,unsigned int* dwFileChecksum);
void CloseCurOpenedUpRecordFile();
boolean GetFileTxBlockData(unsigned int dwBlockIndex,unsigned char* pData);
void SaveUpRecordFileMirror();
boolean LoadUpRecordFileMirror(unsigned int* dwCurBlockIndex);
void DeleteRecordFile(char* pFileName);
void CopyFileToPath(char* SrcFilePathName,char* DstPath);



#endif