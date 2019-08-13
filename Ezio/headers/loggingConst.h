//
// Created by ezio on 09/08/19.
//

#define MAX_LOGS_PER_BRANCH MAX_CLI_PER_SB*3      //used to define the length of the vector used by a branch
                                                  //to collect the client logs

#define CLIENT_ACCEPTED 1                         //log types

#define CLIENT_REMOVED 2

#define CLIENT_SERVED 3

#define CLIENT_ERROR_READ 4

#define CLIENT_DISCONNECTED 5

#define NO_ACTIVITY 0

#define WRITE_ON_DISK_TIMER_SEC 5                //time after which logs of each server branch will
                                                 //be copied on disk

#define MAX_LOG_LEN 50

#define CLIENT_ACCEPTED_S "Connection established\n"

#define CLIENT_REMOVED_S "Client has been removed\n"

#define CLIENT_ERROR_READ_S "Error in reading client request\n"

#define CLIENT_SERVED_S "Client has been served\n"

#define CLIENT_DISCONNECTED_S "Client disconnected from the server\n"

#define NO_ACTIVITY_S "Zero client has arrived since last log update\n"

#define LOG_FILENAME "serverLog.txt"

struct log{
    time_t log_time;
    int log_type;
    struct sockaddr_in client;
};
