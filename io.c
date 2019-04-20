/* File server application | Jack Nelson | CSP2308
 * io.c
 * Input and output functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fileServer.h"
#include "files.h"
#include "logger.h"
#include "io.h"
#include "settings.h"
#include "security.h"

void controlLogin(config_t* Config, int configStatus) {
	/* Login screen for access to server control
	 * Also allows for registering of a server username and password.
	 *
	 * Note that if something interrupts the program between the config file being created
	 * and new credentials being set, the server will use hardcoded defaults of admin:password */

	char username[11];
	char password[31];

	printf("----------------------------------------\n"
	       "       Server control panel login       \n"
	       "----------------------------------------\n");

	if (configStatus == 1) {
		printf("No saved credentials: Please register some now!\n");
		setCredentials(Config);
		printf("\nNow test your created credentials by logging in below.\n\n");
	}

	printf("Username: ");
	getKeyboardInput(username, 11);
	printf("Password: ");
	getKeyboardInput(password, 31);

	if (authenticate(Config->serverCreds, username, password) == 1) {
		printf("Access granted.\n\n");
	} else {
		printf("Access denied.\n");
		exit(EXIT_FAILURE);
	}
}

int clientLogin(threadData_t* serverInfo) {

	//char buffer[256];
	char username[11];
	char password[31];

	bzero(username, 11);
	bzero(password, 31);

	//printf("\n");
	//bzero(buffer, 256);

	//write(serverInfo->clientSocket, "Username: ", 11);
	//read(serverInfo->clientSocket, buffer, sizeof(buffer));
	//strncpy(username, buffer, 10);
	getSocketInput(username, 11, serverInfo->clientSocket);

	//bzero(buffer, 256);

	write(serverInfo->clientSocket, "Password: ", 11);
	//read(serverInfo->clientSocket, buffer, sizeof(buffer));
	//strncpy(password, buffer, 30);
	getSocketInput(password, 31, serverInfo->clientSocket);

	//REMOVE THESE WHEN CLIENT SIDE IS DONE!
	// Not sure why the hell its making me chomp at two?
	//username[strlen(username)-2] = '\0';
	//password[strlen(password)-2] = '\0';
	//printf("%s, %ld", username, strlen(username));
	//printf("%s, %ld", password, strlen(password));

	return(authenticate(serverInfo->Config->serverCreds, username, password));
}

void printWelcome(char* motd) {
	// Prints the main menu along with the MOTD.

	printf("----------------------------------------\n"
	       "  Welcome to Jack's fileshare server!\n"
	       "----------------------------------------\n"
	       "Message of the day:\n"
	       "%s\n"
	       "----------------------------------------\n", motd);
}

void showMainMenuOptions() {
	// Prints the options for the main menu

	printf("\n[1] Start server\n[2] Set server password\n[3] List hosted files\n[4] Shutdown Server / Exit\n");
	printf("\nSelection: ");
}

void getKeyboardInput(char* inputString, int inputLength) {
	// Gets and stores user keyboard input in a given string

	char ch;

	fgets(inputString, inputLength, stdin);

	if (inputString[strlen(inputString)-1] == '\n') {
		inputString[strlen(inputString)-1] = '\0';
	} else {
		while ((ch = getchar() != '\n') && (ch != EOF));
	}
}

void getSocketInput(char* inputString, int inputLength, int sockFd) {
	// Gets and stores input from a socket file descriptor

	read(sockFd, inputString, inputLength);
	
	//if (inputLength > 2) {
		// If the input length is over two, we want to chomp out a newline.

	//	inputString[strlen(inputString)-2] = '\0';
	//}
	
}

int listFiles(fileList_t* FileList, char* shareFolder) {
	// Outputs a list of current files in the shared directory.
	
	// To ensure we have the most up to date list, we'll call getFiles here to refresh the list.
	getFiles(FileList, shareFolder);

	int i;

	if (FileList->fileCount == 0) {
		// Print an error message if folder is empty or non existant
		fprintf(stderr, "Error: No files to list\n");

		return 1; // Indicates failure to read filelist

	} else {

		printf("\n");
		for(i=0;i<(FileList->fileCount); i++) {
		
			printf("%d.\t%s\n", i+1, FileList->sharedFiles[i]);
		}

		return 0; // Successful reading of filelist
	}
}

void printFileContent (int* fileNum, char* fileName, config_t* Config, int log) {
	// Read a hosted file and display it's contents on screen

	// To make the file path, we allocate enough space for both the directory and filename.
	char* filePath = malloc(strlen(Config->shareFolder) + strlen(fileName) + 2);
	char c;

	// Create the file path
	strcpy(filePath, Config->shareFolder);
	strcat(filePath, "/");
	strcat(filePath, fileName);

	printf("\nContents of %s\n------------------------------------------------------------\n\n", filePath);

	FILE* hostedFile;

	if (((checkAccess(filePath)) >= 4) && ((hostedFile = fopen(filePath, "r")) != NULL)) {
		// File exists with the correct permissions and opened with no errors

		c = fgetc(hostedFile);

		while (c != EOF) {

			printf("%c", c);
			c = fgetc(hostedFile);
		}

		fclose(hostedFile);
		logPipe("File contents outputted", log);

	} else {

		perror("Error - Could not open file");
		logPipe("Attempt to read file contents failed.", log);
	}

	printf("\n------------------------------------------------------------\n");
	free(filePath);
}

void readFileMenu(fileList_t* FileList, config_t* Config, int log) {
	// Displays a menu for the user to choose which file to output.

	int fileNum = 1;
	char fileNumberString[FILENUMBERSTRINGLEN]; 

	// List the current files
	if (listFiles(FileList, Config->shareFolder) == 0) {

		logPipe("Shared directory listed from read file menu", log);

		do {

			printf("\n[ q = quit to main menu | r = relist files ]\n\nSelect number of the file you wish to view (or option from above)\nSelection: ");
			getKeyboardInput(fileNumberString, FILENUMBERSTRINGLEN);
			fileNum = atoi(fileNumberString);

			if (strcmp(fileNumberString, "q") == 0) { // User wants to quit

			} else if (strcmp(fileNumberString, "r") == 0) {

				if (listFiles(FileList, Config->shareFolder) == 0) {
					logPipe("Shared directory relisted from read file menu", log);
				}
				continue;

			} else if ((fileNum <= 0) || (fileNum > FileList->fileCount)) {

				printf("Error: Invalid selection.\n\n");
				continue;

			} else {
				// Given input is valid, so print the file's contents
				printFileContent(&fileNum, FileList->sharedFiles[fileNum-1], Config, log);
			}

		} while (strcmp(fileNumberString, "q") != 0);


	}
}
