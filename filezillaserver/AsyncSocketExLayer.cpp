/*CAsyncSocketEx by Tim Kosse (tim.kosse@filezilla-project.org)
            Version 1.1 (2002-11-01)
--------------------------------------------------------

Introduction:
-------------

CAsyncSocketEx is a replacement for the MFC class CAsyncSocket.
This class was written because CAsyncSocket is not the fastest WinSock
wrapper and it's very hard to add new functionality to CAsyncSocket
derived classes. This class offers the same functionality as CAsyncSocket.
Also, CAsyncSocketEx offers some enhancements which were not possible with
CAsyncSocket without some tricks.

How do I use it?
----------------
Basically exactly like CAsyncSocket.
To use CAsyncSocketEx, just replace all occurrences of CAsyncSocket in your
code with CAsyncSocketEx, if you did not enhance CAsyncSocket yourself in
any way, you won't have to change anything else in your code.

Why is CAsyncSocketEx faster?
-----------------------------

CAsyncSocketEx is slightly faster when dispatching notification event messages.
First have a look at the way CAsyncSocket works. For each thread that uses
CAsyncSocket, a window is created. CAsyncSocket calls WSAAsyncSelect with
the handle of that window. Until here, CAsyncSocketEx works the same way.
But CAsyncSocket uses only one window message (WM_SOCKET_NOTIFY) for all
sockets within one thread. When the window recieve WM_SOCKET_NOTIFY, wParam
contains the socket handle and the window looks up an CAsyncSocket instance
using a map. CAsyncSocketEx works differently. It's helper window uses a
wide range of different window messages (WM_USER through 0xBFFF) and passes
a different message to WSAAsyncSelect for each socket. When a message in
the specified range is received, CAsyncSocketEx looks up the pointer to a
CAsyncSocketEx instance in an Array using the index of message - WM_USER.
As you can see, CAsyncSocketEx uses the helper window in a more efficient
way, as it don't have to use the slow maps to lookup it's own instance.
Still, speed increase is not very much, but it may be noticeable when using
a lot of sockets at the same time.
Please note that the changes do not affect the raw data throughput rate,
CAsyncSocketEx only dispatches the notification messages faster.

What else does CAsyncSocketEx offer?
------------------------------------

CAsyncSocketEx offers a flexible layer system. One example is the proxy layer.
Just create an instance of the proxy layer, configure it and add it to the layer
chain of your CAsyncSocketEx instance. After that, you can connect through
proxies.
Benefit: You don't have to change much to use the layer system.
Another layer that is currently in development is the SSL layer to establish
SSL encrypted connections.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you use this class in commercial applications, please send a short message
to tim.kosse@filezilla-project.org
*/
#include "stdafx.h"
#include "AsyncSocketExLayer.h"

#include "AsyncSocketEx.h"

#define WM_SOCKETEX_NOTIFY (WM_USER+3)


//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////
CAsyncSocketExLayer::~CAsyncSocketExLayer()
{
	delete [] m_lpszSocketAddress;
}

CAsyncSocketExLayer *CAsyncSocketExLayer::AddLayer(CAsyncSocketExLayer *pLayer, CAsyncSocketEx *pOwnerSocket)
{
	ASSERT(pLayer);
	ASSERT(pOwnerSocket);
	if (m_pNextLayer)
	{
		return m_pNextLayer->AddLayer(pLayer, pOwnerSocket);
	}
	else
	{
		ASSERT(m_pOwnerSocket==pOwnerSocket);
		pLayer->Init(this, m_pOwnerSocket);
		m_pNextLayer=pLayer;
	}
	return m_pNextLayer;
}

int CAsyncSocketExLayer::Receive(void* lpBuf, int nBufLen, int nFlags /*=0*/)
{
	return ReceiveNext(lpBuf, nBufLen, nFlags);
}

int CAsyncSocketExLayer::Send(const void* lpBuf, int nBufLen, int nFlags /*=0*/)
{
	return SendNext(lpBuf, nBufLen, nFlags);
}


