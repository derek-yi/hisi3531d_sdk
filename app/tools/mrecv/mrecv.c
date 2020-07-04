#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#define N 128

typedef struct sockaddr SA;

int main(int argc, const char *argv[])
{
    int sockfd;
    
    /**创建用户数据包套接字**/
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(-1);
    }
        
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr("239.0.0.1");
    myaddr.sin_port = htons(50001);
    
    /**绑定多播IP**/
    if(bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1)
    {
        perror("bind");
        exit(-1);
    }
    
    /*
    ** struct ip_mreq  {
    **     struct in_addr imr_multiaddr;   IP multicast address of group 设置多播组地址
    **     struct in_addr imr_interface;   local IP address of interface  本机IP
    ** };
    */
    /**加入多播组**/
    struct ip_mreq mreq;
    bzero(&mreq, sizeof(mreq));
    mreq.imr_multiaddr.s_addr = inet_addr("239.0.0.1");
    //mreq.imr_interface.s_addr = inet_addr("172.16.81.122");
    mreq.imr_interface.s_addr = INADDR_ANY; //INADDR_ANY
        
    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt");
        exit(-1);
    }

    char buf[N] = {0};
    while(1)
    {
        /**等待接收数据**/
        recvfrom(sockfd, buf, N, 0, NULL, NULL);
        printf("recv : %s\n", buf);
    }

    close(sockfd);
    return 0;
}

