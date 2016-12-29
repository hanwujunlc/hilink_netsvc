#include "hilink_socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define MULTICAST_GROUP_ADDR4 "238.238.238.238"

int hilink_udp_new(unsigned short lport)
{
    struct sockaddr_in servaddr;
    struct ip_mreq group;
    int fd = HILINK_SOCKET_NO_ERROR;
    int flags;
    int reuse;
	
	/*创建socket*/
    fd = socket(AF_INET,SOCK_DGRAM,0);
    if(fd < 0)
    {
        printf("creat socket fd failed\n");
        return HILINK_SOCKET_CREAT_UDP_FD_FAILED;
    }
	
	/*设置非阻塞模式*/
    flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        printf("fcntl: %s\n", strerror(errno));
        close(fd);
        return HILINK_SOCKET_CREAT_UDP_FD_FAILED;
    }

    if(lport != 0)
    {
        reuse = 1;
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
            (const char *)&reuse, sizeof( reuse ) ) != 0)
        {
            close(fd);
            printf("set SO_REUSEADDR failed\n");
            return HILINK_SOCKET_CREAT_UDP_FD_FAILED;
        }

        memset(&servaddr, 0, sizeof(struct sockaddr_in));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(lport);
        if(bind(fd,(struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) < 0)
        {
            close(fd);
            printf("bind port %u failed\n",lport);
            return HILINK_SOCKET_CREAT_UDP_FD_FAILED;
        }

		/*加入组播组*/
        memset(&group, 0, sizeof(struct ip_mreq));
        group.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP_ADDR4);
        group.imr_interface.s_addr = htonl(INADDR_ANY);
        if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group,
            sizeof(group)) < 0)
        {
            close(fd);
            printf("set multi group failed\n");
            return HILINK_SOCKET_CREAT_UDP_FD_FAILED;
        }
    }

    return fd;
}

int hilink_udp_remove_multi_group(int fd)
{
    struct ip_mreq group;
    int ret = HILINK_SOCKET_NO_ERROR;
    
    memset(&group, 0, sizeof(struct ip_mreq));
    group.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP_ADDR4);
    group.imr_interface.s_addr = htonl(INADDR_ANY);
    if(setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &group,
        sizeof(group)) < 0)
    {
        printf("remove from multi group failed\n");
        ret = HILINK_SOCKET_REMOVE_UDP_FD_FAILED;
    }

    return ret;
}

void hilink_udp_remove(int fd)
{
    close(fd);
}

int hilink_udp_send(int fd, const unsigned char* buf, unsigned short len,
        const char* rip, unsigned short rport)
{
    struct sockaddr_in dstaddr;
    int ret = -1;

    if(buf == NULL || rip == NULL)
    {
        return HILINK_SOCKET_NULL_PTR;
    }

    memset(&dstaddr, 0, sizeof(struct sockaddr_in));
    dstaddr.sin_addr.s_addr = inet_addr(rip);
    dstaddr.sin_port = htons(rport);

	/*UDP发送数据需要判断错误码*/
    ret = (int)(sendto(fd, buf, len, 0, (struct sockaddr*)&dstaddr,
            sizeof(struct sockaddr_in)));
    if(ret < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return HILINK_SOCKET_NO_ERROR;
        }
        else
        {
            return HILINK_SOCKET_SEND_UDP_PACKET_FAILED;
        }
    }

    return ret;
}

int hilink_udp_read(int fd, unsigned char* buf, unsigned short len,
         char* rip, unsigned short riplen, unsigned short* rport)
{
    struct sockaddr_in dstaddr;
    int length = 0;
    unsigned int addrlen = sizeof(dstaddr);

    if(buf == NULL || rip == NULL || rport == NULL)
    {
        return HILINK_SOCKET_NULL_PTR;
    }

	/*UDP读取数据需要判断错误码*/
    length = (int)(recvfrom(fd, buf, len, 0,
                (struct sockaddr*)&dstaddr, &addrlen));
    if(length <= 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            //printf("recv no data,continue reading\n");
            return HILINK_SOCKET_NO_ERROR;
        }
        else
        {
            return HILINK_SOCKET_READ_UDP_PACKET_FAILED;
        }
    }

    buf[length] = '\0';
    printf("udp recv data:%s,length:%d\n", buf, length);

    strncpy(rip, inet_ntoa(dstaddr.sin_addr), riplen);
    *rport = ntohs(dstaddr.sin_port);

    printf("remote ip:%s,port:%u\n", rip, *rport);

    return length;

}