void CAsyncSocketExLayer::OnReceive(int nErrorCode)
{
	if (m_pPrevLayer)
		m_pPrevLayer->OnReceive(nErrorCode);
	else
		if (m_pOwnerSocket->m_lEvent&FD_READ)
			m_pOwnerSocket->OnReceive(nErrorCode);
}

void CAsyncSocketExLayer::OnSend(int nErrorCode)
{
	if (m_pPrevLayer)
		m_pPrevLayer->OnSend(nErrorCode);
	else
		if (m_pOwnerSocket->m_lEvent&FD_WRITE)
			m_pOwnerSocket->OnSend(nErrorCode);
}

void CAsyncSocketExLayer::OnConnect(int nErrorCode)
{
	TriggerEvent(FD_CONNECT, nErrorCode, TRUE);
}

void CAsyncSocketExLayer::OnAccept(int nErrorCode)
{
	if (m_pPrevLayer)
		m_pPrevLayer->OnAccept(nErrorCode);
	else
		if (m_pOwnerSocket->m_lEvent&FD_ACCEPT)
			m_pOwnerSocket->OnAccept(nErrorCode);
}

void CAsyncSocketExLayer::OnClose(int nErrorCode)
{
	if (m_pPrevLayer)
		m_pPrevLayer->OnClose(nErrorCode);
	else
		if (m_pOwnerSocket->m_lEvent&FD_CLOSE)
			m_pOwnerSocket->OnClose(nErrorCode);
}

BOOL CAsyncSocketExLayer::TriggerEvent(long lEvent, int nErrorCode, BOOL bPassThrough /*=FALSE*/ )
{
	ASSERT(m_pOwnerSocket);
	if (m_pOwnerSocket->m_SocketData.hSocket==INVALID_SOCKET)
		return FALSE;

	if (!bPassThrough) {
		if (m_nPendingEvents & lEvent)
			return TRUE;

		m_nPendingEvents |= lEvent;
	}

	if (lEvent & FD_CONNECT) {
		ASSERT(bPassThrough);
		if (!nErrorCode)
			ASSERT(bPassThrough && (GetLayerState()==connected || GetLayerState()==attached));
		else if (nErrorCode) {
			SetLayerState(aborted);
			m_nCriticalError = nErrorCode;
		}
	}
	else if (lEvent & FD_CLOSE) {
		if (!nErrorCode)
			SetLayerState(closed);
		else {
			SetLayerState(aborted);
			m_nCriticalError = nErrorCode;
		}
	}
	ASSERT(m_pOwnerSocket->m_pLocalAsyncSocketExThreadData);
	ASSERT(m_pOwnerSocket->m_pLocalAsyncSocketExThreadData->m_pHelperWindow);
	ASSERT(m_pOwnerSocket->m_SocketData.nSocketIndex!=-1);
	t_LayerNotifyMsg *pMsg = new t_LayerNotifyMsg;
	pMsg->hSocket = m_pOwnerSocket->m_SocketData.hSocket;
	pMsg->lEvent = ( lEvent & 0xffff ) + ( nErrorCode << 16);
	pMsg->pLayer = bPassThrough ? m_pPrevLayer : this;
	BOOL res = PostMessage(m_pOwnerSocket->GetHelperWindowHandle(), WM_USER, (WPARAM)m_pOwnerSocket->m_SocketData.nSocketIndex, (LPARAM)pMsg);
	if (!res)
		delete pMsg;
	return res;
}

void CAsyncSocketExLayer::Close()
{
	CloseNext();
}

void CAsyncSocketExLayer::CloseNext()
{
	if (m_addrInfo)
		freeaddrinfo(m_addrInfo);
	m_nextAddr = 0;
	m_addrInfo = 0;

	m_nPendingEvents = 0;

	SetLayerState(notsock);
	if (m_pNextLayer)
		m_pNextLayer->Close();
}

