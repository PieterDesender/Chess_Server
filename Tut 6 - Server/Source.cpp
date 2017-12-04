/*
Author: Pindrought
Date: 2/24/2016
This is the solution for the server that you should have at the end of tutorial 6.
*/
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include <string>

#include "Server.h"
#include <atomic>
#include <thread>
#include <windows.h>

void ReadCin(atomic<bool>& run);
void mainMethod();

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(7221);

	mainMethod();

	cout << "\nDone";
	_CrtDumpMemoryLeaks();
	cin.get();

	return 0;
}

void mainMethod()
{
	atomic<bool>gameRun(true);
	thread cinThread = thread(ReadCin, ref(gameRun));
	Server MyServer(1111); //Create server on port 1111

	while (gameRun.load())
	{
		MyServer.ListenForNewConnection(); //Accept new connection (if someones trying to connect)
	}

	cout << "\nClosing Server.";
	MyServer.~Server();
	cinThread.join();
	//system("pause");
}


void ReadCin(atomic<bool>& run)
{
	string input;
	while (run.load())
	{
		cin >> input;
		if (input == "q")run.store(false);
	}
	cout << "End Cin thread\n";
}