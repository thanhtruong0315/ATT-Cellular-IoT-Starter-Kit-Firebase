
#include "TrackingCellularInterface.h"
#include "mbed.h"
#include "hardware.h"

// TrackingCellularInterface implementation
TrackingCellularInterface::TrackingCellularInterface(CellularModuleInterface *cellularModule)
{
    _cellularModule = cellularModule;
}

nsapi_error_t TrackingCellularInterface::set_credentials(const char *apn, const char *username, const char *password)
{
    return 0;
}

nsapi_error_t TrackingCellularInterface::connect(const char *apn, const char *username, const char *password)
{
    PRINTF("Init cellular\r\n");
    char *IPAddress = NULL;
    _cellularModule->setAPN(apn, username, password);
    if (_cellularModule->connectInternet() == 0)
    {
        IPAddress = _cellularModule->getIPAddress();
        PRINTF("------IPAddress: %s\r\n", IPAddress);
        _ip_address.set_ip_address(IPAddress);
    }
    PRINTF("Init cellular: DONE %s\r\n", IPAddress);
    return 0;
}

int TrackingCellularInterface::disconnect()
{
    // if (!_wnc_control->disconnect()) {
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }

    return 0;
}

const char *TrackingCellularInterface::get_ip_address()
{
    return _ip_address.get_ip_address();
}

const char *TrackingCellularInterface::get_mac_address()
{
    return 0;
}

//struct tracking_cellular_socket
//{
//int socket;
//WNCControl::IpProtocol proto;
//WNCControl::IP ip;
//int port;
//};

int TrackingCellularInterface::socket_open(void **handle, nsapi_protocol_t proto)
{
    // WNCControl::IpProtocol mdmproto = (proto == NSAPI_UDP) ? WNCControl::IPPROTO_UDP : WNCControl::IPPROTO_TCP;
    // int fd = _wnc_control->socketSocket(mdmproto);
    // if (fd < 0) {
    //     return NSAPI_ERROR_NO_SOCKET;
    // }

    // _wnc_control->socketSetBlocking(fd, 10000);
    // struct tracking_cellular_socket *socket = new struct tracking_cellular_socket;
    // if (!socket) {
    //     return NSAPI_ERROR_NO_SOCKET;
    // }

    // socket->socket = fd;
    // socket->proto = mdmproto;
    // *handle = socket;
    return 0;
}

int TrackingCellularInterface::socket_close(void *handle)
{
    // struct tracking_cellular_socket *socket = (struct tracking_cellular_socket *)handle;
    // _wnc_control->socketFree(socket->socket);

    // delete socket;
    return 0;
}

int TrackingCellularInterface::socket_bind(void *handle, const SocketAddress &address)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int TrackingCellularInterface::socket_listen(void *handle, int backlog)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int TrackingCellularInterface::socket_connect(void *handle, const SocketAddress &addr)
{
    // struct tracking_cellular_socket *socket = (struct tracking_cellular_socket *)handle;

    // if (!_wnc_control->socketConnect(socket->socket, addr.get_ip_address(), addr.get_port())) {
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }

    return 0;
}

int TrackingCellularInterface::socket_accept(void **handle, void *server)
{
    return NSAPI_ERROR_UNSUPPORTED;
}

int TrackingCellularInterface::socket_send(void *handle, const void *data, unsigned size)
{
    // struct tracking_cellular_socket *socket = (struct tracking_cellular_socket *)handle;

    // int sent = _wnc_control->socketSend(socket->socket, (const char *)data, size);
    // if (sent == SOCKET_ERROR) {
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }

    // return sent;
    return 0;
}

int TrackingCellularInterface::socket_recv(void *handle, void *data, unsigned size)
{
    // struct tracking_cellular_socket *socket = (struct tracking_cellular_socket *)handle;
    // if (!_wnc_control->socketReadable(socket->socket)) {
    //     return NSAPI_ERROR_WOULD_BLOCK;
    // }

    // int recv = _wnc_control->socketRecv(socket->socket, (char *)data, size);
    // if (recv == SOCKET_ERROR) {
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }

    // return recv;
    return 0;
}

int TrackingCellularInterface::socket_sendto(void *handle, const SocketAddress &addr, const void *data, unsigned size)
{
    // struct tracking_cellular_socket *socket = (struct tracking_cellular_socket *)handle;

    // int sent = _wnc_control->socketSendTo(socket->socket,
    //         *(WNCControl::IP *)addr.get_ip_bytes(), addr.get_port(),
    //         (const char *)data, size);

    // if (sent == SOCKET_ERROR) {
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }

    // return sent;

    return 0;
}

int TrackingCellularInterface::socket_recvfrom(void *handle, SocketAddress *addr, void *data, unsigned size)
{
    // struct tracking_cellular_socket *socket = (struct tracking_cellular_socket *)handle;
    // if (!_wnc_control->socketReadable(socket->socket)) {
    //     return NSAPI_ERROR_WOULD_BLOCK;
    // }

    // WNCControl::IP ip;
    // int port;

    // int recv = _wnc_control->socketRecvFrom(socket->socket, &ip, &port, (char *)data, size);
    // if (recv == SOCKET_ERROR) {
    //     return NSAPI_ERROR_DEVICE_ERROR;
    // }

    // if (addr) {
    //     addr->set_ip_bytes(&ip, NSAPI_IPv4);
    //     addr->set_port(port);
    // }

    // return recv;
    return 0;
}

void TrackingCellularInterface::socket_attach(void *handle, void (*callback)(void *), void *data)
{
}