BOOL CAsyncSocketExLayer::Connect(LPCTSTR lpszHostAddress, UINT nHostPort)
{
	return ConnectNext(lpszHostAddress, nHostPort);
}

BOOL CAsyncSocketExLayer::Connect( const SOCKADDR* lpSockAddr, int nSockAddrLen )
{
	return ConnectNext(lpSockAddr, nSockAddrLen);
}

int CAsyncSocketExLayer::SendNext(const void *lpBuf, int nBufLen, int nFlags /*=0*/)
{
	if (m_nCriticalError)
	{
		WSASetLastError(m_nCriticalError);
		return SOCKET_ERROR;
	}
	else if (GetLayerState()==notsock)
	{
		WSASetLastError(WSAENOTSOCK);
		return SOCKET_ERROR;
	}
	else if (GetLayerState()==unconnected || GetLayerState()==connecting || GetLayerState()==listening)
	{
		WSASetLastError(WSAENOTCONN);
		return SOCKET_ERROR;
	}

	if (!m_pNextLayer)
	{
		ASSERT(m_pOwnerSocket);
		return send(m_pOwnerSocket->GetSocketHandle(), (LPSTR)lpBuf, nBufLen, nFlags);
	}
	else
		return m_pNextLayer->Send(lpBuf, nBufLen, nFlags);
}

int CAsyncSocketExLayer::ReceiveNext(void *lpBuf, int nBufLen, int nFlags /*=0*/)
{
	if (m_nCriticalError)
	{
		WSASetLastError(m_nCriticalError);
		return SOCKET_ERROR;
	}
	else if (GetLayerState()==notsock)
	{
		WSASetLastError(WSAENOTSOCK);
		return SOCKET_ERROR;
	}
	else if (GetLayerState()==unconnected || GetLayerState()==connecting || GetLayerState()==listening)
	{
		WSASetLastError(WSAENOTCONN);
		return SOCKET_ERROR;
	}

	if (!m_pNextLayer)
	{
		ASSERT(m_pOwnerSocket);
		return recv(m_pOwnerSocket->GetSocketHandle(), (LPSTR)lpBuf, nBufLen, nFlags);
	}
	else
		return m_pNextLayer->Receive(lpBuf, nBufLen, nFlags);
}

