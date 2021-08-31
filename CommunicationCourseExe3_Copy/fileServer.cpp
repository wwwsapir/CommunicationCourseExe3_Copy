#pragma once
#include <stdio.h>
#include "fileServer.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#define PATH_BUFFER_SIZE 255

void addFilesFolderToPath(char* url, char* buffer)
{
	sprintf_s(buffer, PATH_BUFFER_SIZE, "server_files/%s", url);
}

int getFileObject(char* path, char** filelContentPtr, int* contentLenPtr)
{
	FILE* filePointer;
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);
	errno_t err = fopen_s(&filePointer, pathBuffer, "r");
	if (err != 0)
	{
		return FILE_ERROR;
	}
	fseek(filePointer, 0L, SEEK_END);
	*contentLenPtr = ftell(filePointer);
	*filelContentPtr = (char*)malloc(*contentLenPtr);
	fscanf_s(filePointer, *filelContentPtr);
	return SUCCESS;
}

int createFileObject(char* path, char* newFileContent)
{
	FILE* filePointer;
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);
	errno_t err = fopen_s(&filePointer, pathBuffer, "w+");	// if exists, rewrite the file content
	if (err != 0)
	{
		return FILE_ERROR;
	}
	fprintf(filePointer, newFileContent);
	return SUCCESS;
}

int deleteFileObject(char* path)
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

void getLastModifiedDate(char* path, char* buffer)
{
	struct stat attrib;
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);
	stat(pathBuffer, &attrib);

	tm time;
	gmtime_s(&time, &(attrib.st_mtime));
	strftime(buffer, PATH_BUFFER_SIZE, "%d %b, %Y %X", &time);
}