/* File server application | Jack Nelson | CSP2308
 * networking.h
 * networking.c header file */

void serverStart(config_t* Config, int* serverStarted);
void connectionHandler(threadData_t* serverInfo);