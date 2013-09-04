/*
 * Dibbler - a portable DHCPv6
 *
 * author: Paul Schauer <p.schauer(at)comcast(dot)net>
 * changes: Tomasz Mrugalski <thomson@klub.com.pl>
 *
 * released under GNU GPL v2 licence
 *
 * Based on Port-linux/lowlevel-linux.c
 *
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>

#include <net/if_dl.h>
#include <net/if.h>
#include "Portable.h"

#include "dibbler-config.h"

#ifdef OPENBSD
#include "sys/uio.h"
#endif

// #define LOWLEVEL_DEBUG 0

char Message[1024] = {0};

int lowlevelInit() {
    return LOWLEVEL_NO_ERROR;
}

int lowlevelExit() {
    return LOWLEVEL_NO_ERROR;
}

void if_list_release(struct iface * list) {
    struct iface * tmp;
    while (list) {
        tmp = list->next;
        if (list->linkaddrcount)
            free(list->linkaddr);
        if (list->globaladdrcount)
            free(list->globaladdr);
        free(list);
        list = tmp;
    }
}

void if_print(struct iface * iface_ptr) {
    int tmp, tmpInt = 0;

    printf("Interface %s, index=%i type=%x flags=%x\n", iface_ptr->name, iface_ptr->id,
            iface_ptr->hardwareType, iface_ptr->flags);
    printf("\tLink layer Length: %x Addr:", iface_ptr->maclen);
    for (tmp = 0; tmp < iface_ptr->maclen; tmp++) {
        printf("%02x:", (unsigned char) iface_ptr->mac[tmp]);
    }
    printf("\n");
    printf("\tLocal IPv6 address count: %i, address ", iface_ptr->linkaddrcount);
    for (tmp = 0; tmp < iface_ptr->linkaddrcount; tmp++) {
        printf("\t%i=", tmp);
        for (tmpInt = 0; tmpInt < 16; tmpInt += 2) {
            printf("%02x%02x:",
                   (unsigned char) iface_ptr->linkaddr[tmpInt + tmp * 16],
                   (unsigned char) iface_ptr->linkaddr[tmpInt + 1 + tmp * 16]);
        }
        printf("\n");
    }
    if (iface_ptr->linkaddrcount==0)
        printf("\n");

    printf("\t%s Global IPv6 address count: %i, address ", iface_ptr->name,
           iface_ptr->globaladdrcount);
    tmpInt = 0;
    for (tmp = 0; tmp < iface_ptr->globaladdrcount; tmp++) {
        printf("\t%i=", tmp);
        for (tmpInt = 0; tmpInt < 16; tmpInt += 2) {
            printf("%02x%02x:",
                   (unsigned char) iface_ptr->globaladdr[tmpInt+ tmp * 16],
                   (unsigned char) iface_ptr->globaladdr[tmpInt + 1 + tmp * 16]);
        }
        printf("\n");
    }
   if (iface_ptr->globaladdrcount==0)
        printf("\n");
}

struct iface * if_list_add(struct iface * head, struct iface * element) {
    struct iface *tmp;
    element->next = NULL;
    if (!head)
        return element;
    tmp = head;
    while (tmp->next != NULL) {
        tmp = tmp->next;
    }

    tmp->next = element;
    return head;
}

/*
 * returns interface list with detailed informations
 */
struct iface * if_list_get() {

    /*
     * Translating between Mac OS X internal representation of link and IP address
     * and Dibbler internal format.
     */
    struct ifaddrs *addrs_lst = NULL; // list returned by system
    struct ifaddrs *addr_ptr = NULL; // single address
    struct iface *iface_lst = NULL;  // interface list
    struct iface *iface_ptr = NULL;  // pointer to single interface

    if (getifaddrs(&addrs_lst) != 0) {
        perror("Error in getifaddrs: ");
        return iface_lst;
    }