BOOL CAsyncSocketExLayer::ConnectNext(LPCTSTR lpszHostAddress, UINT nHostPort)
{
	ASSERT(GetLayerState() == unconnected);
	ASSERT(m_pOwnerSocket);
	BOOL res{};
	if (m_pNextLayer)
		res = m_pNextLayer->Connect(lpszHostAddress, nHostPort);
	else if (m_nFamily == AF_INET) {
		USES_CONVERSION;

		ASSERT(lpszHostAddress != NULL);

		SOCKADDR_IN sockAddr{};

		LPSTR lpszAscii = T2A((LPTSTR)lpszHostAddress);
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);

		if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
			LPHOSTENT lphost;
			lphost = gethostbyname(lpszAscii);
			if (lphost != NULL)
				sockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
			else {
				WSASetLastError(WSAEINVAL);
				res = FALSE;
			}
		}

		sockAddr.sin_port = htons((u_short)nHostPort);

		res = (SOCKET_ERROR != connect(m_pOwnerSocket->GetSocketHandle(), (SOCKADDR*)&sockAddr, sizeof(sockAddr)) );
	}
	else if (m_nFamily == AF_INET6 || m_nFamily == AF_UNSPEC) {
		USES_CONVERSION;

		ASSERT(lpszHostAddress != NULL);

		addrinfo hints{}, *res0{}, *res1{};
		SOCKET hSocket;
		int error;
		char port[10];

		freeaddrinfo(m_addrInfo);
		m_nextAddr = 0;
		m_addrInfo = 0;

		hints.ai_family = m_nFamily;
		hints.ai_socktype = SOCK_STREAM;
		_snprintf(port, 9, "%lu", nHostPort);
		port[9] = 0;
		error = getaddrinfo(T2CA(lpszHostAddress), port, &hints, &res0);
		if (error)
			return FALSE;

		for (res1 = res0; res1; res1 = res1->ai_next) {
			if (m_nFamily == AF_UNSPEC)
				hSocket = socket(res1->ai_family, res1->ai_socktype, res1->ai_protocol);
			else
				hSocket = m_pOwnerSocket->GetSocketHandle();

			if (INVALID_SOCKET == hSocket) {
				res = FALSE;
				continue;
			}

			if (m_nFamily == AF_UNSPEC) {
				m_pOwnerSocket->AttachHandle(hSocket, res1->ai_family);
				if (!m_pOwnerSocket->AsyncSelect(m_lEvent)) {
					m_pOwnerSocket->Close();
					res = FALSE;
					continue ;
				}
				if (m_pOwnerSocket->m_pFirstLayer) {
					if (WSAAsyncSelect(m_pOwnerSocket->m_SocketData.hSocket, m_pOwnerSocket->GetHelperWindowHandle(), m_pOwnerSocket->m_SocketData.nSocketIndex + WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE) ) {
						m_pOwnerSocket->Close();
						res = FALSE;
						continue;
					}
				}
				if (m_pOwnerSocket->m_pendingCallbacks.size())
					PostMessage(m_pOwnerSocket->GetHelperWindowHandle(), WM_USER + 2, (WPARAM)m_pOwnerSocket->m_SocketData.nSocketIndex, 0);
			}

			if (m_nFamily == AF_UNSPEC) {
				m_pOwnerSocket->m_SocketData.nFamily = m_nFamily = res1->ai_family;
				if (!m_pOwnerSocket->Bind(m_nSocketPort, m_lpszSocketAddress)) {
					m_pOwnerSocket->m_SocketData.nFamily = m_nFamily = AF_UNSPEC;
					Close();
					continue;
				}
			}

			if (!( res = ( SOCKET_ERROR != connect(m_pOwnerSocket->GetSocketHandle(), res1->ai_addr, res1->ai_addrlen) ) )
				&& WSAGetLastError() != WSAEWOULDBLOCK)
			{
				if (hints.ai_family == AF_UNSPEC) {
					m_nFamily = AF_UNSPEC;
					Close();
				}
				continue;
			}

			m_nFamily = res1->ai_family;
			m_pOwnerSocket->m_SocketData.nFamily = res1->ai_family;
			res = TRUE;
			break;
		}

		if (res1)
			res1 = res0->ai_next;

		if (res1) {
			m_addrInfo = res0;
			m_nextAddr = res1;
		}
		else
			freeaddrinfo(res0);

		if (INVALID_SOCKET == m_pOwnerSocket->GetSocketHandle())
			res = FALSE;
	}
	else {
		WSASetLastError(WSAEPROTONOSUPPORT);
		res = false;
	}

	if (res || WSAGetLastError() == WSAEWOULDBLOCK) {
		SetLayerState(connecting);
	}
	return res;
}

BOOL CAsyncSocketExLayer::ConnectNext( const SOCKADDR* lpSockAddr, int nSockAddrLen )
{
	ASSERT(GetLayerState()==unconnected);
	ASSERT(m_pOwnerSocket);
	BOOL res;
	if (m_pNextLayer)
		res=m_pNextLayer->Connect(lpSockAddr, nSockAddrLen);
	else
		res = (SOCKET_ERROR!=connect(m_pOwnerSocket->GetSocketHandle(), lpSockAddr, nSockAddrLen));

	if (res || WSAGetLastError()==WSAEWOULDBLOCK)
		SetLayerState(connecting);
	return res;
}

void CAsyncSocketExLayer::Init(CAsyncSocketExLayer *pPrevLayer, CAsyncSocketEx *pOwnerSocket)
{
	ASSERT(pOwnerSocket);
	m_pPrevLayer=pPrevLayer;
	m_pOwnerSocket=pOwnerSocket;
	m_pNextLayer=0;
#ifndef NOSOCKETSTATES
	SetLayerState(pOwnerSocket->GetState());
#endif //NOSOCKETSTATES
}

