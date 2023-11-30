#include <sys/un.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h> 
#include <netdb.h>
#include<ctype.h>
#include <poll.h>
#include <sys/poll.h>
#include<pthread.h>
#include <sys/types.h>
#include <ctype.h>
#include<stddef.h>

#define MX 1024

int main()
{
    int fd, len;
    struct sockaddr_un un;
    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return(-1);

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, "/tmp/train1");
    // len = offsetof(struct sockaddr_un, sun_path) + strlen(path);
    unlink(un.sun_path);
    if(bind(fd, (struct sockaddr *)&un, sizeof(un))<0)
    {
        printf("Bind failed...\n");
        return -1;
    }
            

    /* fill in socket address structure */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, "/tmp/server");
    // len = offsetof(struct sockaddr_un, sun_path) + strlen(name);

     
    if(connect(fd, (struct sockaddr *)&un, sizeof(un)) < 0)
    {
        printf("Connection failed...\n");
        return -1;
    }

    printf("Train 1 connected to the station master!\nStart entering text: ");    

    char buf[MX];
    while(1)
    {
        fgets(buf, sizeof(buf), stdin);
        send(fd, buf, sizeof(buf), 0);
        printf("Message sent...\n");  

        recv(fd, buf, sizeof(buf), 0);    
        printf("Message from server: %s\n", buf);
    }

    return 0;

}
