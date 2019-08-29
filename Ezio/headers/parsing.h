//
// Created by ezio on 16/07/19.
//

#include <stdbool.h>
#include <fcntl.h>
#include "parsingConst.h"


char *response;
struct paramResponse setting;
char *payloadBuffer;

void acceptAnalyzer(char *request) {
    setting.quality = -1;
    char *attribute;
    char q[4];
    memset(q, 0, 4);

    double qPng = 0, qJpeg = 0, qAll = 0, qImages = 0;
    char accept[200];
    memset(accept, 0, 200);

    char *acceptStart = strstr(request, "Accept:");
    if(acceptStart == NULL){
        perror("Not found Accept line");
        setting.error = true;
        strcpy(setting.statusCode, BR);
        return;
    }

    acceptStart = acceptStart+8;
    strncpy(accept, acceptStart, strstr(acceptStart, "\n") - 1 - acceptStart);

    if ((attribute = strstr(accept, "image/png")) != NULL) {
        if (!strncmp(attribute + 9, ";q", 2)) {
            qPng = atof(strncpy(q, attribute + 12, 3));
        } else qPng = 1;
    }

    if ((attribute = strstr(accept, "image/jpeg")) != NULL) {
        if (!strncmp(attribute + 10, ";q", 2)) {
            qJpeg = atof(strncpy(q, attribute + 13, 3));
        } else qJpeg = 1;
    }

    if (qJpeg == 0 || qPng == 0) {
        if ((attribute = strstr(accept, "image/*")) != NULL) {
             if (!strncmp(attribute + 7, ";q", 2)) {
                 qImages = atof(strncpy(q, attribute + 10, 3));
             } else qImages = 1;
        }
    }
    
    if ((qJpeg==0 || qPng ==0) && qImages ==0){
        if ((attribute = strstr(accept, "*/*")) != NULL) {

            if (!strncmp(attribute + 3, ";q", 2)) {
                qAll = atof(strncpy(q, attribute + 6, 3));
            } else {
                qAll = 1;
            }
        }
    }

    if (qAll== qJpeg && qAll == qPng){
        strcpy(setting.type,NOTHING);
        setting.quality = qAll;
    }
    else if (qJpeg == qPng){
        strcpy(setting.type,NOTHING);
        setting.quality = qPng;
    }
    else {
        double qMax= qAll;
        strcpy(setting.type,NOTHING);
        setting.quality = qAll;


        if(qJpeg > qMax){
            strcpy(setting.type,JPEG);
            setting.quality = qJpeg;
            qMax= qJpeg;

        }
        if (qPng > qMax){
            strcpy(setting.type,PNG);
            setting.quality = qPng;
            qMax = qPng;

        }
        if (qImages >= qMax){
            strcpy(setting.type,NOTHING);
            setting.quality = qImages;

        }
    }
    if (qAll == 0 && qJpeg ==0 && qPng == 0 && qImages == 0){
        setting.error = true;
        strcpy(setting.statusCode, NA);
    }
}

void resolutionPhone(char *request){
    char userAgent[300];
    wurfl_error error;

    char *UAstart = strstr(request, "User-Agent:");
    if(UAstart == NULL){
        perror("Not found User-Agents");
        setting.error = true;
        strcpy(setting.statusCode, BR);
        return;
    }
    UAstart = UAstart +12;
    strncpy(userAgent,UAstart,strstr(UAstart,"\n")-UAstart);

    wurfl_device_handle hdevice = wurfl_lookup_useragent(handler_info->hwurfl, userAgent);
    if (hdevice) {
        // DEVICE FOUND
        setting.width = atoi(wurfl_device_get_capability(hdevice, "resolution_width"));
        setting.height = atoi(wurfl_device_get_capability(hdevice, "resolution_height"));

    wurfl_device_destroy(hdevice);

}}

