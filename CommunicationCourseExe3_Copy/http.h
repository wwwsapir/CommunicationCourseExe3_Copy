#define INVALID_HTTP_MSG -1
#define VALID_HTTP_MSG 1
#define EMPTY_REQ 0
#define NOT_EMPTY_REQ 1
#define STRINGS_EQUAL 0

#define NUMBER_OF_OPTIONS 7
#define MAX_OPTION_SIZE 100
#define EMPTY_STRING "\0"
#define NULL 0
enum httpMethodsEnum { GET, POST, PUT, DEL, OPTIONS, HEAD, TRACE };
const char httpMethods[NUMBER_OF_OPTIONS][MAX_OPTION_SIZE] = { "GET", "POST", "PUT", "DELETE", "OPTIONS", "HEAD", "TRACE" };


struct HttpRequest
{
	char isEmpty = EMPTY_REQ;
	int method = -1;
	char url[1024] = EMPTY_STRING;
	char httpVersion[9];

	char connectionHeader[255] = EMPTY_STRING;
	char contentTypeHeader[255] = EMPTY_STRING;
	int contentLengthHeader = 0;

	char* content = NULL;
	char* rawRequest = NULL;
};

struct HttpResponse
{
	char httpVersion[9] = "HTTP/1.1";
	int responseCode = -1;
	char statusPhrase[36] = EMPTY_STRING;

	char connectionHeader[255] = EMPTY_STRING;
	char contentTypeHeader[255] = "text/html; charset=UTF-8";
	char lastModifiedHeader[255] = EMPTY_STRING;
	char serverHeader[11] = "Windows 10";
	int contentLengthHeader = 0;

	char allowHeader[255] = EMPTY_STRING;
	char* content = NULL;
};

int parseHttpRequest(char *msg, int len, HttpRequest *reqPtr);
int httpResponseToString(HttpResponse response, char buffer[]);
HttpResponse handleGetRequest(HttpRequest req);
HttpResponse handlePostRequest(HttpRequest req);
HttpResponse handlePutRequest(HttpRequest req);
HttpResponse handleTraceRequest(HttpRequest req);
HttpResponse handleOptionsRequest(HttpRequest req);
HttpResponse handleHeadRequest(HttpRequest req);
HttpResponse handleDeleteRequest(HttpRequest req);
int getQueryParameter(char* query, char* parametr, char value[]);
void operateQuery(char* url);