#pragma once
#define FILE_ERROR -1
#define SUCCESS 0

int getFileObject(char* path, char** filelContentPtr, int* contentLenPtr); //f1
int getFileLen(char* path, int* contentLenPtr);
int createFileObject(char* path, char* newFileContent);
int deleteFileObject(char* path);
void getLastModifiedDate(char* path, char* buffer);
int getFileLen(char* path, int* contentLenPtr);