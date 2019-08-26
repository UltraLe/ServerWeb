//
// Created by ezio on 16/07/19.
//
#include <wurfl/wurfl.h>
#include <stdbool.h>
#include <fcntl.h>
#include "parsingConst.h"


char *response;
struct paramResponse pertica;
char *image_bufffer;

void resolutionPhone(char *request){
    char userAgent[300];
    wurfl_error error;

    wurfl_handle hwurfl = wurfl_create();

    // Define wurfl db
    error = wurfl_set_root(hwurfl, "/usr/share/wurfl/wurfl.zip");
    if (error != WURFL_OK) {
        printf("%s\n", wurfl_get_error_message(hwurfl));
        return;
    }

    // Loads wurfl db into memory
    error = wurfl_load(hwurfl);
    if (error != WURFL_OK) {
        printf("%s\n", wurfl_get_error_message(hwurfl));
        return;
    }

    char *UAstart = strstr(request, "User-Agent:") +12;
    strncpy(userAgent,UAstart,strstr(UAstart,"\n")-UAstart);

    wurfl_device_handle hdevice = wurfl_lookup_useragent(hwurfl, userAgent);
    if (hdevice) {
        // DEVICE FOUND
        pertica.width = atoi(wurfl_device_get_capability(hdevice, "resolution_width"));
        pertica.height = atoi(wurfl_device_get_capability(hdevice, "resolution_height"));

        //printf("pertica.widht = %d\n", pertica.width);
        //printf("brand_name = %d\n\n\n", pertica.height);
        //printf("brand_name = %s\n", wurfl_device_get_capability(hdevice, "resolution_width"));
        //printf("brand_name = %s\n", wurfl_device_get_capability(hdevice, "resolution_width"));}
    wurfl_device_destroy(hdevice);
    wurfl_destroy(hwurfl);

}}

char *takeFile(char *path){

    int im_fd;

    printf("Path: %s\n",path+1);

    if((im_fd = open(path+1, O_RDONLY)) == -1){
        perror("Unable to open the image");
        exit(-1);
    }

    int IMAGE_SIZE = 900000;



    int image_byte = lseek(im_fd, 0, SEEK_END);
    image_bufffer = (char *)malloc(IMAGE_SIZE);

    lseek(im_fd, 0, SEEK_SET);

    //TODO fuck

    if((pertica.payloadSize = read(im_fd, image_bufffer, IMAGE_SIZE)) == -1){
        perror("Error in reading the image");
        exit(-1);
    }

    printf("\tImage read correctly in takeFile\n%s\n", image_bufffer);

    printf("Image length: %d byte\n", image_byte);

    return NULL;


    /*
    //printf("%s\n",path+1);
    FILE* file = fopen(path+1, "rb");

    if(file == NULL){
        perror("file non trovato");
        pertica.error = true;
        strcpy(pertica.statusCode, NF);
        return NULL;
    }

    int fs = fseek(file, 0, SEEK_END);

    printf("Image dimension: %d\n", fs);

    if (fs == -1){
        perror("Positioning stream pointer error");
        pertica.error = true;
        strcpy(pertica.statusCode,ISE);
        return NULL;
    }

    unsigned long fileLen = ftell(file);

    printf("Image dimension wih ftell: %d\n", fileLen);

    if (fileLen == -1){
        perror("ftell error");
        pertica.error = true;
        strcpy(pertica.statusCode,ISE);
        return NULL;
    }

    pertica.lenght = fileLen;

    char* file_data;

    rewind(file);

    file_data = malloc((fileLen)*sizeof(char));

    if (file_data == NULL){
        perror("Memory error");
        pertica.error = true;
        strcpy(pertica.statusCode,ISE);
        return NULL;
    }

    char s;
    while (fread(&s, 1, 1, file)) {
        strncat(file_data,&s,1);
    }

    //printf("file contents: %s\n", file_data);
//    printf("file contents: %s\n", "FAMMELO VEDERE");

    fclose(file);
    return file_data;
     */
}

