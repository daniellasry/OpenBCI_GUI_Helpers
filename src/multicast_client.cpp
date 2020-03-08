#include <iostream>
#include <string.h>

#include "multicast_client.h"

///////////////////////////////
/////////// WINDOWS ///////////
//////////////////////////////
#ifdef _WIN32

#include <errno.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")


MultiCastClient::MultiCastClient (const char *ip_addr, int port)
{
    strcpy (this->ip_addr, ip_addr);
    this->port = port;
    client_socket = INVALID_SOCKET;
    memset (&socket_addr, 0, sizeof (socket_addr));
}

int MultiCastClient::init ()
{
    WSADATA wsadata;
    int res = WSAStartup (MAKEWORD (2, 2), &wsadata);
    if (res != 0)
    {
        return (int)MultiCastReturnCodes::WSA_STARTUP_ERROR;
    }
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons (port);
    socket_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    client_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket == INVALID_SOCKET)
    {
        return (int)MultiCastReturnCodes::CREATE_SOCKET_ERROR;
    }

    // ensure that library will not hang in blocking recv/send call
    DWORD timeout = 5000;
    DWORD value = 1;
    setsockopt (client_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof (value));
    setsockopt (client_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof (timeout));
    setsockopt (client_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof (timeout));

    if (bind (client_socket, (const struct sockaddr *)&socket_addr, sizeof (socket_addr)) != 0)
    {
        return (int)MultiCastReturnCodes::BIND_ERROR;
    }

    struct ip_mreq mreq;
    if (inet_pton (AF_INET, ip_addr, &mreq.imr_multiaddr.s_addr) == 0)
    {
        return (int)MultiCastReturnCodes::PTON_ERROR;
    }
    mreq.imr_interface.s_addr = htonl (INADDR_ANY);
    if (setsockopt (client_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof (mreq)) !=
        0)
    {
        return (int)MultiCastReturnCodes::SET_OPT_ERROR;
    }

    return (int)MultiCastReturnCodes::STATUS_OK;
}

int MultiCastClient::recv (void *data, int size)
{
    int len = sizeof (socket_addr);
    int res = recvfrom (client_socket, (char *)data, size, 0, (sockaddr *)&socket_addr, &len);
    if (res == SOCKET_ERROR)
    {
        std::cerr << "WSAGetLastError is " << WSAGetLastError () << std::endl;
        return -1;
    }
    return res;
}

int MultiCastClient::send (void *data, int size)
{
    int len = (unsigned int)sizeof (socket_addr);
    int res = sendto (client_socket, (char *)data, size, 0, (sockaddr *)&socket_addr, len);
    if (res == SOCKET_ERROR)
    {
        std::cerr << "WSAGetLastError is " << WSAGetLastError () << std::endl;
        return -1;
    }
    return res;
}

void MultiCastClient::close ()
{
    closesocket (client_socket);
    client_socket = INVALID_SOCKET;
    WSACleanup ();
}

///////////////////////////////
//////////// UNIX /////////////
///////////////////////////////
#else

#include <netinet/in.h>
#include <netinet/tcp.h>


MultiCastClient::MultiCastClient (const char *ip_addr, int port)
{
    strcpy (this->ip_addr, ip_addr);
    this->port = port;
    client_socket = -1;
    memset (&socket_addr, 0, sizeof (socket_addr));
}

int MultiCastClient::init ()
{
    client_socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client_socket < 0)
    {
        return (int)MultiCastReturnCodes::CREATE_SOCKET_ERROR;
    }

    socket_addr.sin_family = AF_INET;
    socket_addr.sin_port = htons (port);
    socket_addr.sin_addr.s_addr = htonl (INADDR_ANY);

    // ensure that library will not hang in blocking recv/send call
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    int value = 1;
    setsockopt (client_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof (value));
    setsockopt (client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof (tv));
    setsockopt (client_socket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof (tv));

    if (bind (client_socket, (const struct sockaddr *)&socket_addr, sizeof (socket_addr)) != 0)
    {
        return (int)MultiCastReturnCodes::BIND_ERROR;
    }

    struct ip_mreq mreq;
    if (inet_pton (AF_INET, ip_addr, &mreq.imr_multiaddr.s_addr) == 0)
    {
        return (int)MultiCastReturnCodes::PTON_ERROR;
    }
    mreq.imr_interface.s_addr = htonl (INADDR_ANY);
    if (setsockopt (client_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof (mreq)) !=
        0)
    {
        return (int)MultiCastReturnCodes::SET_OPT_ERROR;
    }

    return (int)MultiCastReturnCodes::STATUS_OK;
}

int MultiCastClient::recv (void *data, int size)
{
    unsigned int len = (unsigned int)sizeof (socket_addr);
    int res = recvfrom (client_socket, (char *)data, size, 0, (sockaddr *)&socket_addr, &len);
    return res;
}

int MultiCastClient::send (void *data, int size)
{
    unsigned int len = (unsigned int)sizeof (socket_addr);
    int res = sendto (client_socket, (char *)data, size, 0, (sockaddr *)&socket_addr, len);
    return res;
}

void MultiCastClient::close ()
{
    ::close (client_socket);
    client_socket = -1;
}

#endif
