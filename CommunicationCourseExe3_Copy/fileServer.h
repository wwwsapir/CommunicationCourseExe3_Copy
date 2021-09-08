#pragma once
#define FILE_ERROR -1
#define SUCCESS 0

#define NO_CONTENT 0
#define FILL_CONTENT 1

int getFileObject(char* path, char** filelContentPtr, int* contentLenPtr, int fillContent);
int getFileLen(char* path, int* contentLenPtr);
int createFileObject(char* path, char* newFileContent);
int deleteFileObject(char* path);
void getLastModifiedDate(char* path, char* buffer);
int getFileLen(char* path, int* contentLenPtr);