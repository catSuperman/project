#ifndef __BO_INDEX_H__
#define __BO_INDEX_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "BO.h"



char * Detect_BO(FILE * fptr,char * rdbuf);

int Get_BO_Index(const char * bo,BO_Unit_t *boptr);

int Get_SG_Index(FILE * fptr,char * rdbuf,BO_Unit_t *boptr);

int Free_BO_(int BO_Number);

int Dbc_load(FILE * dbc_cfg_fd,char *dbc_cfg_rdbuf,BO_Unit_t **BO_List);

int DBCinit();
#endif // BO_INDEX_H_INCLUDED