char *uriAnalyzer(char *request){
    char buff[2000];
    char *token ;

    strcpy(buff,request);
    strtok(buff, " ");
    token = strtok(NULL, " ");
    //  printf("Io son il token %s\n",token);

    if((strncmp(token,"/",1) == 0) && strlen(token)==1){
        return "";
    }
    else if (strncmp(token,"/",1)==0){
        return token;

    }

    //Se si arriva qui la richiesta risulta errata
    pertica.error = true;
    strcpy(pertica.statusCode, BR);

    return token;

}

char *sendError(){
    char header[200];
    char data[400];

    DATAERROR(data, pertica.statusCode);
    PARSEHEADER(header,pertica.statusCode,TEXT,strlen(data),CLOSE);
    response = (char *)malloc(500* sizeof(char));
    if(!response){
        perror("non sono riuscito ad allocare");
    }
    strcpy(response,header);
    strcat(response,data);

    //printf("\n\n\n-------------------ecco il pacchetto----------------------\n%s\n", response);

    return response;


}
char *parsingManager(char *request) {

    char path[2048];
    char data[900001];

    memset(data, 0, sizeof(data));

    printf("In parsingManager:\n%s\n", request);
    printf("Con puts: ");
    puts(request);


    if (strncmp(request, "GET", 3) == 0) {
        //Going over the if

    } else if (strncmp(request, "HEAD", 4) == 0) {
        pertica.head = true;

    } else if (strncmp(request, "POST", 4) == 0 ||
               strncmp(request, "DELETE", 6) == 0 ||
               strncmp(request, "PUT", 3) == 0 ||
               strncmp(request, "TRACE", 5) == 0 ||
               strncmp(request, "OPTIONS", 7) == 0) {
        pertica.error = true;
        strcpy(pertica.statusCode, MNA);

    } else {
        pertica.error = true;
        strcpy(pertica.statusCode, BR);
    }

    if (pertica.error) {
        return sendError();
    }


    //Qui vado a creare i data

    strcpy(path, uriAnalyzer(request));

    if (pertica.error) {
        return sendError();
    }

    printf("Path trovato: %s\n", path);

    //home page requested
    if (strcmp(path, "") == 0) {
        strcpy(pertica.type, TEXT);

        //TODO pagina HTML da aggiungere in memoria condivisa?
        memcpy(data, takeFile("/sito/WebPage.html"),pertica.payloadSize); // provvisorio

    } else if (strncmp(path, "/sito", 5) == 0) {
        strcpy(pertica.type, PNG);
        memcpy(data, takeFile(path),pertica.payloadSize);

        //     printf("%s\n", data);

    } else if (strncmp(path, "/Images/", 7) == 0) {
        resolutionPhone(request);
        //TODO Inserire funzione resizing

       //PROVVISORIO
        memcpy(data+1,image_bufffer,pertica.payloadSize);

        printf("\tImage read correctly in parsingManager\n%s\n", data);

        strcpy(pertica.type,PNG);

    } else {
        pertica.error = true;
        strcpy(pertica.statusCode, NF);
    }

    if (pertica.error) {
        return sendError();
    }

    int responseSize = sizeof(char)*pertica.payloadSize+300;

    response = (char *)malloc(responseSize);

    memset(response, 0, responseSize);

    PARSEHEADER(response,OK,pertica.type,pertica.payloadSize,ALIVE);

    printf("Response ready:\n%s\n", response);

    pertica.headerSize = strlen(response);

    //printf("%ld\n",strlen(data));
    //printf("%ld\n",pertica.lenght);
    if(!pertica.head){
        printf("GET method, inserting image\n");
        memcpy(response + pertica.headerSize, data, pertica.payloadSize );
        //strcat(response,"\n\n");

    }

    //printf("\n\n\n-------------------ecco il pacchetto----------------------\n%s\n", response);
    //printf("%ld\n",pertica.lenght);
    return response;
}