//
// Created by ezio on 09/08/19.
//

#define MAX_LOGS_PER_BRANCH MAX_CLI_PER_SB*20      //used to define the length of the vector used by a branch
                                                  //to collect the client logs

#define CLIENT_ACCEPTED 1                         //log types

#define CLIENT_REMOVED 2

#define CLIENT_SERVED 3

#define CLIENT_ERROR_READ 4

#define CLIENT_IDLE 5

#define CLIENT_MERGED 6

#define INTERNAL_SERVER_LOG 7

#define WRITE_ON_DISK_TIMER_SEC 5                //time after which logs of each server branch will
                                                 //be copied on disk

#define MAX_LOG_FILE_BYTE 100000000              //each log file will have a maximum number of bytes, 100MBytes

#define MAX_LOG_FILENAME_LEN 20

#define MAX_FILE_LOGS 2

#define MAX_LOG_LEN 50

#define CLIENT_ACCEPTED_S "Connection established\n"

#define CLIENT_REMOVED_S "Client has been removed\n"

#define CLIENT_ERROR_READ_S "Error in reading client request\n"

#define CLIENT_SERVED_S "Client has been served\n"

#define CLIENT_IDLE_S "Client kicked for inactivity\n"

#define CLIENT_MERGED_S "Client has been transfered to another server branch\n"

#define LOG_FILENAME "serverLog"

struct log{
    time_t log_time;
    int log_type;
    struct sockaddr_in client;
    char errorMess[MAX_LOG_LEN];
};
