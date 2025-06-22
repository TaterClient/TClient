#pragma once

#include <engine/shared/protocol.h>
#include <engine/shared/network.h>
#include <base/system.h>

// Simple UDP proxy that rewrites DDNet handshake packets (CLIENTVER, INFO)
// to spoof client version/string before forwarding them to the real server.
// Usage: Construct, call Init(listen, upstream, spoofVerStr, spoofVerInt) and
// then Pump() each frame to forward queued traffic.
class CProxy
{
public:
	CProxy();

	// Initialize sockets; returns false on error.
	//  listenAddr  – address to bind for incoming client packets (usually 0.0.0.0:8303)
	//  serverAddr  – address of the real upstream server
	//  pSpoofVer   – replacement version string for both CLIENTVER and INFO
	//  spoofVerInt – replacement integer version for CLIENTVER
	bool Init(const NETADDR &listenAddr, const NETADDR &serverAddr,
	          const char *pSpoofVer, int spoofVerInt);

	// Process all pending packets (non-blocking). Should be called frequently.
	void Pump();

private:
	NETSOCKET m_SockClient; // socket bound to listenAddr (receives from client, sends to server)
	NETSOCKET m_SockServer; // socket bound to random port (receives from server, sends to client)

	NETADDR m_ServerAddr; // cached upstream address
	NETADDR m_ClientAddr; // last/only connected client address (single-client proxy)

	char m_aSpoofVersion[64];
	int m_SpoofDDNet;

	// Helpers
	void HandleFromClient();
	void HandleFromServer();
	bool RewriteHandshake(unsigned char *pData, int *pSize, int maxSize);
};