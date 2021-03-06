using namespace std;
#pragma once
#include <stdio.h>
#include "fileServer.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <iostream>

#define PATH_BUFFER_SIZE 255
#define HTTP_TIME_FORMAT_STR_GMT "%a, %d %b %Y %X GMT"

void addFilesFolderToPath(char* url, char* buffer)
{
	for (int i = 0; url[i]; i++)
	{
		if (url[i] == '\\')
		{
			url[i] = '/';
		}
	}
	sprintf_s(buffer, PATH_BUFFER_SIZE, "server_files%s", url);
}

int getFileObject(char* path, char** filelContentPtr, int* contentLenPtr, int fillContent)
{
	int stat = getFileLen(path, contentLenPtr);
	if (stat != SUCCESS)
	{
		return FILE_ERROR;
	}

	FILE *filePointer;
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);

	fopen_s(&filePointer, pathBuffer, "r"); //open file for read
	if (filePointer == NULL)
	{
		return FILE_ERROR;
	}
	*filelContentPtr = (char*)malloc(*contentLenPtr); //allocate memory for the content
	int contentLenRead = 0;
	int linesCounter = 0;
	while (fgets(*filelContentPtr + contentLenRead, *contentLenPtr, filePointer) != NULL)
	{
		if (*filelContentPtr != NULL)
		{
			contentLenRead = strlen(*filelContentPtr);
			linesCounter += 1;
		}
	};
	fclose(filePointer);
	*contentLenPtr -= (linesCounter - 1);	// Work-around for windows /n/r (two chars) in newline.
	if (fillContent == NO_CONTENT)
	{
		free(*filelContentPtr);
		*filelContentPtr = NULL;
	}
	return SUCCESS;
}

int getFileLen(char* path, int* contentLenPtr)
{
	FILE* filePointer;
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);

	errno_t err = fopen_s(&filePointer, pathBuffer, "r"); //open file for read
	if (err != 0)
	{
		return FILE_ERROR;
	}
	fseek(filePointer, 0L, SEEK_END); //go to end of the file
	*contentLenPtr = ftell(filePointer); //get pointer to the end of the file
	fclose(filePointer);
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
	//fprintf(filePointer,"%s", newFileContent);
	if (newFileContent != NULL)
		fputs(newFileContent, filePointer);
	else
		fputs("", filePointer);
	fclose(filePointer);
	return SUCCESS;
}

int deleteFileObject(char* path)
{
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);
	if (remove(pathBuffer) == 0)
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
	strftime(buffer, PATH_BUFFER_SIZE, HTTP_TIME_FORMAT_STR_GMT, &time);
}