    /* First pass through entire addrs_lst: collect unique interface names and flags */
    addr_ptr = addrs_lst;
    while (addr_ptr != NULL) {
        // check if this interface name is already on target list
        iface_ptr = iface_lst;
        while (iface_ptr!=NULL) {
            if (!strcmp(addr_ptr->ifa_name, iface_ptr->name))
                break;
            iface_ptr = iface_ptr->next;
        }
        if (!iface_ptr) { // interface with that name not found, let's add one!
            iface_ptr = malloc(sizeof(struct iface));
            memset(iface_ptr, 0, sizeof(struct iface));

            strlcpy(iface_ptr->name, addr_ptr->ifa_name, MAX_IFNAME_LENGTH);
            iface_ptr->id = if_nametoindex(iface_ptr->name);
            iface_ptr->flags = addr_ptr->ifa_flags;
#ifdef LOWLEVEL_DEBUG
            printf("Detected interface %s, ifindex=%d, flags=%d\n",
                   iface_ptr->name, iface_ptr->id, iface_ptr->flags);
#endif

            // add this new structure to the end of the interfaces list
            iface_lst = if_list_add(iface_lst, iface_ptr);
        }

        addr_ptr = addr_ptr->ifa_next;
    }

    /*
     * Second pass through addrs_lst: collect link and IP layer info for each interface
     * by name
     */

    // for each address...
    for (addr_ptr = addrs_lst; addr_ptr != NULL; addr_ptr = addr_ptr->ifa_next) {
        for (iface_ptr = iface_lst; iface_ptr != NULL; iface_ptr = iface_ptr->next) {
            // ... find its corresponding interface
            if (strncmp(iface_ptr->name, addr_ptr->ifa_name, strlen(addr_ptr->ifa_name)))
                continue;

            switch (addr_ptr->ifa_addr->sa_family)
                {
                case AF_INET6:
                    {
                        char * ptr = (char*)(&((struct sockaddr_in6 *) addr_ptr->ifa_addr)->sin6_addr);
                        if (ptr[0] == 0xfe && ptr[1] == 0x80) { // link-local IPv6 address
                            char * addrs = malloc( (iface_ptr->linkaddrcount+1)*16);
                            memcpy(addrs, iface_ptr->linkaddr, 16*iface_ptr->linkaddrcount);
                            memcpy(addrs + 16*iface_ptr->linkaddrcount, ptr, 16);
                            free(iface_ptr->linkaddr);
                            iface_ptr->linkaddr = addrs;
                            iface_ptr->linkaddrcount++;
                        } else { // this is global address
                            char * addrs = malloc( (iface_ptr->globaladdrcount+1)*16);
                            memcpy(addrs, iface_ptr->globaladdr, 16*iface_ptr->globaladdrcount);
                            memcpy(addrs + 16*iface_ptr->globaladdrcount, ptr, 16);
                            free(iface_ptr->globaladdr);
                            iface_ptr->globaladdr = addrs;
                            iface_ptr->globaladdrcount++;
                        }
                        break;
                    } // end of AF_INET6 handling
                case AF_LINK:
                    {
                        struct sockaddr_dl *linkInfo;
                        linkInfo = (struct sockaddr_dl *) addr_ptr->ifa_addr;

                        // Note: sdl_type is unsigned character; hardwareType is integer
                        iface_ptr->hardwareType = linkInfo->sdl_type;
                        if (linkInfo->sdl_alen > 1) {
                            memcpy(iface_ptr->mac, LLADDR(linkInfo),
                                   linkInfo->sdl_alen);
                            iface_ptr->maclen = linkInfo->sdl_alen;
                        }
                        break;
                    }
                default:
                    break; // ignore other address families
                }
        }
    }

    /* Print out iface_lst data if debug mode */
#ifdef LOWLEVEL_DEBUG
    iface_ptr = iface_lst;
    while (iface_ptr) {
        if_print(iface_ptr);
        iface_ptr = iface_ptr->next;
    }
#endif

    return iface_lst;
} /* end of if_list_get */

int ipaddr_add(const char * ifacename, int ifaceid, const char * addr,
        unsigned long pref, unsigned long valid, int prefixLength) {
    char buf[512];
    int status;
    sprintf(buf, "ifconfig %s inet6 %s prefixlen %d add", ifacename, addr, prefixLength);
    status = system(buf);
    if (!status)
        return LOWLEVEL_NO_ERROR;
    return LOWLEVEL_ERROR_UNSPEC;
}

