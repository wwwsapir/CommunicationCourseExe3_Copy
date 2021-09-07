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

int getFileObject(char* path, char** filelContentPtr, int* contentLenPtr)
{
	int stat = getFileLen(path, contentLenPtr);
	if (stat != SUCCESS)
	{
		return FILE_ERROR;
	}

	FILE* filePointer;
	char pathBuffer[PATH_BUFFER_SIZE];
	addFilesFolderToPath(path, pathBuffer);

	fopen_s(&filePointer, pathBuffer, "r"); //open file for read (must succeed because getFileLen succeeded)
	*filelContentPtr = (char*)malloc(*contentLenPtr); //allocate memory for the content
	int contentLenRead = 0;
	while (fgets(*filelContentPtr + contentLenRead, *contentLenPtr, filePointer) != NULL)
	{
		if (*filelContentPtr != NULL)
		{
			contentLenRead = strlen(*filelContentPtr);
		}
	};
	fclose(filePointer);
	//cout << "\ncontent is:\n" <<  *filelContentPtr << "\n";
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
	fputs(newFileContent, filePointer);
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
	strftime(buffer, PATH_BUFFER_SIZE, "%d %b, %Y %X", &time);
}