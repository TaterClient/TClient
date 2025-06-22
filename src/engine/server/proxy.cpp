#include "proxy.h"

#include <engine/shared/packer.h>
#include <engine/shared/unpacker.h>
#include <engine/shared/protocol.h>
#include <engine/shared/network.h>
#include <base/system.h>

#include <cstdio>
#include <cstring>

bool CProxy::Init(const NETADDR &listenAddr, const NETADDR &serverAddr,
                  const char *pSpoofVer, int spoofVerInt)
{
    m_ServerAddr = serverAddr;
    str_copy(m_aSpoofVersion, pSpoofVer, sizeof(m_aSpoofVersion));
    m_SpoofDDNet = spoofVerInt;

    if(net_udp_create(&m_SockClient, &listenAddr, 0) != 0)
    {
        dbg_msg("proxy", "failed to bind client socket");
        return false;
    }
    // create server socket on any port
    NETADDR bindSrv = listenAddr;
    bindSrv.port = 0; // let OS choose
    if(net_udp_create(&m_SockServer, &bindSrv, 0) != 0)
    {
        dbg_msg("proxy", "failed to create server socket");
        return false;
    }
    dbg_msg("proxy", "listening on %d, forwarding to %s:%d", listenAddr.port, m_ServerAddr.ip ? "ip" : "", m_ServerAddr.port);
    return true;
}

void CProxy::Pump()
{
    HandleFromClient();
    HandleFromServer();
}

static bool RecvChunk(CNetUDP *pSock, CNetChunk &Chunk)
{
    unsigned char aBuf[NET_MAX_PAYLOAD];
    NETADDR From;
    int size = net_udp_recv(pSock, &From, aBuf, sizeof(aBuf));
    if(size <= 0)
        return false;
    Chunk.m_ClientId = -1;
    Chunk.m_Address = From;
    Chunk.m_pData = aBuf;
    Chunk.m_DataSize = size;
    Chunk.m_Flags = 0;
    return true;
}

void CProxy::HandleFromClient()
{
    CNetChunk Chunk;
    while(true)
    {
        unsigned char aBuf[NET_MAX_PAYLOAD];
        NETADDR From;
        int size = net_udp_recv(&m_SockClient, &From, aBuf, sizeof(aBuf));
        if(size <= 0)
            break;

        m_ClientAddr = From; // remember
        Chunk.m_ClientId = -1;
        Chunk.m_Address = From;
        Chunk.m_pData = aBuf;
        Chunk.m_DataSize = size;
        Chunk.m_Flags = 0;

        RewriteHandshake(Chunk);

        net_udp_send(&m_SockServer, &m_ServerAddr, Chunk.m_pData, Chunk.m_DataSize);
    }
}

void CProxy::HandleFromServer()
{
    unsigned char aBuf[NET_MAX_PAYLOAD];
    NETADDR From;
    while(true)
    {
        int size = net_udp_recv(&m_SockServer, &From, aBuf, sizeof(aBuf));
        if(size <= 0)
            break;
        if(m_ClientAddr.port != 0)
            net_udp_send(&m_SockClient, &m_ClientAddr, aBuf, size);
    }
}

bool CProxy::RewriteHandshake(CNetChunk &Chunk)
{
    CUnpacker Unpacker;
    Unpacker.Reset(Chunk.m_pData, Chunk.m_DataSize);
    int MsgID = Unpacker.GetInt();

    if(MsgID != NETMSG_CLIENTVER && MsgID != NETMSG_INFO)
        return false;

    if(MsgID == NETMSG_CLIENTVER)
    {
        CUuid Uuid;
        mem_copy(&Uuid, Unpacker.GetRaw(sizeof(Uuid)), sizeof(Uuid));
        // ignore old int & string
        Unpacker.GetInt();
        Unpacker.GetString(CUnpacker::SANITIZE_CC);

        CMsgPacker Packer(NETMSG_CLIENTVER, true);
        Packer.AddRaw(&Uuid, sizeof(Uuid));
        Packer.AddInt(m_SpoofDDNet);
        Packer.AddString(m_aSpoofVersion, 0);
        if(Packer.Error())
            return false;
        mem_copy(Chunk.m_pData, Packer.Data(), Packer.Size());
        Chunk.m_DataSize = Packer.Size();
        return true;
    }
    else if(MsgID == NETMSG_INFO)
    {
        // skip old version
        Unpacker.GetString(CUnpacker::SANITIZE_CC);
        const char *pPassword = Unpacker.GetString(CUnpacker::SANITIZE_CC);

        CMsgPacker Packer(NETMSG_INFO, true);
        Packer.AddString(m_aSpoofVersion, 0); // send spoofed net version
        Packer.AddString(pPassword, 0);
        if(Packer.Error())
            return false;
        mem_copy(Chunk.m_pData, Packer.Data(), Packer.Size());
        Chunk.m_DataSize = Packer.Size();
        return true;
    }
    return false;
}