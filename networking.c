/* File server application | Jack Nelson | CSP2308
 * networking.c
 * Provision and handling of network communications */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "fileServer.h"
#include "networking.h"
#include "io.h"
#include "logger.h"
#include "security.h"
#include "files.h"

void serverStart(config_t* Config, int* serverStarted) {

	pid_t pid;

	if ((pid = fork()) < 0) {

		perror("Could not create server process");

	} else if (pid == 0) {

		// Child / Server process
		struct sockaddr_in serverAddress, clientAddress;
		int listenSocket;

		if ((listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0) {

				perror("Could not create socket");

		} else {
			
			serverAddress.sin_family = AF_INET;
			serverAddress.sin_addr.s_addr = INADDR_ANY;
			serverAddress.sin_port = htons(Config->portNumber);

			if (bind(listenSocket, (struct sockaddr*) &serverAddress, sizeof(struct sockaddr_in)) < 0) {
				
				perror("Could not bind port");

			} else {

				// This bit goes in a loop with threading stuff
				listen(listenSocket, 128);

				int clientSocket;
				unsigned int clientSize = sizeof(struct sockaddr_in);
				
				if ((clientSocket = accept(listenSocket, (struct sockaddr*) &clientAddress, &clientSize)) < 0) {
					
					perror("Could not accept new connection");

				} else {
				
					// Build the theadData struct containing everything we need for client interaction	
					threadData_t serverInfo = {clientSocket, Config};

					// Maybe take the client address out later if we dont end up using it?
					strcpy(serverInfo.clientAddress, inet_ntoa(clientAddress.sin_addr));

					logPipe("Client connected\n", Config->logFd);

					// Start a new thread and enter the thread function
					connectionHandler(&serverInfo);
				}
				
				close(listenSocket);
				*serverStarted = 0;
			}
		}

		exit(0);
	
	} else {

		*serverStarted = 1;
		// Parent / Control process
	}
}

void connectionHandler(threadData_t* ServerInfo) {

	if (clientLogin(ServerInfo) == 1) {

		char menuChoiceString[2];
		int menuChoice = 0;

		// User authenticated successfully
		write(ServerInfo->clientSocket, ServerInfo->Config->motd, sizeof(ServerInfo->Config->motd));

		while (menuChoice != 4) {

			read(ServerInfo->clientSocket, menuChoiceString, 2);
			menuChoice = atoi(menuChoiceString);

			switch (menuChoice) {
				case (1):
					listFiles(ServerInfo);
					break;
				case (2):
					recieveFileMenu(ServerInfo);
					break;
				case (3):
					sendFileMenu(ServerInfo);
					break;
				case (4):
					printf("Quit\n");
			}
		}


	} else {

		// User failed authentication
		write(ServerInfo->clientSocket, "Access Denied.", 14);
	}
	
}