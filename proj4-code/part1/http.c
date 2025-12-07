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
    if (fgets(buf, BUFSIZE, socket_stream) == NULL) {
      fclose(socket_stream);
      return -1;
    }

    //char buf[BUFSIZE];
    //while (fgets(buf, BUFSIZE, socket_stream) != NULL) {
    //    if (strcmp(buf, "\r\n") == 0) {
    //        break;
    //    }
    //}

    if (ferror(socket_stream)) {
        perror("fgets");
        fclose(socket_stream);
        return -1;
    }

    if (fclose(socket_stream) != 0) {
        perror("fclose");
        return -1;
    }


    //int bytes_read;
    //while ((bytes_read = read(fd, buf, BUFSIZE)) > 0) { // this hangs because wait for read
    //    continue;
    //}

    int len = strlen(buf);
    for (int i = 0; i < len; i ++){
        if (buf[i] == '/'){
            int j = i;
            int k = 0;
            while (buf[j] != ' '){
                resource_name[k] = buf[j];
                j ++;
                k++;
            }
	    resource_name[k] = '\0';
            break;
        }
    }
    // this no beuno, closes for the larger calling function too!
    //if (close(fd) != 0) {
    //  perror("close");
    //    return -1;
    //}


    return 0;
}


int sendresponse(int status, int destination,int content_length, const char *target_resource) {	
	char message[BUFSIZE];
	if(status == 404){
		snprintf(message, sizeof(message),"HTTP/1.0 404 Not Found\r\n");
	}
	else {
		snprintf(message, sizeof(message), "HTTP/1.0 200 OK\r\n");
		snprintf(message + strlen(message), sizeof(message) - strlen(message), "Content-Type: %s\r\n", get_mime_type(&target_resource[strlen(target_resource)-4]));
	}
	snprintf(message + strlen(message), sizeof(message) - strlen(message), "Content-Length: %d", content_length); 
	snprintf(message + strlen(message), sizeof(message) - strlen(message), "\r\n\r\n"); 
	if (write(destination, message, strlen(message)) == -1){
		/// error checking?
		close(destination);// should this be here?
        	return -1;
    		}

	if (status == 200){ //technically we might not need this check but i wrote it so here we are
		int fd = open(target_resource, O_RDONLY);
		if (fd == -1){
			perror("open");
			return -1;
		}
		char buffer[BUFSIZE]; // we could reuse message.	
		ssize_t readin;
		while ((readin = read(fd, buffer, BUFSIZE)) > 0){
			if (write(destination, buffer, readin) == -1){
				perror("write to dest");
				close(fd);
				return -1;
			}
		}
		// do we need one more?
		if (readin == -1){
			perror("read");
			close(fd);
			return -1;
		}	
		close(fd);
	}
	return 0;
		
//	}
}
//TODO Calvin has been working on this.
int write_http_response(int fd, const char *resource_path) {
	
	//printf("before write dup");
	int sock_fd_copy = dup(fd);
	//printf("after write dup");
    	if (sock_fd_copy == -1) {
        	perror("dup");
        	return -1;
    	}

    	FILE *socket_stream = fdopen(sock_fd_copy, "w");
    	if (socket_stream == NULL) {
        	perror("fdopen");
        	close(sock_fd_copy);
        	return -1;
    	}
//    	if (setvbuf(socket_stream, NULL, _IOFBF, BUFSIZE) != 0) { //now buffering is better
 //       	perror("setvbuf");
 //	      	fclose(socket_stream);
  //      	return -1;
   // 	}
	struct stat sb;
	if(stat(resource_path,&sb) == -1){
		if (errno == ENOENT){
			sendresponse(404,sock_fd_copy, 0, 0);
		}
		else{ perror("stat");
		// should there be more here? #TODO should this return -1 on empty?
		close(sock_fd_copy);
		}
		return -1;
	}
	off_t content_length = sb.st_size;
	sendresponse(200, sock_fd_copy, (int) content_length, resource_path); // this might need to be a pointer 
	
	//sb.st_size; ??


    return 0;
}


