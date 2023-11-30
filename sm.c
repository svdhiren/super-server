#include <sys/un.h>
#include <unistd.h> 
#include <stdio.h> 
#include<fcntl.h>
#include<sys/wait.h>
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
#define m 1

int send_fd(int socket, int fd_to_send)
 {
  struct msghdr socket_message;
  struct iovec io_vector[1];
  struct cmsghdr *control_message = NULL;
  char message_buffer[1];
  /* storage space needed for an ancillary element with a paylod of length is CMSG_SPACE(sizeof(length)) */
  char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
  int available_ancillary_element_buffer_space;

  /* at least one vector of one byte must be sent */
  message_buffer[0] = 'F';
  io_vector[0].iov_base = message_buffer;
  io_vector[0].iov_len = 1;

  /* initialize socket message */
  memset(&socket_message, 0, sizeof(struct msghdr));
  socket_message.msg_iov = io_vector;
  socket_message.msg_iovlen = 1;

  /* provide space for the ancillary data */
  available_ancillary_element_buffer_space = CMSG_SPACE(sizeof(int));
  memset(ancillary_element_buffer, 0, available_ancillary_element_buffer_space);
  socket_message.msg_control = ancillary_element_buffer;
  socket_message.msg_controllen = available_ancillary_element_buffer_space;

  /* initialize a single ancillary data element for fd passing */
  control_message = CMSG_FIRSTHDR(&socket_message);
  control_message->cmsg_level = SOL_SOCKET;
  control_message->cmsg_type = SCM_RIGHTS;
  control_message->cmsg_len = CMSG_LEN(sizeof(int));
  *((int *) CMSG_DATA(control_message)) = fd_to_send;

  return sendmsg(socket, &socket_message, 0);
 }


int check(int occupied[])
{
    for(int i=0;i<m;i++)
        if(!occupied[i])
            return 1;
    return 0;
}

int main()
{
    int max_fd, as_fd[m], fd, udp_fd, len;
    struct sockaddr_un un, cli_un, as_un[m], udp_un;
    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
            printf("socket failed");
            return -1;
        }     
    
    for(int i=0;i<m;i++)
    {
        if ((as_fd[i] = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
            printf("alternate socket %d failed", i);
            return -1;
        }
    }            

    /* Activating the main server's fd */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, "/tmp/server");
    unlink(un.sun_path);

    if(bind(fd, (struct sockaddr *)&un, sizeof(un))<0)
    {
        printf("Bind failed");
        return -1;
    }    
    
    /* Activating the as_fd from main server to contact with alt server.*/

    char *path[] = {"/tmp/alt_server0", "/tmp/alt_server2", "/tmp/alt_server3"};
    for(int i=0;i<m;i++)
    {
        memset(&as_un[i], 0, sizeof(as_un));
        as_un[i].sun_family = AF_UNIX;
        strcpy(as_un[i].sun_path, path[i]);

        if(connect(as_fd[i], (struct sockaddr *)&as_un, sizeof(as_un)) < 0)
        {
            printf("Alternate server %d connection failed...\n", i);
            return -1;
        }
    }

    printf("Main server connected to all alternate servers...\n");            

    //Now wait for client connection.
    if(listen(fd, 3)<0)
    {
        printf("Listen fail...");
        return -1;
    }

    printf("Main server waiting for connections...\n");

    len = sizeof(cli_un);
    int cfd;
    int occupied[m] = {0};
    

    fd_set readfds;


    while(1)
    {

        FD_ZERO(&readfds);

        if(check(occupied))
        {
            max_fd = fd;
            FD_SET(fd, &readfds); 
        }
        else
            max_fd = as_fd[0];

        for(int i=0;i<m;i++)
        {
            if(as_fd[i]>max_fd)
                max_fd = as_fd[i];
            FD_SET(as_fd[i], &readfds); 
        }

        int activity = select( max_fd + 1 , &readfds , NULL , NULL , NULL);

        if(activity<=0)
        {
            printf("select failed...");
            return -1;
        }

        for(int i=0;i<m;i++)
        {
            if(FD_ISSET(as_fd[i], &readfds))
            {
                // That means a train has left the platform i.
                char buf[MX];
                recv(as_fd[i], buf, sizeof(buf), 0);
                occupied[i] = 0;
                printf("A train has left platform number %d\n", i);
            }
        }

        if(FD_ISSET(fd, &readfds))
        {
            if((cfd = accept(fd, (struct sockaddr *)&cli_un, (socklen_t*)&len)) < 0)
            {
                printf("Accept failed");
                return -1;
            }

            printf("Train connection accepted!\n");

            
            for(int i=0;i<m;i++)
            {
                if(!occupied[i])
                {
                    occupied[i] = 1;
                    send_fd(as_fd[i], cfd);
                    printf("Train from %s has been sent to platform %d\n", cli_un.sun_path, i+1);
                }
            }
        }
        
        // printf("cfd sent to platform server...\n\n"); 
    }   
    
    return 0;

}