int ipaddr_update(const char* ifacename, int ifindex, const char* addr,
        unsigned long pref, unsigned long valid, int prefixLength) {

    /// @todo: implement this
    sprintf(Message, "Address update on BSD systems not implemented yet.");
    return LOWLEVEL_ERROR_NOT_IMPLEMENTED;
}

int ipaddr_del(const char * ifacename, int ifaceid, const char * addr,
        int prefixLength) {
    char buf[512];
    int status;
    sprintf(buf, "ifconfig %s inet6 %s prefixlen %d delete", ifacename, addr, prefixLength);
    status = system(buf);
    if (!status)
        return LOWLEVEL_NO_ERROR;
    return LOWLEVEL_ERROR_UNSPEC;
}

int sock_add(char * ifacename, int ifaceid, char * addr, int port,
        int thisifaceonly, int reuse) {
    int error;
    int on = 1;
    struct addrinfo hints;
    struct addrinfo *res;
    int Insock;
    int multicast;
    char port_char[6];
    char * tmp;
    struct sockaddr_in6 bindme;
    sprintf(port_char, "%d", port);

    if (!strncasecmp(addr, "ff", 2))
        multicast = 1;
    else
        multicast = 0;

#ifdef LOWLEVEL_DEBUG
    printf(
            "LOWLEVEL_DEBUG iface: %s(id=%d), addr=%s, port=%d, ifaceonly=%d reuse=%d###\n",
            ifacename, ifaceid, addr, port, thisifaceonly, reuse);
    fflush(stdout);
#endif

    /* Open a socket for inbound traffic */
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;
    if ((error = getaddrinfo(NULL, port_char, &hints, &res))) {
        sprintf(Message,
                "getaddrinfo failed. Is IPv6 protocol supported by kernel?");
        return LOWLEVEL_ERROR_GETADDRINFO;
    }
    if ((Insock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        sprintf(Message,
                "socket creation failed. Is IPv6 protocol supported by kernel?");
        return LOWLEVEL_ERROR_UNSPEC;
    }

    /* Mac OS X have IPV6_PKTINFO only */
    /* OpenBSD, NetBSD require IPV6_RECVPKTINFO */
#if (HAVE_DECL_IPV6_RECVPKTINFO == 0) && (HAVE_DECL_IPV6_PKTINFO == 0)
#error "Both IPV6_RECVPKTINFO and IPV6_PKTINFO not defined. Need at least one of them"
#endif

    /* Set the options to receive info about ipv6 traffic */
#if HAVE_DECL_IPV6_RECVPKTINFO == 1
    if (setsockopt(Insock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof (on)) < 0)
#endif
#if HAVE_DECL_IPV6_PKTINFO == 1
    if (setsockopt(Insock, IPPROTO_IPV6, IPV6_PKTINFO, &on, sizeof(on)) < 0)
#endif
    {
        sprintf(Message, "Failed to set up socket option (tried both IPV6_RECVPKTINFO and IPV6_PKTINFO).");
        return LOWLEVEL_ERROR_SOCK_OPTS;
    }

    if (thisifaceonly) {
#if 0
        /* this part also looks like linux only code */
        if (setsockopt(Insock, SOL_SOCKET, SO_BINDTODEVICE, ifacename, strlen(ifacename) + 1) < 0) {
            sprintf(Message, "Unable to bind socket to interface %s.", ifacename);
            return LOWLEVEL_ERROR_BIND_IFACE;
        }
#endif
    }

    /* allow address reuse (this option sucks - why allow running multiple servers?) */
    if (reuse != 0) {
        if (setsockopt(Insock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
            sprintf(Message, "Unable to set up socket option SO_REUSEADDR.");
            return LOWLEVEL_ERROR_REUSE_FAILED;
        }
    }

    /* bind socket to a specified port */
    bzero(&bindme, sizeof (struct sockaddr_in6));
    bindme.sin6_family = AF_INET6;
    bindme.sin6_port = htons(port);
    /* Bind to interface using scope_id */
    bindme.sin6_scope_id = ifaceid;
    tmp = (char*) (&bindme.sin6_addr);
    inet_pton6(addr, tmp);
    if (bind(Insock, (struct sockaddr*) & bindme, sizeof (bindme)) < 0) {
        sprintf(Message, "Unable to bind socket: %s", strerror(errno));
        return LOWLEVEL_ERROR_BIND_FAILED;
    }

    freeaddrinfo(res);

    /* multicast server stuff */
    if (multicast) {
        struct ipv6_mreq mreq6;
        memset(&mreq6, 0, sizeof (mreq6));
        mreq6.ipv6mr_interface = ifaceid;
        tmp = (char*) (&mreq6.ipv6mr_multiaddr);
        inet_pton6(addr, tmp);

        /* Add to the all agent multicast address */
        if (setsockopt(Insock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof (mreq6))) {
            sprintf(Message, "error joining ipv6 group");
            return LOWLEVEL_ERROR_MCAST_MEMBERSHIP;
        }
    }

    return Insock;
}

int sock_del(int fd) {
    return close(fd);
}

int sock_send(int sock, char *addr, char *buf, int message_len, int port, int iface) {
    int result;
    struct sockaddr_in6 dst;

    memset(&dst, 0, sizeof (struct sockaddr_in6));
    dst.sin6_len = sizeof(struct sockaddr_in6);
    dst.sin6_family = PF_INET6;
    dst.sin6_port = htons(port); // htons?
    inet_pton6(addr,(char*)&dst.sin6_addr);
    dst.sin6_scope_id = iface;

    result = sendto(sock, buf, message_len, 0, (struct sockaddr*)&dst, sizeof(struct sockaddr_in6));

    if (result < 0) {
        sprintf(Message, "Unable to send data (dst addr: %s), error=%d", addr, result);
        return LOWLEVEL_ERROR_SOCKET;
    }
    return LOWLEVEL_NO_ERROR;
}

/*
 *
 */
int sock_recv(int fd, char * myPlainAddr, char * peerPlainAddr, char * buf,
        int buflen) {
    struct msghdr msg; /* message received by recvmsg */
    struct sockaddr_in6 peerAddr; /* sender address */
    struct sockaddr_in6 myAddr; /* my address */
    struct iovec iov; /* simple structure containing buffer address and length */

    struct cmsghdr *cm; /* control message */
    struct in6_pktinfo *pktinfo;

    char control[CMSG_SPACE(sizeof (struct in6_pktinfo))];
    char controlLen = CMSG_SPACE(sizeof (struct in6_pktinfo));
    int result = 0;
    bzero(&msg, sizeof (msg));
    bzero(&peerAddr, sizeof (peerAddr));
    bzero(&myAddr, sizeof (myAddr));
    bzero(&control, sizeof (control));
    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = &peerAddr;
    msg.msg_namelen = sizeof (peerAddr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = controlLen;

    result = recvmsg(fd, &msg, 0);

    if (result == -1) {
        return LOWLEVEL_ERROR_UNSPEC;
    }

    /* get source address */
    inet_ntop6((void*) & peerAddr.sin6_addr, peerPlainAddr);

    /* get destination address */
    for (cm = (struct cmsghdr *) CMSG_FIRSTHDR(&msg); cm; cm = (struct cmsghdr *) CMSG_NXTHDR(&msg, cm)) {
        if (cm->cmsg_level != IPPROTO_IPV6 || cm->cmsg_type != IPV6_PKTINFO)
            continue;
        pktinfo = (struct in6_pktinfo *) (CMSG_DATA(cm));
        inet_ntop6((void*) & pktinfo->ipi6_addr, myPlainAddr);
    }
    return result;
}

void microsleep(int microsecs) {
    struct timespec x, y;

    x.tv_sec = (int) microsecs / 1000000;
    x.tv_nsec = (microsecs - x.tv_sec * 1000000) * 1000;
    nanosleep(&x, &y);
}

/*
 * returns: -1 - address not found, 0 - addr is ok, 1 - addr is tentative
 */
int is_addr_tentative(char * ifacename, int iface, char * addr) {
    /// @todo: implement this
    return 0;
}

uint32_t getAAASPIfromFile() {
    char filename[1024];
    struct stat st;
    uint32_t ret;
    FILE *file;

    strcpy(filename, "/var/lib/dibbler/AAA/AAA-SPI");

    if (stat(filename, &st))
        return 0;

    file = fopen(filename, "r");
    if (!file)
        return 0;

    fscanf(file, "%9x", &ret);
    fclose(file);

    return ret;
}

char * getAAAKeyFilename(uint32_t SPI) {
    static char filename[1024];
    if (SPI != 0)
        snprintf(filename, 1024, "%s%s%x", "/var/lib/dibbler/AAA/", "AAA-key-",
            SPI);
    else
        strcpy(filename, "/var/lib/dibbler/AAA/AAA-key");
    return filename;
}

char * getAAAKey(uint32_t SPI, unsigned *len) {
    char * filename = 0;
    struct stat st;
    char * retval;
    int offset = 0;
    int fd;
    int ret;

    filename = getAAAKeyFilename(SPI);

    if (stat(filename, &st))
        return NULL;

    fd = open(filename, O_RDONLY);
    if (0 > fd)
        return NULL;

    retval = malloc(st.st_size);
    if (!retval) {
        close(fd);
        return NULL;
    }

    while (offset < st.st_size) {
        ret = read(fd, retval + offset, st.st_size - offset);
        if (!ret)
            break;
        if (ret < 0) {
            free(retval);
            return NULL;
        }
        offset += ret;
    }
    close(fd);

    if (offset != st.st_size) {
        free(retval);
        return NULL;
    }

    *len = st.st_size;
    return retval;
}

char * error_message() {
    return Message;
}


/**
 * begin link monitoring
 *
 * @param monitored_links head of the monitored links list
 * @param notify pointer to variable that is going to be modifed if change is detected
 */
void link_state_change_init(volatile struct link_state_notify_t * monitored_links, volatile int * notify)
{
    printf("Link change monitoring is not supported yet on MacOS or BSD. Sorry.\n");
    return;
}

/**
 * cleanup code for link state monitoring
 *
 */
void link_state_change_cleanup()
{
    return;
}


extern int sock_add_tcp (char * ifacename,int ifaceid, char * addr, int port) {


    int on = 1,result=0;

    int error;

    struct addrinfo *res, *rp;
    struct addrinfo hints;
    fd_set master_set;
    int Insock;
    char port_char[6];
    char * tmp;
    int connectionNumber =1;
    ifaceid = 6;

//#ifdef LOWLEVEL_DEBUG
    printf("\n ### iface: %s(id=%d), addr=%s, port=%d \n", ifacename,ifaceid, addr, port);
    fflush(stdout);
//#endif

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    if( (error = getaddrinfo(NULL,  port_char, &hints, &res)) ) {
        sprintf(Message, "getaddrinfo failed. Is IPv6 protocol supported by kernel?");
        return LOWLEVEL_ERROR_GETADDRINFO;
    }

    if (port > 0) {

        sprintf(port_char,"%d",port);
        if( (Insock = socket(AF_INET6, SOCK_STREAM,0 )) < 0) {
            sprintf(Message, "socket creation failed. Is IPv6 protocol supported by kernel?");
            return LOWLEVEL_ERROR_UNSPEC;
        } else {
            printf("\n TCP socket has been created correctly\n " );
        }

        if (setsockopt(Insock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on) ) < 0) {
            sprintf(Message, "Unable to set up socket option SO_REUSEADDR");
            return LOWLEVEL_ERROR_SOCK_OPTS;
        }

        /* Set the options  to receivce ipv6 traffic */
        if (setsockopt(Insock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) < 0) {
          sprintf(Message, "Unable to set up socket option IPV6_RECVPKTINFO.");
          return LOWLEVEL_ERROR_SOCK_OPTS;
        }

        // set socket as nonblocking
        if (ioctl(Insock, FIONBIO, (char *)&on) < 0) {
           sprintf(Message, "Unable to set up socket as nonblocking - ioctl's failure.");
           return LOWLEVEL_ERROR_SOCK_OPTS;
        }

        freeaddrinfo(res);


        //initialize master fd_set
        FD_ZERO(&master_set);
        FD_SET(Insock,&master_set);
    }
    /* Open a TCP socket for inbound traffic */

   // struct sockaddr_in6 bindmeClient;

    //TCP client part
    if (port == 0) {

        port=547;
        sprintf(port_char,"%d",port);

        /* bind socket to a specified port */
        /*bzero(&bindmeClient, sizeof(struct sockaddr_in6));
        bindmeClient.sin6_family = AF_INET6;
        bindmeClient.sin6_port   = htons(port);
        bindmeClient.sin6_family = IPPROTO_TCP;
        tmp = (char*)(&bindmeClient.sin6_addr);
        inet_pton6(addr, tmp);*/
        //bindmeClient.sin6_addr = addr;

        memset(&hints,0,sizeof(hints));
        memset(&res,0,sizeof(res));

        hints.ai_flags    = AI_NUMERICSERV;
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

       /* if ((error = getaddrinfo(hostp->hostname, hostp->port,
                    &hints, &res)) != 0) */

        printf("\n ### iface: %s(id=%d), addr=%s, port=%d \n", ifacename,ifaceid, addr, port);

        if( (error = getaddrinfo(addr, port_char, &hints, &res)) ) {
            sprintf(Message, "getaddrinfo failed. Is IPv6 protocol supported by kernel?");
            return LOWLEVEL_ERROR_GETADDRINFO;
        } else {
            printf ("\n getaddrinfo called correctly\n");
            //printf("\n addr: %s\n", res->ai_addr);
        }
        //(connect(Insock, (struct sockaddr_in6*)&bindmeClient, sizeof(struct sockaddr_in6)) != 0)

        for(rp=res; rp!=NULL;rp->ai_next) {
            Insock = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
            if(Insock==-1) {
                continue;
            } else {
                printf("\n TCP socket has been created correctly\n " );
            }
            if (connect(Insock, rp->ai_addr, rp->ai_addrlen) != 0) {
                 Rerror("Unable to connect with DHCP server, connect function failed");
            } else {
                 result++;
                 break;
            }
        }

        if (result==0) {
            printf("\n Can't connect on any interface with specified host");
            return LOWLEVEL_ERROR_CONNECT_FAILED;
        } else {
            printf("\n Connected with host");
            return Insock;
        }
    }
    result=0;
    //TCP server part
    struct sockaddr_in6 bindmeServer;
    if ( port > 0) {

        /* bind socket to a specified port */
        bzero(&bindmeServer, sizeof(struct sockaddr_in6));
        bindmeServer.sin6_family = AF_INET6;
        bindmeServer.sin6_port   = htons(port);
        tmp = (char*)(&bindmeServer.sin6_addr);
        inet_pton6(addr, tmp);

        socklen_t bindmeServerSize;
        bindmeServerSize= sizeof (struct sockaddr_in6);

        printf("\n Socket FD: %d", Insock );
       // printf("\n Size of struct passed to bind:%d\n",sizeof(struct sockaddr_in6));
        //printf("\n Size of struct passed to bind:%d",sizeof(struct sockaddr_in6));
        result = bind( Insock,(struct sockaddr_in6 *)&bindmeServer,bindmeServerSize);
        printf("\n Result:%d \n",result);

        if (result < 0 ) {
            sprintf(Message, "Unable to bind socket: %s", strerror(errno) );
            return LOWLEVEL_ERROR_BIND_FAILED;
        } else {
            sprintf(Message, "Socket has been bind succesfully");
            printf("\n Socket has been bind succesfully \n");
        }

        if (connectionNumber > 0)  {
             if ( listen (Insock,connectionNumber) != 0 ) {
                 printf("\n Listen function has NOT been called correctly \n");
                 return LOWLEVEL_ERROR_LISTEN_FAILED;
             } else {
                 sprintf(Message,"Listen function has been called correctly");
                 printf("\n Listen function has been called correctly \n");
             }
        } else {
            sprintf(Message, "Connection number hasn't been specified");
            return LOWLEVEL_ERROR_LISTEN_FAILED;
        }

    }

    printf("\nRETURN SOCK FD:%d\n",Insock);
    return Insock;

}

//!getsOpt(sock->getFD(), SOL_SOCKET, SO_TYPE, (char*)&stype)
//int getsockopt(int socket, int level, int option_name,void *restrict option_value, socklen_t *restrict option_len);
extern int getsOpt(int fd) {

    int len, sockType, result;

    len = sizeof(sockType);
    result = getsockopt(fd,SOL_SOCKET,SO_TYPE,&sockType,&len);
    if (result < 0) {
        Rerror("Getsockopt function failed");
        return result;
    } else {
        sprintf(Message, "Getsockopt OK");
        return sockType;
    }
    return 0;
}

extern int accept_tcp (int fd) {

    int fd_new;
    /*
    if(!master_set) {
        sprintf(Message, "Master set of file descriptor not defined");
        return 1;
    }*/

    /*addrLength = sizeof(socketStruct.sockaddr_in);
    if (( addr!=NULL) && (addrLength!=0 ) ) {
        fd_new = accept(fd,(struct sockaddr*) &addr, &addrLength);
        if (fd_new == -1) {
            sprintf(Message, "Accept function failed. Cannot create net socket descriptor");
            close(fd_new);
            return 1;
        } else {
            return fd_new;
        }

    } */

    fd_new = accept(fd,NULL,NULL);
    if (fd_new < 0) {
        //sprintf(Message, "Accept function failed. Cannot create net socket descriptor");
        //close(fd_new);

        if (errno !=  EWOULDBLOCK) {
            sprintf(Message, "Accept function failed. Cannot create net socket descriptor" );

            //Here should be check if client didn't close connection
            //close(fd_new);
            return -1;
        }

    }
    return fd_new;
}

//extern int getPeerName_ipv6(int fd,struct socketStruct,char * addr) {

//    int addrLength;
//    int fd_new;
//    addrLength = sizeof(socketStruct.sockaddr_in);
//     if(getpeername(fd, addr, addrLength) < 0) {
//         sprintf(Message, "Getpeername function failed. Cannot return peername address");
//         close(fd_new);
//         return 1;
//     } else {
//         return 0;
//     }
//}

extern int sock_recv_tcp(int fd, char * recvBuffer, int bufLength, int flags) {

    int iResult;
    iResult = recv (fd, recvBuffer, bufLength, flags);
    if (iResult < 0) {
        sprintf (Message,"Cannot receive data, receive function socket error");
        Rerror("Receive function error:");
        return LOWLEVEL_ERROR_UNSPEC;
    } else {
        return iResult;
    }
}


extern int sock_send_tcp(int fd,char * addr, char *buf, int buflen, int flags, int port) {

    struct addrinfo hints, *res;
    int iResult = 0;
    char cport[10];

    sprintf(cport,"%d",port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET6;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(addr, cport, &hints, &res) < 0) {
        return -1; /* Error in transmitting */
    }

    if (!buflen){
        buflen=(int)strlen(buf);
    }
    iResult = send (fd,buf,buflen,flags);

    if (iResult < 0)
    {
        sprintf(Message, "Unable to send data (dst addr: %s)", addr);
        Rerror("Tcp send function error");
        return LOWLEVEL_ERROR_SOCKET;
    } else {
        printf("\n %d bytes has been send\n",iResult);
    }

    freeaddrinfo(res);

    return iResult;
}

extern int terminate_tcp_connection(int fd,int how) {

    /*SD_RECEIVE 0
    SD_SEND    1
    SD_BOTH    2*/
    int iResult;

    iResult = shutdown(fd,how);
    if (iResult < 0) {
        close(fd);
        sprintf(Message, "Shutdown failed. Close function called\n");
        return LOWLEVEL_ERROR_SOCKET;
    }
    return iResult;

}
