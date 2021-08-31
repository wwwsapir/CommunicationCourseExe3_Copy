#include <stdio.h>
#include "fileServer.h"

int getObject(char* path, char* buffer)
{
	FILE * filePointer = fopen(path, "r");
	if (filePointer == NULL)
	{
		return FILE_ERROR;
	}
	fscanf(filePointer, buffer);
	return SUCCESS;
}

int createObject(char* path, char* buffer)
{
	FILE * filePointer = fopen(path, "w+");	// if exists, rewrite the file content
	if (filePointer == NULL)
	{
		return FILE_ERROR;
	}
	fprintf(filePointer, buffer);
	return SUCCESS;
}

int deleteObject(char* path, char* buffer)
{
	if (remove(path) == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FILE_ERROR;
	}
}