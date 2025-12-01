#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    } else if (strcmp(".mp3", file_extension) == 0) {
        return "audio/mpeg";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    int sock_fd_copy = dup(fd);
    if (sock_fd_copy == -1) {
        perror("dup");
        return -1;
    }
    FILE *socket_stream = fdopen(sock_fd_copy, "r");
    if (socket_stream == NULL) {
        perror("fdopen");
        close(sock_fd_copy);
        return -1;
    }
    if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        fclose(socket_stream);
        return -1;
    }

    char buf[BUFSIZE];
    while (fgets(buf, BUFSIZE, socket_stream) != NULL) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
    }

    if (ferror(socket_stream)) {
        perror("fgets");
        fclose(socket_stream);
        return -1;
    }

    if (fclose(socket_stream) != 0) {
        perror("fclose");
        return -1;
    }


    int bytes_read;
    while (read(fd, buf, BUFSIZE) > 0) {
        continue;
    }

    int len = strlen(buf);
    for (int i = 0; i < len; i ++){
        if (buf[i] == "/"){
            int j = i;
            int k = 0;
            while (buf[j] != " "){
                resource_name[k] = buf[j];
                j ++;
                k++;
            }
            break;
        }
    }

    if (close(fd) != 0) {
        perror("close");
        return -1;
    }


    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    // TODO Not yet implemented
    return 0;
}
