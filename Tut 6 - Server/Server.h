#pragma once

#include <Ws2tcpip.h>
#include <WinSock2.h>
#include <string>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

#include <vector>

using namespace std;

enum class Packet
{
	P_Ack,
	P_Move,
	P_Quit,
	P_Reject,
	P_PlayerInfo,
	P_QuitAck,
	P_Start
};

enum class Color
{
	BLACK,
	WHITE
};

struct Player {
	Player(int id, Color color) : id(id)  ,color(color) {}
	int id;
	Color color;
};

struct Match {
	Match(Player* playerW = nullptr, Player* playerB = nullptr, bool active = false) : playerW(playerW), playerB(playerB), active(false) {}
	~Match() {
		playerW = nullptr;
		playerB = nullptr;
	}
	Player* playerW;
	Player* playerB;
	bool active = false;
};

class Server
{
public:
	Server(int PORT);
	~Server();
	bool ListenForNewConnection();

private:
	bool SendInt(int id, int value);
	bool GetInt(int id, int & value);

	bool SendPacketType(int id, Packet type);
	bool GetPacketType(int id, Packet & type);

	bool SendString(int id, std::string & value);
	bool GetString(int id, std::string & value);

	bool ProcessPacket(int id, int otherPlayerId, Packet type);

	int GetIdOfFirstInactiveMatch();
	
	template<typename T> int GetIdOfNextEmptyVecPos(vector<T> vec);

	Color GetColorOfPlayer(int id);

	static void ClientHandlerThread(int id);
	static void ClientHandlerRejectThread(int  id);

private:
	int m_TotalConnections = 0;
	int m_MaxConnections = 50;
	SOCKADDR_IN m_Addr; //Address that we will bind our listening socket to
	int m_Addrlen = sizeof(m_Addr);
	SOCKET m_SListen;

	vector <SOCKET> m_VecSockets;
	vector <SOCKET> m_VecRejectedSockets;
	vector <Match*> m_VecMatches;
	vector <Player*> m_VecPlayers;

	template<typename T>
	void deleteVector(vector<T> vec);
};

static Server * m_Serverptr; //Serverptr is necessary so the static ClientHandler method can access the server instance/functions.
