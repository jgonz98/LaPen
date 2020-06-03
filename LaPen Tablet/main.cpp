#include <SDL.h>
#include <string>
#include <iostream>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <WinUser.h>
#pragma comment(lib, "ws2_32.lib")


void MouseSetup(INPUT *buffer, double SCREEN_WIDTH, double SCREEN_HEIGHT){
	buffer->type = INPUT_MOUSE;
	buffer->mi.dx = (0 * (0xFFFF / SCREEN_WIDTH));
	buffer->mi.dy = (0 * (0xFFFF / SCREEN_HEIGHT));
	buffer->mi.mouseData = 0;
	buffer->mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
	buffer->mi.time = 0;
	buffer->mi.dwExtraInfo = 0;
}


void MouseMoveAbsolute(INPUT *buffer, int x, int y, double SCREEN_WIDTH, double SCREEN_HEIGHT){
	buffer->mi.dx = (x * (0xFFFF / SCREEN_WIDTH));
	buffer->mi.dy = (y * (0xFFFF / SCREEN_HEIGHT));
	buffer->mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE);

	SendInput(1, buffer, sizeof(INPUT));
}


void LMouseClickdown(INPUT *buffer){
	buffer->mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN);
	SendInput(1, buffer, sizeof(INPUT));
}

void LMouseClickup(INPUT *buffer) {
	buffer->mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP);
	SendInput(1, buffer, sizeof(INPUT));
}


int server() {
	//Ask user for port
	int port;
	std::cout << "Enter the port you would like to listen on:";
	std::cin >> port;

	//Set resolution height and width
	double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN) - 1;
	double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN) - 1;

	//Remote resolution
	double rScreenWidth = NULL, rScreenHeight = NULL;

	//Initialize winsock
	WSADATA data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		return EXIT_FAILURE;
	}

	//Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		std::cerr << "Can't create socket, Err#" << WSAGetLastError() << std::endl;
		WSACleanup();
		return EXIT_FAILURE;
	}

	//Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	//Tell winsock the socket is for listening
	listen(listening, SOMAXCONN);

	//Wait for a connection
	std::cout << "Listening for a connection.." << std::endl;
	sockaddr_in client;
	int clientSize = sizeof(client);

	SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
	if (clientSocket == INVALID_SOCKET) {
		std::cerr << "Can't create socket, Err#" << WSAGetLastError() << std::endl;
		WSACleanup();
		return EXIT_FAILURE;
	}

	char host[NI_MAXHOST];			//Clients remote name
	char service[NI_MAXHOST];		//Service (i.e. port) the client is connected on
	
	ZeroMemory(host, NI_MAXHOST);
	ZeroMemory(service, NI_MAXHOST);

	if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
		std::cout << host << " connected on port " << service << std::endl;
	}
	else {
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
	}

	//Close listening socket
	closesocket(listening);

	//TCP Datatype
	char buf[4096];

	//Recieve resolution data and echo for connection confirmation 
	while (true) {
		ZeroMemory(buf, 4096);
		//Wait for client to send data
		int bytesReceived = recv(clientSocket, buf, 4096, 0);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "Error in recv(). Quitting" << std::endl;
			break;
		}

		if (bytesReceived == 0) {
			std::cout << "Client disconnected" << std::endl;
			break;
		}

		//Set remote screen resolution X & Y
		if (rScreenHeight == NULL) {
			rScreenHeight = std::stod(buf);
			std::cout << "RECIEVED: rScreenHeight: " << buf << std::endl;
			send(clientSocket, buf, bytesReceived + 1, 0);
		}

		if (rScreenWidth == NULL) {
			recv(clientSocket, buf, 4096, 0);
			rScreenWidth = std::stod(buf);
			std::cout << "RECIEVED: rScreenWidth: " << buf << std::endl;
			send(clientSocket, buf, bytesReceived + 1, 0);
			std::cout << "Setup Complete" << std::endl;
			break;
		}
	}
	
	//Setup difference in resolution for mouse position
	double xMultiplier = 1;
	double yMultiplier = 1;
	yMultiplier = fScreenHeight / rScreenHeight;
	xMultiplier = fScreenWidth / rScreenWidth;

	//Recieve mouse cordinates
	while (true) {
		ZeroMemory(buf, 4096);
		//Wait for client to send data
		int bytesReceived = recv(clientSocket, buf, 4096, 0);
		if (bytesReceived == SOCKET_ERROR) {
			std::cerr << "Error in recv(). Quitting" << std::endl;
			break;
		}

		if (bytesReceived == 0) {
			std::cout << "Client disconnected" << std::endl;
			break;
		}
		
		//Process Recieved data
		std::string xMouse;
		std::string yMouse;
		std::string bMouse;
		int splitPoint = NULL;

		for (int i = 0; i < bytesReceived - 1; i++) {
			if (buf[i] == '.') {
				splitPoint = i;
				break;
			}
			xMouse += buf[i];
		}

		for (int i = splitPoint+1; i < bytesReceived - 1; i++) {
			if (buf[i] == '.') {
				splitPoint = i;
				break;
			}
			yMouse += buf[i];
		}

		for (int i = splitPoint + 1; i < bytesReceived - 1; i++) {
			if (buf[i] == '\0') {
				break;
			}
			bMouse += buf[i];
		}


			
		//Output recieved data for debugging
		std::cout << "RECEIVED: " << buf << std::endl;
		std::cout << "PROCESSED: " << xMouse << '.' << yMouse << '.' << bMouse << std::endl;
		
		//Do mouse actions
		INPUT buffer[1];
		MouseSetup(buffer, fScreenWidth, fScreenHeight);
		if (std::stoi(xMouse) <= fScreenWidth && std::stoi(yMouse) <= fScreenHeight && std::stoi(bMouse) > -1) {
			MouseMoveAbsolute(buffer, std::stoi(xMouse)*xMultiplier, std::stoi(yMouse)*yMultiplier, fScreenWidth, fScreenHeight);
			if (bMouse == "1")
				LMouseClickdown(buffer);
			else
				LMouseClickup(buffer);
		}
		else
			std::cout << "PACKET LOSS OR ERROR" << std::endl;
	}
	//Close the socket
	closesocket(clientSocket);

	//Cleanup winsock
	WSACleanup();
}

