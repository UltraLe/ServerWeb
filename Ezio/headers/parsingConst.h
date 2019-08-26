//
// Created by giovanni on 22/08/19.
//

#ifndef SERVERWEB_PARSING_H
#define SERVERWEB_PARSING_H

#endif //SERVERWEB_PARSING_H

#define OK  "200 OK"
#define NC  "204 No Content"
#define BR "400 Bad Request"
#define NF  "404 Not Found"
#define MNA  "405 Method Not Allowed"
#define ISE  "500 Internal Server Error"
#define VNS  "505 HTTP Version Not Supported"

#define TEXT "Content-Type: text/html\n"
#define JPEG "Content-Type: image/jpeg\n"
#define PNG "Content-Type: image/png\n"
#define NOTHING ""

#define ALIVE "keep-alive"
#define CLOSE "close"

#define PARSEHEADER(string, errorCode, type, lenght, status) sprintf(string, "HTTP/1.1 %s\nServer: Exodus\nVary: Accept-Encoding\n%sContent-Length: %ld\nConnection: %s\n\n", errorCode ,type ,lenght,status)

#define DATAERROR(data,error) sprintf(data,"<!DOCTYPE html><html><head><title>Page Error</title></head><body><h1 align=\"center\">Error %s!</h1></body></html>", error)

//TODO fuck2

struct paramResponse{
    char statusCode[31];
    char type[30];
    bool error;
    long payloadSize;
    long headerSize;
    bool head;
    int height;
    int width;
};