char *takeFile(char *path){
    int im_fd;

    printf("Path: %s\n",path+1);

    if((im_fd = open(path+1, O_RDONLY)) == -1){
        perror("file not Found");
        setting.error = true;
        strcpy(setting.statusCode, NF);
        return NULL;
    }

    int image_byte = lseek(im_fd, 0, SEEK_END);
    payloadBuffer = (char *)malloc(image_byte);

    lseek(im_fd, 0, SEEK_SET);

    if((setting.payloadSize = read(im_fd, payloadBuffer, image_byte)) == -1){
        perror("Error in reading the image");
        setting.error = true;
        strcpy(setting.statusCode, ISE );
        return NULL;
    }

    return NULL;
}

char *uriAnalyzer(char *request){
    char buff[2000];
    char *pathToken ;
    char *HTTPToken;

    strcpy(buff,request);
    strtok(buff, " \n\r");
    pathToken = strtok(NULL, " \n\r");
    HTTPToken = strtok(NULL," \n\r");

    if ((HTTPToken != NULL) && !(strcmp(HTTPToken,"HTTP/1.0") == 0 ||
                            strcmp(HTTPToken,"HTTP/1.1") == 0 ||
                            strcmp(HTTPToken,"HTTP/2.0")== 0)) {

        setting.error = true;
        strcpy(setting.statusCode, BR);
        return pathToken;
    }


    if(strcmp(pathToken,"/") == 0) {
        return "";
    }
    else if (strncmp(pathToken,"/",1)==0 && strlen(pathToken)>1){
        return pathToken;
    }
    //If you arrive here the request's path is incorrect
    setting.error = true;
    strcpy(setting.statusCode, BR);

    return pathToken;

}

char *sendError(){
    char header[200];
    char data[400];

    DATAERROR(data, setting.statusCode);
    setting.payloadSize = strlen(data);
    PARSEHEADER(header,setting.statusCode,TEXT,setting.payloadSize,CLOSE);
    setting.headerSize = strlen(header);

    response = (char *)malloc(500* sizeof(char));
    strcpy(response,header);
    strcat(response,data);
    return NULL ;

}
char *parsingManager(char *request) {

    char path[2048];
    char data[900001];

    memset(data, 0, sizeof(data));

    printf("In parsingManager:\n%s\n", request); //DA TOGLIERE

    //Check that the method is acceptable
    if (strncmp(request, "GET ", 4) == 0) {
        //Going over the if

    } else if (strncmp(request, "HEAD ", 5) == 0) {
        setting.head = true;

    } else if (strncmp(request, "POST ", 5) == 0 ||
               strncmp(request, "DELETE ", 7) == 0 ||
               strncmp(request, "PUT ", 4) == 0 ||
               strncmp(request, "TRACE ", 6) == 0 ||
               strncmp(request, "OPTIONS ", 8) == 0) {
        setting.error = true;
        strcpy(setting.statusCode, MNA);

    } else {
        setting.error = true;
        strcpy(setting.statusCode, BR);
    }

    if (setting.error) {
        return sendError();
    }

    //I check that the path is correct
    strcpy(path, uriAnalyzer(request));

    if (setting.error) {
        return sendError();
    }

    //home page requested
    if (strcmp(path, "") == 0) {
        strcpy(setting.type, TEXT);
        takeFile("/sito/WebPage.html");

    } else if (strncmp(path, "/sito", 5) == 0) {
        strcpy(setting.type, PNG);
        takeFile(path);

    } else if (strncmp(path, "/Images/", 7) == 0) {
        resolutionPhone(request);
        if (setting.error) {
            return sendError();
        }

        acceptAnalyzer(request);
        if (setting.error) {
            return sendError();
        }
        //TODO Inserire funzione resizing

        takeFile(path);
        //strcpy(setting.type,JPEG);//Lo setta riccardo

    } else {
        setting.error = true;
        strcpy(setting.statusCode, NF);
    }

    if (setting.error) {
        return sendError();
    }

    int responseSize = sizeof(char)*setting.payloadSize+300;

    response = (char *)malloc(responseSize);
    memset(response, 0, responseSize);

    // I set the header
    PARSEHEADER(response,OK,setting.type,setting.payloadSize,ALIVE);
    setting.headerSize = strlen(response);

    //I check if the metod is HEAD
    if(!setting.head){
        memcpy(response + setting.headerSize, payloadBuffer, setting.payloadSize );
    }
    return response;
}