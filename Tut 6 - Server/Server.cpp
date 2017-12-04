#include "Server.h"
#include <algorithm>

Server::Server(int PORT) //Port = port to broadcast on. BroadcastPublically = false if server is not open to the public (people outside of your router), true = server is open to everyone (assumes that the port is properly forwarded on router settings)
{
	//Winsock Startup
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0) //If WSAStartup returns anything other than 0, then that means an error has occured in the WinSock Startup.
	{
		MessageBoxA(NULL, "WinSock startup failed", "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	inet_pton(AF_INET, "127.0.0.1", &m_Addr.sin_addr);
	m_Addr.sin_port = htons(PORT); //Port
	m_Addr.sin_family = AF_INET; //IPv4 Socket
	m_SListen = socket(AF_INET, SOCK_STREAM, NULL); //Create socket to listen for new connections
	if (bind(m_SListen, (SOCKADDR*)&m_Addr, sizeof(m_Addr)) == SOCKET_ERROR) //Bind the address to the socket, if we fail to bind the address..
	{
		std::string ErrorMsg = "Failed to bind the address to our listening socket. Winsock Error:" + std::to_string(WSAGetLastError());
		MessageBoxA(NULL, ErrorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}
	if (listen(m_SListen, SOMAXCONN) == SOCKET_ERROR) //Places sListen socket in a state in which it is listening for an incoming connection. Note:SOMAXCONN = Socket Oustanding Max Connections, if we fail to listen on listening socket...
	{
		std::string ErrorMsg = "Failed to listen on listening socket. Winsock Error:" + std::to_string(WSAGetLastError());
		MessageBoxA(NULL, ErrorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
		exit(1);
	}
	m_Serverptr = this;

	m_VecRejectedSockets.resize(m_MaxConnections, NULL);
	m_VecSockets.resize(m_MaxConnections, NULL);
	m_VecMatches.resize(m_MaxConnections / 2, nullptr);
	m_VecPlayers.resize(m_MaxConnections, nullptr);
}

Server::~Server()
{
	deleteVector(m_VecMatches);
	deleteVector(m_VecPlayers);
}

bool Server::ListenForNewConnection()
{	
	SOCKET newConnection = accept(m_SListen, (SOCKADDR*)&m_Addr, &m_Addrlen); //Accept a new connection
	if (newConnection == 0) //If accepting the client connection failed
	{
		std::cout << "Failed to accept the client's connection." << std::endl;
		return false;
	}
	else //If client connection properly accepted
	{
		int suggestedId = GetIdOfNextEmptyVecPos(m_VecSockets);
		if (suggestedId == 404) {
			std::cout << "Max connections reached." << std::endl;
			suggestedId = GetIdOfNextEmptyVecPos(m_VecRejectedSockets);
			m_VecRejectedSockets[suggestedId] = newConnection;
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerRejectThread, (LPVOID)(suggestedId), NULL, NULL);
			return false;
		}
		std::cout << "Connection created with id: " << suggestedId << std::endl;
		m_VecSockets[suggestedId] = newConnection; //Set socket in array to be the newest connection before creating the thread to handle this client's socket.
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(suggestedId), NULL, NULL); //Create Thread to handle this client. The index in the socket array for this thread is the value (i).
		m_TotalConnections++; //Incremenent total # of clients that have connected
		return true;
	}
}

bool Server::ProcessPacket(int playerId, int otherPlayerId, Packet type)
{
	switch (type)
	{
		case Packet::P_Move:
		{
			string move;
			GetString(playerId, move);

			SendPacketType(otherPlayerId, Packet::P_Move);
			SendString(otherPlayerId, move);
			break;
		}
		case Packet::P_Quit:
		{
			SendPacketType(otherPlayerId, Packet::P_Quit);
			break;
		}
		case Packet::P_QuitAck:{}
		default: //If packet type is not accounted for
		{
			std::cout << "Unrecognized packet: " << (int)type << std::endl; //Display that packet was not found
			break;
		}
	}
	return true;
}

int Server::GetIdOfFirstInactiveMatch()
{
	int count = 0;
	/*
	for_each(m_VecMatches.begin(), m_VecMatches.end(), [&count](Match* m) {
		if (m->active == false)return count;
		++count;
	});
	*/
	for (auto e : m_VecMatches) {
		if(e != nullptr)
			if (e->active == false)return count;
		if (e == nullptr) {
			m_VecMatches[count] = new Match();
			return count;
		}
		++count;
	}
	return 404;
}

template<typename T> int Server::GetIdOfNextEmptyVecPos(vector<T> vec)
{
	int count = 0;
	/*
	for_each(vec.begin(), vec.end(), [&count](T e) {
		if (e == nullptr)
			return count;
		++count;
	});
	*/
	for (auto e : vec) {
		if (e == NULL)return count;
		++count;
	}
	return 404;
}

template<typename T>
void Server::deleteVector(vector<T> vec)
{
	for (UINT i = 0; i < vec.size(); ++i)
	{
		if (vec[i] != nullptr) {
			delete vec[i];
		}
	}
	vec.clear(); 
}

Color Server::GetColorOfPlayer(int id)
{
	if (m_TotalConnections % 2 == 0)return Color::BLACK;
	return Color::WHITE;
}

void Server::ClientHandlerThread(int id) //ID = the index in the SOCKET Connections array
{
	Packet PacketType;
	int playerId, otherPlayerId, matchId;

	m_Serverptr->m_VecPlayers[id] = new Player(id, m_Serverptr->GetColorOfPlayer(id));

	if (!m_Serverptr->SendPacketType(id, Packet::P_Ack)) {
		cout << "failed to send P_Ack to id: " << id;
		return;
	}

	if (!m_Serverptr->SendPacketType(id, Packet::P_PlayerInfo)) {
		cout << "failed to send P_PlayerInfo to id: " << id;
		return;
	}

	if (!m_Serverptr->SendInt(id, id)) {
		cout << "failed to send playerinfo id to id: " << id;
		return;
	}


	if (!m_Serverptr->SendInt(id, (int)(m_Serverptr->m_VecPlayers[id]->color))) {
		cout << "failed to send playerinfo color to id: " << id;
		return;
	}

	if (m_Serverptr->m_VecPlayers[id]->color == Color::BLACK) {
		matchId = m_Serverptr->GetIdOfFirstInactiveMatch();
		m_Serverptr->m_VecMatches[matchId]->playerB = m_Serverptr->m_VecPlayers[id];
		otherPlayerId = m_Serverptr->m_VecMatches[matchId]->playerW->id;
		m_Serverptr->m_VecMatches[matchId]->active = true;

		m_Serverptr->SendPacketType(id, Packet::P_Start);
	}
	else {
		matchId = m_Serverptr->GetIdOfNextEmptyVecPos(m_Serverptr->m_VecMatches);
		m_Serverptr->m_VecMatches[matchId] = new Match(m_Serverptr->m_VecPlayers[id]);
		while (!m_Serverptr->m_VecMatches[matchId]->active)Sleep(100);

		otherPlayerId = m_Serverptr->m_VecMatches[matchId]->playerB->id;		
		m_Serverptr->SendPacketType(id, Packet::P_Start);
	}

	while (true)
	{
		if (!m_Serverptr->GetPacketType(id, PacketType)) break;
		if (!m_Serverptr->ProcessPacket(id, otherPlayerId, PacketType)) break;
		if (PacketType == Packet::P_Quit || PacketType == Packet::P_QuitAck) break;
	}
	m_Serverptr->m_TotalConnections--;
	std::cout << "Lost connection to client id: " << id << std::endl;

	if (m_Serverptr->m_VecPlayers[id]->color == Color::WHITE)delete m_Serverptr->m_VecMatches[matchId];
	m_Serverptr->m_VecMatches[matchId] = nullptr;

	delete m_Serverptr->m_VecPlayers[id];
	m_Serverptr->m_VecPlayers[id] = nullptr;

	closesocket(m_Serverptr->m_VecSockets[id]);
	m_Serverptr->m_VecSockets[id] = NULL;
}

void Server::ClientHandlerRejectThread(int id) //ID = the index in the SOCKET Connections array
{
	
	//bool sendTrue = false;
	//while (!sendTrue); {
	//	sendTrue = m_Serverptr->SendInt(m_Serverptr->m_VecRejectedSockets[id], (int)Packet::P_Reject);
	//	Sleep(500);
	//} 
	int k = 0;
	m_Serverptr->SendInt(m_Serverptr->m_VecRejectedSockets[id], (int)Packet::P_Reject);

	closesocket(m_Serverptr->m_VecRejectedSockets[id]);
	m_Serverptr->m_VecRejectedSockets[id] = NULL;
}