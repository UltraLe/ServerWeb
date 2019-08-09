//
// Created by ezio on 08/08/19.
//

#include <fcntl.h>

int loggerToWait = 0;
//struct log *sortedLogs;
char *sortedLogsString;

int timeLenTemplate;
int cliLenTemplate;



void releaseWaitingLoggers()
{
    for(int i = 0; i < loggerToWait; ++i){

        if(sem_post(&(info->sem_loggerManagerHasFinished)) == -1){
            perror("Error on sem_post (loggerManagerFinished)");
            exit(-1);
        }

    }
}



char *logToString(struct log currentLog)
{
    char *stringLog;


    return stringLog;
}



int writeLogsOnDisk()
{
    int fd;
    off_t currentOffset;

    if((fd = open(LOG_FILENAME, O_RDWR)) == -1){
        perror("Error in open (loggerManager)");
        return -1;
    }

    currentOffset = lseek(fd, SEEK_END, 0);

    if(currentOffset == (off_t)-1){
        perror("Error on lseek");
        exit(-1);
    }

    //writing logs
    if(write(fd, sortedLogsString, strlen(sortedLogsString)) == -1){
        perror("Error in writing on file");
        exit(-1);
    }

    close(fd);

}



int sortLoggersLogs()
{
    //vector of idexes
    int *branchIndex;

    branchIndex = (int *)malloc(sizeof(int)*loggerToWait);
    if(branchIndex == NULL){
        perror("Error in malloc (branchIndex\n");
        return -1;
    }

    //initializing indexes
    for(int i = 0; i < loggerToWait; ++i)
        branchIndex[i] = 0;

    //allocating space for the sorted stringLogs
    sortedLogsString = (char *)malloc(sizeof(char)*loggerToWait*MAX_LOGS_PER_BRANCH*(MAX_LOG_LEN+cliLenTemplate+timeLenTemplate));
    clock_t min = clock() + 1;
    clock_t tempLogClock;

    int j;
    int minIndex;

    struct log *tempLog;
    struct log minLog;

    for (int i = 0; i < loggerToWait; ) {

        //TODO increase i every time that a log of a server branch is completely read by the manager..
        //TODO .. increase i every time that log_time[branchIndex[i]] == (-1 || MAX_LOGS_PER BRANCHES)

        j = 0;
        minIndex = 0;

        for (struct branches_info_list *current = first_branch_info; current != NULL; current = current->next) {

            //main matter
            tempLog = ((current->info)->loggerLogs);
            tempLogClock = (tempLog[branchIndex[j]]).log_time;

            //if there are no more logs for the current branch,
            //increase i, and continue
            if(tempLog[branchIndex[j]].log_type == -1){
                ++i;
                continue;
            }

            if (tempLogClock < min) {
                min = tempLogClock;
                minIndex = j;
                minLog = *tempLog;
            }

            ++j;
        }

        //increasing index of branch that has the minimum (in clock) log
        branchIndex[minIndex]++;

        if(branchIndex[minIndex] == MAX_LOGS_PER_BRANCH)
            ++i;

        //here the minimum has been selected, converted and appended
        strcat(sortedLogsString, logToString(minLog));

    }




}



void loggerManager()
{
    timeLenTemplate = strlen("[Insert time]->");
    cliLenTemplate = strlen("[000.000.000.000:00000]");

    do{

        if(sem_wait(&semOnBranchesNum) == -1){
            perror("Error on sem_wait (semOnBranchesNum)");
            exit(-1);
        }

        loggerToWait = actual_branches_num;

        if(sem_post(&semOnBranchesNum) == -1){
            perror("Error on sem_post (semOnBranchesNum)");
            exit(-1);
        }

        //logger manager sleeps until 'actual_branches_num' logger are ready
        //to send the log of their clients

        for(int i = 0; i < loggerToWait; ++i){

            if(sem_wait(&(info->sem_awakeLoggerManager)) == -1){
                perror("Error on sem_wait (loggerManager)");
                exit(-1);
            }

        }

        //sorting logs
        if(sortLoggersLogs() == -1){
            printf("Error on sort loggers logs\n");
            exit(-1);
        }

        //writing on disk
        if(writeLogsOnDisk() == -1){
            printf("Error on sort loggers logs\n");
            exit(-1);
        }

        //releasing sortedLogsString
        free(sortedLogsString);

        //releasing loggers
        releaseWaitingLoggers();

    }while(1);

}