int hilink_tcp_connect(const char* dst, unsigned short port)
{
    struct sockaddr_in servaddr;
    int fd;
    int flags;
    int reuse;

    if(NULL == dst)
    {
        return HILINK_SOCKET_NULL_PTR;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        printf("creat socket fd failed\n");
        return HILINK_SOCKET_TCP_CONNECT_FAILED;
    }

	/*设置非阻塞模式*/
    flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        printf("fcntl: %s\n", strerror(errno));
        close(fd);
        return HILINK_SOCKET_TCP_CONNECT_FAILED;
    }

    reuse = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                    (const char *) &reuse, sizeof( reuse ) ) != 0 )
    {
        close(fd);
        printf("set SO_REUSEADDR failed\n");
        return HILINK_SOCKET_TCP_CONNECT_FAILED;
    }

    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(dst);
    servaddr.sin_port = htons(port);
    if(connect(fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) == 0)
    {
        return fd;
    }
    else
    {
        if(errno == EINPROGRESS)
        {
            printf("tcp conncet noblock\n");
            return fd;
        }
        else
        {
            close(fd);
            return HILINK_SOCKET_TCP_CONNECT_FAILED;
        }
    }
}

void hilink_tcp_disconnect(int fd)
{
    close(fd);
}

int hilink_tcp_send(int fd, const unsigned char* buf, unsigned short len)
{
    int ret = -1;

    if(buf == NULL)
    {
        return HILINK_SOCKET_NULL_PTR;
    }

	/*TCP发送数据需要判断错误码*/
    ret = (int)(send(fd, buf, len, MSG_DONTWAIT));
    if(ret < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return HILINK_SOCKET_NO_ERROR;
        }
        else
        {
            return HILINK_SOCKET_SEND_TCP_PACKET_FAILED;
        }
    }

    return ret;
}

int hilink_tcp_state(int fd)
{
    int errcode = HILINK_SOCKET_NO_ERROR;
    int tcp_fd = fd;
    if(tcp_fd < 0) {
        return HILINK_SOCKET_TCP_CONNECT_FAILED;
    }
    fd_set rset, wset;
    int ready_n;

    FD_ZERO(&rset);
    FD_SET(tcp_fd, &rset);
    wset = rset;

    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

	/*使用select机制判断tcp连接状态*/
    ready_n = select(tcp_fd + 1, &rset, &wset, NULL, &timeout);
    if(0 == ready_n)
    {
        printf("select time out\n");  
        errcode = HILINK_SOCKET_TCP_CONNECTING;         
    }
    else if(ready_n < 0)
    {
        printf("select error\n");  
        errcode = HILINK_SOCKET_TCP_CONNECT_FAILED; 
    }
    else
    {
        printf("FD_ISSET(tcp_fd, &rset):%d\n FD_ISSET(tcp_fd, &wset):%d\n", 
                        FD_ISSET(tcp_fd, &rset) , FD_ISSET(tcp_fd, &wset));        
         // test in linux environment,kernel version 3.5.0-23-generic 
         // tcp server do not send msg to client after tcp connecting
        int ret;
        int len;
        if(0 != getsockopt (tcp_fd, SOL_SOCKET, SO_ERROR, &ret, &len))
        {
            printf("getsocketopt failed\r\n");
            errcode = HILINK_SOCKET_TCP_CONNECT_FAILED;
        }
        printf("getsocketopt ret=%d\r\n",ret);
        if(0 != ret)
        {
            errcode = HILINK_SOCKET_TCP_CONNECT_FAILED;
        }
    }
             
    return errcode;
}

int hilink_tcp_read(int fd, unsigned char* buf, unsigned short len)
{
    int ret = -1;

    if(buf == NULL)
    {
        return HILINK_SOCKET_NULL_PTR;
    }

	/*TCP读取数据需要判断错误码*/
    ret = (int)(recv(fd, buf, len, MSG_DONTWAIT));
    if(ret <= 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
            return HILINK_SOCKET_NO_ERROR;
        }
        else
        {
            return HILINK_SOCKET_READ_TCP_PACKET_FAILED;
        }
    }

    return ret;
}