int main(int argc, char *argv[]) {
	
	//Ask if they are a PC or tablet
	char systemMode;
	std::cout << "Enter Y/N if this is the PC: ";
	std::cin >> systemMode;
	std::cin.ignore();
	systemMode = toupper(systemMode);
	if(systemMode == 'Y'){
		server();
		return EXIT_SUCCESS;
	}

	//TCP Server/Client setup
	//Ask for ip address and port
	std::string ipAddress;
	int port;
	std::cout << "Enter the IP address of the PC:";
	std::getline(std::cin,ipAddress);
	std::cout << "Enter the port of the IP address:";
	std::cin >> port;
	std::cout << "\nIP:" << ipAddress << ":" << port << std::endl;
	
	//Set Resolution Height and Width
	double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN) - 1;
	double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN) - 1;

	//Initialize winSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start Winsock, Err #" << wsResult << std::endl;
		return EXIT_FAILURE;
	}

	//Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, Err#" << WSAGetLastError() << std::endl;
		WSACleanup();
		return EXIT_FAILURE;
	}

	//Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	//Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		std::cerr << "Can't connect to server, Err #" << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return EXIT_FAILURE;
	}

	//TCP Data Variable
	char buf[4096];
	int resState = 0;
	
	//Send resolution until it is echoed
	while (true) {
		ZeroMemory(buf, 4096);
		Sleep(500);
		if (resState == 0) {
			send(sock, std::to_string(fScreenHeight).c_str(), std::to_string(fScreenHeight).size() + 1, 0);
			int bytesReceived = recv(sock, buf, 4096, 0);

			if (std::to_string(fScreenHeight) == buf) {
				resState++;
			}
		}
		else if (resState == 1) {
			send(sock, std::to_string(fScreenWidth).c_str(), std::to_string(fScreenWidth).size() + 1, 0);
			int bytesReceived = recv(sock, buf, 4096, 0);

			if (std::to_string(fScreenWidth) == buf) {
				resState++;
			}
		}
		else if (resState == 2) {
			std::string complete = "Setup Complete";
			std::cout << complete << std::endl;
			break;
		}
	}


	//SDL Initialization Check
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
	}
	
	//SDL Objects
	SDL_Window* window;
	SDL_Renderer* renderer;
	
	//Window Setup
	window = SDL_CreateWindow("LaPen Tablet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, SDL_WINDOW_FULLSCREEN);
	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	
	//Cursor Setup
	SDL_ShowCursor(SDL_ENABLE);

	// Check that the window was successfully created
	if (window == NULL) {
		// In the case that the window could not be made...
		printf("Could not create window: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	//While loop to get scan for mouse input but quit on escape
	const Uint8* state = SDL_GetKeyboardState(NULL);
	int xMouse, yMouse, xcMouse = 0, ycMouse = 0, bMouse = 0, bcMouse = 0;
	SDL_Event Mouse;

	while (state[SDL_SCANCODE_ESCAPE] == 0) {
		ZeroMemory(buf, 4096);
		SDL_PollEvent(&Mouse);
		SDL_GetGlobalMouseState(&xMouse, &yMouse);
		
		if (Mouse.type == SDL_MOUSEBUTTONDOWN && Mouse.button.button == SDL_BUTTON_LEFT)
			bMouse = 1;
		else if (Mouse.type == SDL_MOUSEBUTTONUP && Mouse.button.button == SDL_BUTTON_LEFT)
			bMouse = 0;

		//Send mouse position
		if (xcMouse != xMouse || ycMouse != yMouse || bMouse != bcMouse) {
			std::string mousePos = std::to_string(xMouse) + '.' + std::to_string(yMouse) + '.' + std::to_string(bMouse);
			int sendResult = send(sock, mousePos.c_str(), mousePos.size() + 1, 0);
			if (sendResult != SOCKET_ERROR) {
				//Wait for response
				std::cout << "SENDING: " << mousePos << std::endl;
			}
		}
		xcMouse = xMouse;
		ycMouse = yMouse;
		bcMouse = bMouse;
		SDL_PumpEvents();
	}

	//Clean up
	SDL_DestroyWindow(window);
	SDL_Quit();
	closesocket(sock);
	WSACleanup();
	return EXIT_SUCCESS;
}
