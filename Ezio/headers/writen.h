//
// Created by ezio on 18/07/19.
//

//
// Created by ezio on 18/07/19.
//

#include <errno.h>

ssize_t writen(int fd, const void *buf, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = buf;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if ((nwritten < 0) && (errno == EINTR)) nwritten = 0;
            else return(-1);
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return(n-nleft);
}