/* test code */
#ifdef TEST_UDP
int main()
{
    int udp_max_size = 512;
    unsigned char buffer[udp_max_size];
    int fd;
    unsigned short local_port = 5683;
    char* dst_ip = "10.177.56.197";
    unsigned short dst_port = 5683;
    int send_len = -1;
    int recv_len = -1;
    int recv_flag = 0;
    char remote_ip[16] = {0};
    unsigned short remote_port = -1;
    int input_len;
    fd = hilink_udp_new(local_port);
    if(fd < 0)
    {
        return -1;
    }

    while(1)
    {
        if(recv_flag == 0)
        {
            //向目标端发送数据
            memset(buffer, 0, udp_max_size);
            printf("input data you want send:\r\n");
            fgets((char*)buffer, udp_max_size, stdin);
            input_len = strlen((char*)buffer);
            buffer[input_len-1] = '\0';
            send_len = hilink_udp_send(fd, buffer, input_len-1,
                         dst_ip, dst_port);
            if(send_len < 0)
            {
                printf("send udp packet failed\r\n");
                break;
            }
            else if(send_len == 0)
            {
                memset(buffer, 0, udp_max_size);
                printf("send packet block,once again\r\n");
                continue;
            }
            recv_flag = 1;
            printf("I have sent to server %s\r\n", buffer);
            printf("Waiting respond from server\r\n");
        }

        memset(buffer, 0, udp_max_size);
        recv_len = hilink_udp_read(fd, buffer, udp_max_size,
                        remote_ip, 16, &remote_port);
        if(recv_len > 0)
        {
            recv_flag = 0;
        }
        else if(recv_len == 0)
        {
            continue;
        }
        else
        {
            printf("udp read failed\n");
            break;
        }
        printf("I have received from server\n");
    }

    hilink_udp_remove(fd);

    return 0;
}
//#else
int main()
{
    int tcp_max_size = 512;
    unsigned char buffer[tcp_max_size];
    int fd;
    char* dst_ip = "10.177.56.197";
    unsigned short dst_port = 5684;
    int send_len = -1;
    int recv_len = -1;
    int recv_flag = 0;
    int input_len;
    int tcp_connect_flag = 0;

    fd = hilink_tcp_connect(dst_ip, dst_port);
    if(fd < 0)
    {
        hilink_tcp_disconnect(fd);
        printf("tcp connect failed\n");
    }

    while(1)
    {
        if(tcp_connect_flag == 0)
        {
            if(hilink_tcp_state(fd)!= 0)
            {
                printf("tcp not connect\n");
                continue;
            }
            else
            {
                printf("tcp connect successuflly\n");
                tcp_connect_flag = 1;
            }
        }

        if(recv_flag == 0)
        {
            //向目标端发送数据
            memset(buffer, 0, tcp_max_size);
            printf("input data you want send:\r\n");
            fgets((char*)buffer, tcp_max_size, stdin);
            input_len = strlen((char*)buffer);
            buffer[input_len-1] = '\0';
            if(strlen((char*)buffer) == 0)
            {
                printf("you input no data\r\n");
                continue;
            }
            send_len = hilink_tcp_send(fd, buffer, input_len-1);
            if(send_len < 0)
            {
                printf("send tcp packet failed\r\n");
                break;
            }
            else if(send_len == 0)
            {
                memset(buffer, 0, tcp_max_size);
                printf("send packet block,once again\r\n");
                continue;
            }
            recv_flag = 1;
            printf("I have sent to server %s\r\n", buffer);
            printf("Waiting respond from server\r\n");
        }

        memset(buffer, 0, tcp_max_size);
        recv_len = hilink_tcp_read(fd, buffer, tcp_max_size);
        if(recv_len > 0)
        {
            recv_flag = 0;
        }
        else if(recv_len == 0)
        {
            printf("recv no data\n");
            continue;
        }
        else
        {
            printf("tcp read failed\n");
            break;
        }
        printf("recv data:%s,length:%d\n", buffer, recv_len);
        printf("I have received from server\n");
    }

    hilink_tcp_disconnect(fd);

    return 0;
}

#endif
