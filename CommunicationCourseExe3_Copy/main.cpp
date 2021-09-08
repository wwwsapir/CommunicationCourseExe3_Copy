#define _WINSOCK_DEPRECATED_NO_WARNINGS
using namespace std;
#include <iostream>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include "http.h"

const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv = EMPTY;			// Receiving?
	int	send = EMPTY;			// Sending?
	HttpRequest req1;
	HttpRequest req2;
	long int lastReceiveTime;
};

const int HTTP_SERVER_PORT = 80;
const int MAX_SOCKETS = 60;

bool addSocket(SOCKET id, int what, SocketState sockets[], int* socketCountPtr);
void removeSocket(int index, SocketState sockets[], int* socketCountPtr);
void acceptConnection(int index, SocketState sockets[], int* socketCountPtr);
void receiveMessage(int index, SocketState sockets[], int* socketCountPtr);
void sendMessage(int index, SocketState sockets[], int* socketCountPtr);


void main()
{
	struct SocketState sockets[MAX_SOCKETS];
	int socketsCount = 0;

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
	addSocket(listenSocket, LISTEN, sockets, &socketsCount);

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

		//close connection if idle more then 2 minutes
		//must come directly after blocking "select" 
		for (int i = 1; i < MAX_SOCKETS; i++) //i =1 excluding main listener socket 
		{
			//socket which dont send, listening and did not receive for more then 2 minutes
			long int tnow = GetTickCount64();
			//if (((sockets[i].send == EMPTY && sockets[i].recv == EMPTY)|| (sockets[i].send == IDLE && sockets[i].recv == RECEIVE)) && (tnow - sockets[i].lastReceiveTime) / 1000 > 120 )
			if (sockets[i].send == IDLE && sockets[i].recv != EMPTY && (tnow - sockets[i].lastReceiveTime) / 1000 > 120)
			{
				SOCKET idt = sockets[i].id;
				if (FD_ISSET(sockets[i].id, &waitRecv)) //remove this timeouted socket
				{
					nfd--;
				}
				closesocket(sockets[i].id);
				removeSocket(i, sockets, &socketsCount);
				cout << "HTTP Server: Client socket " << idt << " closed after 2 minutes timeout" << endl;
			}
		}


		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i, sockets, &socketsCount);
					break;

				case RECEIVE:
					receiveMessage(i, sockets, &socketsCount);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				sendMessage(i, sockets, &socketsCount);
			}
		}


		
	}

	// Closing connections and Winsock.
	cout << "HTTP Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what, SocketState sockets[], int* socketCountPtr)
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
			sockets[i].lastReceiveTime = GetTickCount64();
			(*socketCountPtr)++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index, SocketState sockets[], int* socketCountPtr)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	(*socketCountPtr)--;
	deleteRequest(&sockets[index].req1, FREE_CONTENT);
	deleteRequest(&sockets[index].req2, FREE_CONTENT);
}

void acceptConnection(int index, SocketState sockets[], int* socketCountPtr)
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
	cout << "HTTP Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected to socket " << msgSocket << endl;

	if (addSocket(msgSocket, RECEIVE, sockets, socketCountPtr) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index, SocketState sockets[], int* socketCountPtr)
{
	SOCKET msgSocket = sockets[index].id;
	char reqBuffer[10000];

	int bytesRecv = recv(msgSocket, reqBuffer, 10000, 0);

	//marking the time for the 2 minutes time out
	if (bytesRecv >0)
		sockets[index].lastReceiveTime = GetTickCount64();

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "HTTP Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index, sockets, socketCountPtr);
		return;
	}

	HttpRequest req;
	int isValid = parseHttpRequest(reqBuffer, bytesRecv, &req);
	if (isValid == INVALID_HTTP_MSG)
	{
		sockets[index].req1.isEmpty = NOT_EMPTY_REQ;
		sockets[index].req1.isValid = INVALID_HTTP_MSG;
		sockets[index].send = SEND;
		return;
	}
	else if (strcmp(req.connectionHeader, "close") == STRINGS_EQUAL ||
		strcmp(req.connectionHeader, "Close") == STRINGS_EQUAL)
	{
		closesocket(msgSocket);
		removeSocket(index, sockets, socketCountPtr);
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

		sockets[index].req1.isValid = VALID_HTTP_MSG;
		sockets[index].send = SEND;
	}
}

void sendMessage(int index, SocketState sockets[], int* socketCountPtr)
{
	int bytesSent = 0;

	SOCKET msgSocket = sockets[index].id;
	HttpResponse response;
	if (sockets[index].req1.isValid == VALID_HTTP_MSG)
	{
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
	}
	else  // Invalid http request
	{
		response = handleInvalidRequest(sockets[index].req1);
	}
	deleteRequest(&sockets[index].req1, FREE_CONTENT);

	char responseStrBuffer[10000] = EMPTY_STRING;
	int responseLen = httpResponseToString(response, responseStrBuffer);

	bytesSent = send(msgSocket, responseStrBuffer, responseLen, 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "HTTP Server: " << bytesSent << " bytes sent via " << msgSocket << " socket\n";
	// If there's a second message waiting for response then move it to be the first "in line". If not, make sending idle.
	if (sockets[index].req2.isEmpty == EMPTY_REQ)
	{
		sockets[index].send = IDLE;
	}
	else
	{
		sockets[index].req1 = sockets[index].req2;
		deleteRequest(&sockets[index].req2, LEAVE_CONTENT_AS_IS);
	}
}