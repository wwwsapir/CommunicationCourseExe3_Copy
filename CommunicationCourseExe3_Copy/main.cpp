#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std;
#include <iostream>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include "http.h"


struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	HttpRequest req1;
	HttpRequest req2;
};



const int HTTP_SERVER_PORT = 80;
const int MAX_SOCKETS = 60;

const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);


struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


void main()
{
	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "HTTP server: Error at WSAStartup()\n";
		return;
	}
	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "HTTP Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(HTTP_SERVER_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "HTTP Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "HTTP Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout << "HTTP Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				sendMessage(i);
				/*switch (sockets[i].send)
				{
				case SEND:

				break;
				}*/
			}
		}
	}

	// Closing connections and Winsock.
	cout << "HTTP Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	// Set the socket to be in non-blocking mode.
	unsigned long flag = 1;
	if (ioctlsocket(id, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
	sockets[index].req1.isEmpty = EMPTY_REQ;
	sockets[index].req2.isEmpty = EMPTY_REQ;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "HTTP Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;
	char reqBuffer[10000];

	int bytesRecv = recv(msgSocket, reqBuffer, 10000, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}

	HttpRequest req;
	int isValid = parseHttpRequest(reqBuffer, bytesRecv, &req);
	if (isValid == INVALID_HTTP_MSG)
	{
		return; //todo : return meaningfull error if type known
	}
	else if (strcmp(req.connectionHeader, "close") == STRINGS_EQUAL ||
		strcmp(req.connectionHeader, "Close") == STRINGS_EQUAL)
	{
		closesocket(msgSocket);
		removeSocket(index);
	}
	else
	{
		if (sockets[index].req1.isEmpty == EMPTY_REQ)
		{
			sockets[index].req1 = req;
		}
		else
		{
			sockets[index].req2 = req;
		}

		if (sockets[index].req1.isEmpty != EMPTY_REQ)
		{
			sockets[index].send = SEND;
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;

	SOCKET msgSocket = sockets[index].id;
	HttpResponse response;
	switch (sockets[index].req1.method)
	{
	case GET:
		response = handleGetRequest(sockets[index].req1);
		break;
	case POST:
		response = handlePostRequest(sockets[index].req1);
		break;
	case PUT:
		response = handlePutRequest(sockets[index].req1);
		break;
	case TRACE:
		response = handleTraceRequest(sockets[index].req1);
		break;
	case OPTIONS:
		response = handleOptionsRequest(sockets[index].req1);
		break;
	case HEAD:
		response = handleHeadRequest(sockets[index].req1);
		break;
	case DEL:
		response = handleDeleteRequest(sockets[index].req1);
		break;
	}
	//to do : free malloc of data for request
	//free(sockets[index].req1.content);
	//sockets[index].req1.content = NULL;
	char responseStrBuffer[10000] = EMPTY_STRING;
	int responseLen = httpResponseToString(response, responseStrBuffer);
	//to do : free malloc of data for responce
	//free(response.content);
	bytesSent = send(msgSocket, responseStrBuffer, responseLen, 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: " << bytesSent << " bytes sent" << "\n";
	// If there's a second message waiting for response then move it to be the first one
	if (sockets[index].req2.isEmpty == EMPTY_REQ)
	{
		sockets[index].send = IDLE;
	}
	else
	{
		sockets[index].req1 = sockets[index].req2;
		sockets[index].req2.isEmpty = EMPTY_REQ;
	}
}