int CAsyncSocketExLayer::GetLayerState()
{
	return m_nLayerState;
}

void CAsyncSocketExLayer::SetLayerState(int nLayerState)
{
	ASSERT(m_pOwnerSocket);
	int nOldLayerState = GetLayerState();
	m_nLayerState = nLayerState;
	if (nOldLayerState != nLayerState)
		DoLayerCallback(LAYERCALLBACK_STATECHANGE, GetLayerState(), nOldLayerState);
}

void CAsyncSocketExLayer::CallEvent(int nEvent, int nErrorCode)
{
	if (m_nCriticalError)
		return;
	m_nCriticalError = nErrorCode;
	switch (nEvent)
	{
	case FD_READ:
	case FD_FORCEREAD:
		if (GetLayerState()==connecting && !nErrorCode)
		{
			m_nPendingEvents |= nEvent;
			break;
		}
		else if (GetLayerState()==attached)
			SetLayerState(connected);
		if (nEvent & FD_READ)
			m_nPendingEvents &= ~FD_READ;
		else
			m_nPendingEvents &= ~FD_FORCEREAD;
		if (GetLayerState()==connected || nErrorCode)
		{
			if (nErrorCode)
				SetLayerState(aborted);
			OnReceive(nErrorCode);
		}
		break;
	case FD_WRITE:
		if (GetLayerState()==connecting && !nErrorCode)
		{
			m_nPendingEvents |= nEvent;
			break;
		}
		else if (GetLayerState()==attached)
			SetLayerState(connected);
		m_nPendingEvents &= ~FD_WRITE;
		if (GetLayerState()==connected || nErrorCode)
		{
			if (nErrorCode)
				SetLayerState(aborted);
			OnSend(nErrorCode);
		}
		break;
	case FD_CONNECT:
		if (GetLayerState()==connecting || GetLayerState() == attached)
		{
			if (!nErrorCode)
				SetLayerState(connected);
			else
			{
				if (!m_pNextLayer && m_nextAddr)
					if (TryNextProtocol())
					{
						m_nCriticalError = 0;
						return;
					}
				SetLayerState(aborted);
			}
			m_nPendingEvents &= ~FD_CONNECT;
			OnConnect(nErrorCode);

			if (!nErrorCode)
			{
				if ((m_nPendingEvents & FD_READ) && GetLayerState()==connected)
					OnReceive(0);
				if ((m_nPendingEvents & FD_FORCEREAD) && GetLayerState()==connected)
					OnReceive(0);
				if ((m_nPendingEvents & FD_WRITE) && GetLayerState()==connected)
					OnSend(0);
			}
			m_nPendingEvents = 0;
		}
		break;
	case FD_ACCEPT:
		if (GetLayerState()==listening)
		{
			if (nErrorCode)
				SetLayerState(aborted);
			m_nPendingEvents &= ~FD_ACCEPT;
			OnAccept(nErrorCode);
		}
		break;
	case FD_CLOSE:
		if (GetLayerState()==connected || GetLayerState()==attached)
		{
			if (nErrorCode)
				SetLayerState(aborted);
			else
				SetLayerState(closed);
			m_nPendingEvents &= ~FD_CLOSE;
			OnClose(nErrorCode);
		}
		break;
	}
}

//Creates a socket
bool CAsyncSocketExLayer::Create(UINT nSocketPort, int nSocketType,
			long lEvent, LPCTSTR lpszSocketAddress, int nFamily /*=AF_INET*/, bool reusable /*=false*/)
{
	return CreateNext(nSocketPort, nSocketType, lEvent, lpszSocketAddress, nFamily, reusable);
}

