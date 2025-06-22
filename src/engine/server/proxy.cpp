#include "proxy.h"

#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/network.h>
#include <engine/shared/uuid_manager.h>
#include <base/system.h>

#include <cstdio>
#include <cstring>

CProxy::CProxy()
{
	m_SockClient = NETSOCKET_INVALID;
	m_SockServer = NETSOCKET_INVALID;
	mem_zero(&m_ServerAddr, sizeof(m_ServerAddr));
	mem_zero(&m_ClientAddr, sizeof(m_ClientAddr));
	mem_zero(m_aSpoofVersion, sizeof(m_aSpoofVersion));
	m_SpoofDDNet = -1;
}

bool CProxy::Init(const NETADDR &listenAddr, const NETADDR &serverAddr,
                  const char *pSpoofVer, int spoofVerInt)
{
	m_ServerAddr = serverAddr;
	str_copy(m_aSpoofVersion, pSpoofVer, sizeof(m_aSpoofVersion));
	m_SpoofDDNet = spoofVerInt;

	m_SockClient = net_udp_create(listenAddr);
	if(m_SockClient == NETSOCKET_INVALID)
	{
		dbg_msg("proxy", "failed to bind client socket");
		return false;
	}
	
	// create server socket on any port
	NETADDR bindSrv = listenAddr;
	bindSrv.port = 0; // let OS choose
	m_SockServer = net_udp_create(bindSrv);
	if(m_SockServer == NETSOCKET_INVALID)
	{
		dbg_msg("proxy", "failed to create server socket");
		return false;
	}
	
	char aServerStr[128];
	net_addr_str(&m_ServerAddr, aServerStr, sizeof(aServerStr), true);
	dbg_msg("proxy", "listening on port %d, forwarding to %s", listenAddr.port, aServerStr);
	return true;
}

void CProxy::Pump()
{
	HandleFromClient();
	HandleFromServer();
}

void CProxy::HandleFromClient()
{
	unsigned char *pData;
	NETADDR From;
	
	while(true)
	{
		int size = net_udp_recv(m_SockClient, &From, &pData);
		if(size <= 0)
			break;

		m_ClientAddr = From; // remember client

		// Copy data to modify it
		unsigned char aBuffer[NET_MAX_PAYLOAD];
		if(size > (int)sizeof(aBuffer))
			size = sizeof(aBuffer);
		mem_copy(aBuffer, pData, size);

		RewriteHandshake(aBuffer, &size, sizeof(aBuffer));

		net_udp_send(m_SockServer, &m_ServerAddr, aBuffer, size);
	}
}

void CProxy::HandleFromServer()
{
	unsigned char *pData;
	NETADDR From;
	
	while(true)
	{
		int size = net_udp_recv(m_SockServer, &From, &pData);
		if(size <= 0)
			break;
		if(m_ClientAddr.port != 0)
			net_udp_send(m_SockClient, &m_ClientAddr, pData, size);
	}
}

bool CProxy::RewriteHandshake(unsigned char *pData, int *pSize, int maxSize)
{
	CUnpacker Unpacker;
	Unpacker.Reset(pData, *pSize);
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
		if(Packer.Error() || Packer.Size() > maxSize)
			return false;
		mem_copy(pData, Packer.Data(), Packer.Size());
		*pSize = Packer.Size();
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
		if(Packer.Error() || Packer.Size() > maxSize)
			return false;
		mem_copy(pData, Packer.Data(), Packer.Size());
		*pSize = Packer.Size();
		return true;
	}
	return false;
}