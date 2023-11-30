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


int recv_fd(int socket)
 {
  int sent_fd, available_ancillary_element_buffer_space;
  struct msghdr socket_message;
  struct iovec io_vector[1];
  struct cmsghdr *control_message = NULL;
  char message_buffer[1];
  char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];

  /* start clean */
  memset(&socket_message, 0, sizeof(struct msghdr));
  memset(ancillary_element_buffer, 0, CMSG_SPACE(sizeof(int)));

  /* setup a place to fill in message contents */
  io_vector[0].iov_base = message_buffer;
  io_vector[0].iov_len = 1;
  socket_message.msg_iov = io_vector;
  socket_message.msg_iovlen = 1;

  /* provide space for the ancillary data */
  socket_message.msg_control = ancillary_element_buffer;
  socket_message.msg_controllen = CMSG_SPACE(sizeof(int));

  if(recvmsg(socket, &socket_message, MSG_CMSG_CLOEXEC) < 0)
   return -1;

  if(message_buffer[0] != 'F')
  {
   /* this did not originate from the above function */
   return -1;
  }

  if((socket_message.msg_flags & MSG_CTRUNC) == MSG_CTRUNC)
  {
   /* we did not provide enough space for the ancillary element array */
   return -1;
  }

  /* iterate ancillary elements */
   for(control_message = CMSG_FIRSTHDR(&socket_message);
       control_message != NULL;
       control_message = CMSG_NXTHDR(&socket_message, control_message))
  {
   if( (control_message->cmsg_level == SOL_SOCKET) &&
       (control_message->cmsg_type == SCM_RIGHTS) )
   {
    sent_fd = *((int *) CMSG_DATA(control_message));
    return sent_fd;
   }
  }

  return -1;
 }

void* advertisment()
{
    // int s = socket (PF_INET, SOCK_RAW, IPPROTO_UDP);
    // char datagram[4096];	
    // struct ip *iph = (struct ip *) datagram; 
    // struct sockaddr_in sin; 
    // socklen_t l = sizeof(sin) ;

    // memset(&sin,0,sizeof(sin)) ;
    // sin.sin_family = AF_INET;
    // if(inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr)<=0) 
    //     { 
    //         printf("\nInvalid address/ Address not supported \n");             
    //     }     
    // bind(s,(struct sockaddr *) &sin,sizeof(sin));

    while(1)
    {
        
        // int n;
        // if( (n = recvfrom(s,datagram,4096,MSG_WAITALL,(struct sockaddr *) &sin,&l) ) < 0)	
        //     printf ("error\n");
        // else
            printf("Received an advertisement...\n");
            sleep(3);

    } 
}

int main()
{
    int fd, len;
    pthread_t th;
    printf("My pid is: %d\n", getpid());
    struct sockaddr_un un, cli_un;
    /* create a UNIX domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        {
            printf("socket failed");
            return -1;
        }              

    /* fill in socket address structure */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, "/tmp/alt_server0");
    unlink(un.sun_path);

    if(bind(fd, (struct sockaddr *)&un, sizeof(un))<0)
    {
        printf("Bind failed");
        return -1;
    }        



    //Now wait for client connection.
    if(listen(fd, 3)<0)
    {
        printf("Listen fail...");
        return -1;
    }

    printf("Platform 1 waiting for connections...\n");

    len = sizeof(cli_un);
    int sfd;
    if((sfd = accept(fd, (struct sockaddr *)&cli_un, (socklen_t*)&len)) < 0)
    {
        printf("Accept failed");
        return -1;
    }

    printf("Station master connection accepted!\n"); 
    pthread_create(&th, NULL, advertisment, NULL);   

    fd_set readfds;
    int client_fd;
    int flag=0;
    int max_fd, k=0;

    while(1)
    {
        FD_ZERO(&readfds);
        
        //Set the sfd. If there is any activity, then it means that there is an incoming cfd.
        max_fd = sfd;
        FD_SET(sfd, &readfds);        

        if(flag)
        {
            FD_SET( client_fd , &readfds); 
			if(client_fd > max_fd) 
				max_fd = client_fd; 
        } 

        int activity = select( max_fd + 1 , &readfds , NULL , NULL , NULL);

        if(activity<=0)
        {
            printf("select failed...");
            return -1;
        }

        if(FD_ISSET(sfd, &readfds))
        {
            //There is an incoming cfd.
            client_fd = recv_fd(sfd);            
            flag = 1;

        }        
        			 				
        if (FD_ISSET(client_fd,&readfds)) 
        { 
            char buf[MX];
            if(read(client_fd, buf, sizeof(buf)) == 0)
            {
                // Train has left the platform.
                close(client_fd);
                flag = 0;
                memset(buf, ' ', sizeof(buf));
                // Notify the station master.
                send(sfd, buf, sizeof(buf), 0);
            }

            else
            {
                printf("Platform 1 received the message: %s\n", buf);

                //Capitalize the text.
                for(int i=0;buf[i]!='\0';i++)
                {
                    if(isalpha(buf[i]))
                    buf[i] = toupper(buf[i]);
                }

                send(client_fd, buf, sizeof(buf), 0);
                printf("Message sent...\n");
            }
            
        } 	
    }



        

    return 0;

}
