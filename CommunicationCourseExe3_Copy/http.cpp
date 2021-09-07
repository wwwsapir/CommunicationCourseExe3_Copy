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
	char* rest = msg;


	char* line = strtok_s(msg, "\n", &rest);
	int lineID = 0;
	bool inContentPartOfRequest = false;

	while (line != NULL && lineID >= 0 && !inContentPartOfRequest) {
		cout << "parseHttpRequest lineID: "<< lineID << " line: " << line << "\n";
		if (lineID == 0) //method url version
		{
			if (parseMethod(line, reqPtr) == INVALID_HTTP_MSG)
			{
				return INVALID_HTTP_MSG;
			}

			//save raw request for trace method
			if (reqPtr->method == TRACE)
			{
				reqPtr->rawRequest = (char*)malloc(len);
				if (reqPtr->rawRequest !=NULL)
					strcpy(reqPtr->rawRequest, msg);
			}
		}
		else
		{
			char* key = strtok(line, ":");
			char* value = strtok(NULL, "\r") + 1; //+1 skip space

			if (strcmp("\r", key) == STRINGS_EQUAL && reqPtr->contentLengthHeader > 0) //empty row before data - end of headers
			{
				reqPtr->content = (char *)malloc(reqPtr->contentLengthHeader);
				if(reqPtr->content != NULL)
					strcpy(reqPtr->content, rest);
				inContentPartOfRequest = true;
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
	char httpDate[1000];
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	strftime(httpDate, sizeof httpDate, "%a, %d %b %Y %H:%M:%S %Z", &tm);

	int r1 = 0;
	char baseHeaders[1000] = EMPTY_STRING;
	r1 = sprintf(baseHeaders, "%s %d %s\nServer: %s\nConnection: %s\nDate: %s\n",
		response.httpVersion,
		response.responseCode,
		response.statusPhrase,
		response.serverHeader,
		response.connectionHeader,
		httpDate);

	if (response.contentLengthHeader > 0) 
	{
		//response has content
		r1 = r1 + sprintf(buffer, "%sContent-Length: %d\nLast-Modified: %s\nContent-Type: %s\n\n%s\n",
			baseHeaders,
			response.contentLengthHeader,
			response.lastModifiedHeader,
			response.contentTypeHeader,
			response.content);
	}
	else
	{
		//resonse only headers
		if (strlen(response.allowHeader) > 0) //OPTIONS responce extention
		{
			r1 = r1 + sprintf(buffer, "%sAllow: %s\nContent-Length: 0\n", baseHeaders, response.allowHeader);
		}
		else
		{
			r1 = r1 + sprintf(buffer, "%sContent-Length: 0\n", baseHeaders);
		}
		
	}

	//debug log
	cout << "httpResponseToString : finished , response length : " << r1 << " - " << strlen(buffer) << "\n";
	cout << "Response : " << "\n";
	cout << buffer << "\n";
	return strlen(buffer);
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
	HttpResponse res;

	operateQuery(req.url); //takes care language parameters and default document name

	int stat = createFileObject(req.url, req.content);
	if (stat == SUCCESS)
	{
		strcpy(res.statusPhrase, "OK");
		res.responseCode = 200;
	}
	else
	{
		strcpy(res.statusPhrase, "Internal Server Error");
		res.responseCode = 500;
	}
	strcpy(res.connectionHeader, req.connectionHeader);
	return res;
}

HttpResponse handleTraceRequest(HttpRequest req)
{
	HttpResponse res;
	res.responseCode = 200;
	strcpy(res.statusPhrase, "OK");
	strcpy(res.contentTypeHeader, "message/http");
	strcpy(res.connectionHeader, req.connectionHeader);

	return res;
}

HttpResponse handleOptionsRequest(HttpRequest req)
{
	HttpResponse res;
	strcpy(res.statusPhrase, "No Content");
	res.responseCode = 204;
	strcpy(res.connectionHeader, req.connectionHeader);
	strcpy(res.allowHeader, "GET, POST, PUT, DELETE, OPTIONS, HEAD, TRACE");
	return res;
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