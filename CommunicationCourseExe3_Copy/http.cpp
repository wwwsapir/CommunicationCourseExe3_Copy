#define _CRT_SECURE_NO_WARNINGS
#include "http.h"
#include <iostream>
#include <string.h>
#include "fileServer.h"
using namespace std;

int parseMethod(char * line, HttpRequest* reqPtr)
{
	char* part = strtok(line, " "); //method
	reqPtr->method = -1;
	for (int i = 0; i < NUMBER_OF_OPTIONS; i++)
	{
		if (!strcmp(httpMethods[i], part))
		{
			reqPtr->method = i;
		}
	}
	if (reqPtr->method == -1) //error validation
	{
		return INVALID_HTTP_MSG;
	};
	part = strtok(NULL, " "); //url
	// Remove prededing "/" of url:
	//if (strlen(part) > 0 && part[0] == '/') //todo fix "/" removal
	//{
	//	part = part + 1;
	//}
	strcpy(reqPtr->url, part);
	//cout << part << "\n";
	part = strtok(NULL, "\r"); //version
							   //cout << part << "\n";
	strcpy(reqPtr->httpVersion, part);
	return VALID_HTTP_MSG;
}

void parseHeader(char* line, HttpRequest* reqPtr, char* key, char* value)
{
	if (strcmp("Connection", key) == STRINGS_EQUAL)
	{
		strcpy(reqPtr->connectionHeader, value);
	}
	if (strcmp("Content-Type", key) == STRINGS_EQUAL)
	{
		strcpy(reqPtr->contentTypeHeader, value);
	}
	if (strcmp("Content-Length", key) == STRINGS_EQUAL)
	{
		reqPtr->contentLengthHeader = atoi(value);
	}
}


int parseHttpRequest(char *msg, int len, HttpRequest *reqPtr)
{
	if (len == 0)
	{
		return INVALID_HTTP_MSG;
	}
	msg[len] = '\0';
	cout << msg;
	char* rest = msg;

	char* line = strtok_s(msg, "\n", &rest);
	int lineID = 0;

	while (line != NULL && lineID >= 0) {
		cout << line << "\n";
		if (lineID == 0) //method url version
		{
			if (parseMethod(line, reqPtr) == INVALID_HTTP_MSG)
			{
				return INVALID_HTTP_MSG;
			}
		}
		else 
		{
			char* key = strtok(line, ":");
			char* value = strtok(NULL, "\r") + 1; //+1 skip space

			if (strcmp("\r", key) != STRINGS_EQUAL && reqPtr->contentLengthHeader > 0) //empty row before data - end of headers
			{
				reqPtr->content = (char *)malloc(reqPtr->contentLengthHeader);
				strcpy(reqPtr->content, rest);
			}
			else
			{
				parseHeader(line, reqPtr, key, value);
			}
		}
		line = strtok_s(NULL, "\n", &rest);
		lineID++;
	}
	reqPtr->isEmpty = NOT_EMPTY_REQ;
	return VALID_HTTP_MSG;
}

int httpResponseToString(HttpResponse response, char buffer[])
{
	int r1 = sprintf(buffer, "%s %d %s\n%s", response.httpVersion, response.responseCode,response.statusPhrase,response.content);
	cout << buffer;
	return r1;
}

HttpResponse handleGetRequest(HttpRequest req)
{
	HttpResponse res;
	int contentLen;

	operateQuery(req.url); //takes care language parameters and default documet name

	int stat = getFileObject(req.url, &res.content, &contentLen); //f1
	strcpy(res.connectionHeader, "keep-alive"); //todo :change responce keep-alive to requested
	if (stat == SUCCESS)
	{
		res.contentLengthHeader = contentLen;
		strcpy(res.contentTypeHeader, "text/html");
		getLastModifiedDate(req.url, res.lastModifiedHeader);
		strcpy(res.statusPhrase, "OK");
		res.responseCode = 200;
	}
	else
	{
		strcpy(res.statusPhrase, "Not Found");
		res.responseCode = 404;
	}
	return res;
}

void operateQuery(char* url)
{
	//handle query params
	char langParamValue[3] = EMPTY_STRING;
	char newURL[1024];
	if (getQueryParameter(url, "lang", langParamValue) > 0)
	{
		sprintf(newURL, "/%s%s", langParamValue, url);
	}
	else
	{
		sprintf(newURL, "/%s%s", "en", url);
	}
	strcpy(url, newURL);

	//clear query parameters after handling
	for (int i = 0; url[i]; i++)
	{
		if (url[i] == '?')
			url[i] = '\0';
	}

	//handle default documents
	if (url[strlen(url) - 1] == '/')
	{
		sprintf(newURL, "%s%s", url, "index.html");
		strcpy(url, newURL);
	}
}


int getQueryParameter(char* query, char* parametr, char value[])
{
	char* addr = strstr(query, parametr);
	if (addr == NULL) //parameter not found
	{
		return 0;
	}
	//extract parameter
	addr = addr + strlen(parametr) + 1; //skip 'parameter=' 
	int valueLength = 0;
	while (addr[valueLength] && addr[valueLength] != '\0' && addr[valueLength] != '&')
	{
		valueLength++;
	}
	strncpy(value, addr, valueLength);
	return valueLength;
}

HttpResponse handlePostRequest(HttpRequest req)
{
	cout << "handlePostRequest Not Implemented!";
	return HttpResponse();
}

HttpResponse handlePutRequest(HttpRequest req)
{
	cout << "handlePutRequest Not Implemented!";
	return HttpResponse();
}

HttpResponse handleTraceRequest(HttpRequest req)
{
	cout << "handleTraceRequest Not Implemented!";
	return HttpResponse();
}

HttpResponse handleOptionsRequest(HttpRequest req)
{
	cout << "handleOptionsRequest Not Implemented!";
	return HttpResponse();
}

HttpResponse handleHeadRequest(HttpRequest req)
{
	cout << "handleHeadRequest Not Implemented!";
	return HttpResponse();
}

HttpResponse handleDeleteRequest(HttpRequest req)
{
	HttpResponse res;

	operateQuery(req.url); //takes care language parameters and default documet name
	
	int stat = deleteFileObject(req.url);
	if (stat == SUCCESS)
	{
		strcpy(res.statusPhrase, "OK");
		res.responseCode = 204;
	}
	else
	{
		strcpy(res.statusPhrase, "Not Found");
		res.responseCode = 404;
	}
	strcpy(res.contentTypeHeader, "text/html");
	res.contentLengthHeader = 0;
	return res;
}