bool CAsyncSocketExLayer::CreateNext(UINT nSocketPort, int nSocketType, long lEvent, LPCTSTR lpszSocketAddress, int nFamily /*=AF_INET*/, bool reusable /*=false*/)
{
	ASSERT(GetLayerState()==notsock);
	bool res = false;

	m_nFamily = nFamily;

	if (m_pNextLayer) {
		res = m_pNextLayer->Create(nSocketPort, nSocketType, lEvent, lpszSocketAddress, nFamily);
	}
	else if (m_nFamily == AF_UNSPEC) {
		m_lEvent = lEvent;
		delete [] m_lpszSocketAddress;
		if (lpszSocketAddress && *lpszSocketAddress) {
			m_lpszSocketAddress = new TCHAR[_tcslen(lpszSocketAddress) + 1];
			_tcscpy(m_lpszSocketAddress, lpszSocketAddress);
		}
		else {
			m_lpszSocketAddress = 0;
		}
		m_nSocketPort = nSocketPort;
		res = true;
	}
	else {
		SOCKET hSocket = socket(nFamily, nSocketType, 0);
		if (hSocket == INVALID_SOCKET) {
			m_pOwnerSocket->Close();
			return false;
		}
		m_pOwnerSocket->AttachHandle(hSocket, nFamily);
		if (!m_pOwnerSocket->AsyncSelect(lEvent)) {
			m_pOwnerSocket->Close();
			return false;
		}
		if (m_pOwnerSocket->m_pFirstLayer) {
			if (WSAAsyncSelect(m_pOwnerSocket->m_SocketData.hSocket, m_pOwnerSocket->GetHelperWindowHandle(), m_pOwnerSocket->m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE) ) {
				m_pOwnerSocket->Close();
				return false;
			}
		}

		if (reusable && nSocketPort != 0) {
			BOOL value = TRUE;
			m_pOwnerSocket->SetSockOpt(SO_REUSEADDR, reinterpret_cast<const void*>(&value), sizeof(value));
		}

		if (!m_pOwnerSocket->Bind(nSocketPort, lpszSocketAddress)) {
			m_pOwnerSocket->Close();
			return false;
		}
		res = true;
	}
	if (res) {
		SetLayerState(unconnected);
	}
	return res;
}

int CAsyncSocketExLayer::DoLayerCallback(int nType, int nParam1, int nParam2, char* str /*=0*/)
{
	if (!m_pOwnerSocket)
		return 0;

	int nError = WSAGetLastError();

	t_callbackMsg msg;
	msg.pLayer = this;
	msg.nType = nType;
	msg.nParam1 = nParam1;
	msg.nParam2 = nParam2;
	msg.str = str;

	m_pOwnerSocket->AddCallbackNotification(msg);

	WSASetLastError(nError);

	return 0;
}

BOOL CAsyncSocketExLayer::Listen( int nConnectionBacklog)
{
	return ListenNext( nConnectionBacklog);
}

BOOL CAsyncSocketExLayer::ListenNext( int nConnectionBacklog)
{
	ASSERT(GetLayerState()==unconnected);
	BOOL res;
	if (m_pNextLayer)
		res=m_pNextLayer->Listen(nConnectionBacklog);
	else
		res=listen(m_pOwnerSocket->GetSocketHandle(), nConnectionBacklog);
	if (res!=SOCKET_ERROR)
	{
		SetLayerState(listening);
	}
	return res!=SOCKET_ERROR;
}

BOOL CAsyncSocketExLayer::Accept( CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr /*=NULL*/, int* lpSockAddrLen /*=NULL*/ )
{
	return AcceptNext(rConnectedSocket, lpSockAddr, lpSockAddrLen);
}

BOOL CAsyncSocketExLayer::AcceptNext( CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr /*=NULL*/, int* lpSockAddrLen /*=NULL*/ )
{
	ASSERT(GetLayerState()==listening);
	BOOL res;
	if (m_pNextLayer)
		res=m_pNextLayer->Accept(rConnectedSocket, lpSockAddr, lpSockAddrLen);
	else
	{
		SOCKET hTemp = accept(m_pOwnerSocket->m_SocketData.hSocket, lpSockAddr, lpSockAddrLen);

		if (hTemp == INVALID_SOCKET)
			return FALSE;
		VERIFY(rConnectedSocket.InitAsyncSocketExInstance());
		rConnectedSocket.AttachHandle(hTemp, GetFamily());
#ifndef NOSOCKETSTATES
		rConnectedSocket.SetState(connected);
#endif //NOSOCKETSTATES
	}
	return TRUE;
}

