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

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
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
		return;
	}
	else if (strcmp(req.connectionHeader, "close") == 0)
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
		//response = handleGetRequest(sockets[index].req1);
		break;
	case POST:
		//response = handlePostRequest(sockets[index].req1);
		break;
	case PUT:
		//response = handlePutRequest(sockets[index].req1);
		break;
	case TRACE:
		//response = handleTraceRequest(sockets[index].req1);
		break;
	case OPTIONS:
		//response = handleOptionsRequest(sockets[index].req1);
		break;
	case HEAD:
		//response = handleHeadRequest(sockets[index].req1);
		break;
	case DEL:
		//response = handleDeleteRequest(sockets[index].req1);
		break;
	}

	char responseStrBuffer[10000];
	int responseLen = httpResponseToString(response, responseStrBuffer);
	bytesSent = send(msgSocket, responseStrBuffer, responseLen, 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "HTTP Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

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