BOOL CAsyncSocketExLayer::ShutDown()
{
	return ShutDownNext();
}

BOOL CAsyncSocketExLayer::ShutDownNext()
{
	if (m_nCriticalError) {
		WSASetLastError(m_nCriticalError);
		return FALSE;
	}
	else if (GetLayerState() == notsock) {
		WSASetLastError(WSAENOTSOCK);
		return FALSE;
	}
	else if (GetLayerState() == unconnected || GetLayerState() == connecting || GetLayerState() == listening) {
		WSASetLastError(WSAENOTCONN);
		return FALSE;
	}

	if (!m_pNextLayer) {
		ASSERT(m_pOwnerSocket);
		return shutdown(m_pOwnerSocket->GetSocketHandle(), SD_SEND) == 0;
	}
	else
		return m_pNextLayer->ShutDownNext();
}

int CAsyncSocketExLayer::GetFamily() const
{
	return m_nFamily;
}

bool CAsyncSocketExLayer::SetFamily(int nFamily)
{
	if (m_nFamily != AF_UNSPEC)
		return false;

	m_nFamily = nFamily;
	return true;
}

bool CAsyncSocketExLayer::TryNextProtocol()
{
	closesocket(m_pOwnerSocket->m_SocketData.hSocket);
	m_pOwnerSocket->DetachHandle();

	BOOL ret = FALSE;
	for (; m_nextAddr; m_nextAddr = m_nextAddr->ai_next) {
		SOCKET hSocket = socket(m_nextAddr->ai_family, m_nextAddr->ai_socktype, m_nextAddr->ai_protocol);

		if (hSocket == INVALID_SOCKET)
			continue;

		m_pOwnerSocket->AttachHandle(hSocket, m_nextAddr->ai_family);
		if (!m_pOwnerSocket->AsyncSelect(m_lEvent)) {
			closesocket(m_pOwnerSocket->m_SocketData.hSocket);
			m_pOwnerSocket->DetachHandle();
			continue;
		}

		if (m_pOwnerSocket->m_pFirstLayer) {
			if (WSAAsyncSelect(m_pOwnerSocket->m_SocketData.hSocket, m_pOwnerSocket->GetHelperWindowHandle(), m_pOwnerSocket->m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE))
			{
				closesocket(m_pOwnerSocket->m_SocketData.hSocket);
				m_pOwnerSocket->DetachHandle();
				continue;
			}
		}

		m_pOwnerSocket->m_SocketData.nFamily = m_nextAddr->ai_family;
		m_nFamily = m_nextAddr->ai_family;
		if (!m_pOwnerSocket->Bind(m_nSocketPort, m_lpszSocketAddress))
		{
			closesocket(m_pOwnerSocket->m_SocketData.hSocket);
			m_pOwnerSocket->DetachHandle();
			continue;
		}

		if (connect(m_pOwnerSocket->GetSocketHandle(), m_nextAddr->ai_addr, m_nextAddr->ai_addrlen) == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
			closesocket(m_pOwnerSocket->m_SocketData.hSocket);
			m_pOwnerSocket->DetachHandle();
			continue;
		}

		SetLayerState(connecting);

		ret = true;
		break;
	}

	if (m_nextAddr)
		m_nextAddr = m_nextAddr->ai_next;

	if (!m_nextAddr) {
		freeaddrinfo(m_addrInfo);
		m_nextAddr = 0;
		m_addrInfo = 0;
	}

	if (m_pOwnerSocket->m_SocketData.hSocket == INVALID_SOCKET || !ret)
		return FALSE;
	else
		return TRUE;
}