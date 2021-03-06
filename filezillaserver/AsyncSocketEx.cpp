/*CAsyncSocketEx by Tim Kosse (tim.kosse@filezilla-project.org)
            Version 1.3 (2003-04-26)
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
#include "AsyncSocketEx.h"

#include "AsyncSocketExLayer.h"

thread_local CAsyncSocketEx::t_AsyncSocketExThreadData* CAsyncSocketEx::thread_local_data = 0;

#ifndef _AFX
#ifndef VERIFY
#define VERIFY(x) (void(x))
#endif //VERIFY
#ifndef ASSERT
#define ASSERT(x)
#endif //ASSERT
#endif //_AFX

/////////////////////////////
//Helper Window class
#define WM_SOCKETEX_NOTIFY (WM_USER+3)
#define MAX_SOCKETS (0xBFFF-WM_SOCKETEX_NOTIFY+1)

class CAsyncSocketExHelperWindow
{
public:
	CAsyncSocketExHelperWindow(CAsyncSocketEx::t_AsyncSocketExThreadData* pThreadData)
	{
		//Initialize data
		m_pAsyncSocketExWindowData = new t_AsyncSocketExWindowData[512]; //Reserve space for 512 active sockets
		memset(m_pAsyncSocketExWindowData, 0, 512*sizeof(t_AsyncSocketExWindowData));
		m_nWindowDataSize = 512;
		m_nSocketCount = 0;
		m_nWindowDataPos = 0;
		m_pThreadData = pThreadData;

		//Create window
		WNDCLASSEX wndclass;
		wndclass.cbSize = sizeof wndclass;
		wndclass.style = 0;
		wndclass.lpfnWndProc = WindowProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = GetModuleHandle(0);
		wndclass.hIcon = 0;
		wndclass.hCursor = 0;
		wndclass.hbrBackground = 0;
		wndclass.lpszMenuName = 0;
		wndclass.lpszClassName = _T("CAsyncSocketEx Helper Window");
		wndclass.hIconSm = 0;

		RegisterClassEx(&wndclass);

		m_hWnd = CreateWindow(_T("CAsyncSocketEx Helper Window"), _T("CAsyncSocketEx Helper Window"), 0, 0, 0, 0, 0, 0, 0, 0, GetModuleHandle(0));
		ASSERT(m_hWnd);
		if (m_hWnd) {
			SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
		}
	};

	virtual ~CAsyncSocketExHelperWindow()
	{
		//Clean up socket storage
		delete [] m_pAsyncSocketExWindowData;
		m_pAsyncSocketExWindowData = 0;
		m_nWindowDataSize = 0;
		m_nSocketCount = 0;

		//Destroy window
		if (m_hWnd) {
			DestroyWindow(m_hWnd);
			m_hWnd = 0;
		}
	}

	//Adds a socket to the list of attached sockets
	BOOL AddSocket(CAsyncSocketEx *pSocket, int &nSocketIndex)
	{
		ASSERT(pSocket);
		if (!pSocket) {
			return FALSE;
		}
		if (!m_nWindowDataSize) {
			ASSERT(!m_nSocketCount);
			m_nWindowDataSize = 512;
			m_pAsyncSocketExWindowData = new t_AsyncSocketExWindowData[512]; //Reserve space for 512 active sockets
			memset(m_pAsyncSocketExWindowData, 0, 512 * sizeof(t_AsyncSocketExWindowData));
		}

		if (nSocketIndex != -1) {
			ASSERT(m_pAsyncSocketExWindowData);
			ASSERT(m_nWindowDataSize>nSocketIndex);
			ASSERT(m_pAsyncSocketExWindowData[nSocketIndex].m_pSocket == pSocket);
			ASSERT(m_nSocketCount);
			if (!m_pAsyncSocketExWindowData) {
				return FALSE;
			}
			return TRUE;
		}

		//Increase socket storage if too small
		if (m_nSocketCount >= (m_nWindowDataSize - 10)) {
			int nOldWindowDataSize = m_nWindowDataSize;
			ASSERT(m_nWindowDataSize < MAX_SOCKETS);
			m_nWindowDataSize += 512;
			if (m_nWindowDataSize > MAX_SOCKETS)
				m_nWindowDataSize = MAX_SOCKETS;
			t_AsyncSocketExWindowData *tmp = m_pAsyncSocketExWindowData;
			m_pAsyncSocketExWindowData = new t_AsyncSocketExWindowData[m_nWindowDataSize];
			memcpy(m_pAsyncSocketExWindowData, tmp, nOldWindowDataSize * sizeof(t_AsyncSocketExWindowData));
			memset(m_pAsyncSocketExWindowData + nOldWindowDataSize, 0, (m_nWindowDataSize - nOldWindowDataSize) * sizeof(t_AsyncSocketExWindowData));
			delete [] tmp;
		}

		//Search for free slot
		for (int i = m_nWindowDataPos; i < (m_nWindowDataSize + m_nWindowDataPos); ++i) {
			if (!m_pAsyncSocketExWindowData[i % m_nWindowDataSize].m_pSocket) {
				m_pAsyncSocketExWindowData[i % m_nWindowDataSize].m_pSocket = pSocket;
				nSocketIndex = i % m_nWindowDataSize;
				m_nWindowDataPos = (i + 1) % m_nWindowDataSize;
				++m_nSocketCount;
				return TRUE;
			}
		}

		//No slot found, maybe there are too much sockets!
		return FALSE;
	}

	//Removes a socket from the socket storage
	BOOL RemoveSocket(CAsyncSocketEx *pSocket, int &nSocketIndex)
	{
		ASSERT(pSocket);
		if (!pSocket)
			return FALSE;
		if (nSocketIndex == -1)
			return TRUE;

		// Remove additional messages from queue
		MSG msg;
		while (PeekMessage(&msg, m_hWnd, WM_SOCKETEX_NOTIFY + nSocketIndex, WM_SOCKETEX_NOTIFY + nSocketIndex, PM_REMOVE | PM_NOYIELD));

		ASSERT(m_pAsyncSocketExWindowData);
		ASSERT(m_nWindowDataSize > 0);
		ASSERT(m_nSocketCount > 0);
		ASSERT(m_pAsyncSocketExWindowData[nSocketIndex].m_pSocket == pSocket);
		m_pAsyncSocketExWindowData[nSocketIndex].m_pSocket = 0;
		nSocketIndex = -1;
		--m_nSocketCount;

		return TRUE;
	}

	void RemoveLayers(CAsyncSocketEx *pOrigSocket)
	{
		// Remove all layer messages from old socket
		std::vector<MSG> msgList;
		MSG msg;
		while (PeekMessage(&msg, m_hWnd, WM_USER, WM_USER, PM_REMOVE | PM_NOYIELD)) {
			//Verify parameters, lookup socket and notification message
			//Verify parameters
			if (msg.wParam >= static_cast<UINT>(m_nWindowDataSize)) //Index is within socket storage
				continue;

			CAsyncSocketEx *pSocket = m_pAsyncSocketExWindowData[msg.wParam].m_pSocket;
			CAsyncSocketExLayer::t_LayerNotifyMsg *pMsg = (CAsyncSocketExLayer::t_LayerNotifyMsg *)msg.lParam;
			if (!pMsg || !pSocket || pSocket == pOrigSocket || pSocket->m_SocketData.hSocket != pMsg->hSocket) {
				delete pMsg;
				continue;
			}

			msgList.push_back(msg);
		}

		for (auto const& msg : msgList) {
			if (!PostMessage(m_hWnd, msg.message, msg.wParam, msg.lParam)) {
				CAsyncSocketExLayer::t_LayerNotifyMsg *pMsg = (CAsyncSocketExLayer::t_LayerNotifyMsg *)msg.lParam;
				delete pMsg;
			}
		}
	}

	//Processes event notifications sent by the sockets or the layers
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message>=WM_SOCKETEX_NOTIFY)
		{
			//Verify parameters
			ASSERT(hWnd);
			CAsyncSocketExHelperWindow *pWnd=(CAsyncSocketExHelperWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			ASSERT(pWnd);
			if (!pWnd)
				return 0;

			if (message < static_cast<UINT>(WM_SOCKETEX_NOTIFY+pWnd->m_nWindowDataSize)) //Index is within socket storage
			{
				//Lookup socket and verify if it's valid
				CAsyncSocketEx *pSocket=pWnd->m_pAsyncSocketExWindowData[message - WM_SOCKETEX_NOTIFY].m_pSocket;
				SOCKET hSocket = wParam;
				if (!pSocket)
					return 0;
				if (hSocket == INVALID_SOCKET)
					return 0;
				if (pSocket->m_SocketData.hSocket != hSocket)
					return 0;

				int nEvent = lParam & 0xFFFF;
				int nErrorCode = lParam >> 16;

				//Dispatch notification
				if (!pSocket->m_pFirstLayer) {
					//Dispatch to CAsyncSocketEx instance
					switch (nEvent)
					{
					case FD_READ:
#ifndef NOSOCKETSTATES
						if (pSocket->GetState() == connecting && !nErrorCode)
						{
							pSocket->m_nPendingEvents |= FD_READ;
							break;
						}
						else if (pSocket->GetState() == attached)
							pSocket->SetState(connected);
						if (pSocket->GetState() != connected)
							break;

						// Ignore further FD_READ events after FD_CLOSE has been received
						if (pSocket->m_SocketData.onCloseCalled)
							break;
#endif //NOSOCKETSTATES

#ifndef NOSOCKETSTATES
						if (nErrorCode)
							pSocket->SetState(aborted);
#endif //NOSOCKETSTATES
						if (pSocket->m_lEvent & FD_READ) {
							pSocket->OnReceive(nErrorCode);
						}
						break;
					case FD_FORCEREAD: //Forceread does not check if there's data waiting
#ifndef NOSOCKETSTATES
						if (pSocket->GetState() == connecting && !nErrorCode)
						{
							pSocket->m_nPendingEvents |= FD_FORCEREAD;
							break;
						}
						else if (pSocket->GetState() == attached)
							pSocket->SetState(connected);
						if (pSocket->GetState() != connected)
							break;
#endif //NOSOCKETSTATES
						if (pSocket->m_lEvent & FD_READ)
						{
#ifndef NOSOCKETSTATES
							if (nErrorCode)
								pSocket->SetState(aborted);
#endif //NOSOCKETSTATES
							pSocket->OnReceive(nErrorCode);
						}
						break;
					case FD_WRITE:
#ifndef NOSOCKETSTATES
						if (pSocket->GetState() == connecting && !nErrorCode)
						{
							pSocket->m_nPendingEvents |= FD_WRITE;
							break;
						}
						else if (pSocket->GetState() == attached && !nErrorCode)
							pSocket->SetState(connected);
						if (pSocket->GetState() != connected)
							break;
#endif //NOSOCKETSTATES
						if (pSocket->m_lEvent & FD_WRITE)
						{
#ifndef NOSOCKETSTATES
							if (nErrorCode)
								pSocket->SetState(aborted);
#endif //NOSOCKETSTATES
							pSocket->OnSend(nErrorCode);
						}
						break;
					case FD_CONNECT:
#ifndef NOSOCKETSTATES
						if (pSocket->GetState() == connecting)
						{
							if (nErrorCode && pSocket->m_SocketData.nextAddr)
							{
								if (pSocket->TryNextProtocol())
									break;
							}
							pSocket->SetState(connected);
						}
						else if (pSocket->GetState() == attached && !nErrorCode)
							pSocket->SetState(connected);
#endif //NOSOCKETSTATES
						if (pSocket->m_lEvent & FD_CONNECT)
							pSocket->OnConnect(nErrorCode);
#ifndef NOSOCKETSTATES
						if (!nErrorCode)
						{
							if ((pSocket->m_nPendingEvents&FD_READ) && pSocket->GetState() == connected)
								pSocket->OnReceive(0);
							if ((pSocket->m_nPendingEvents&FD_FORCEREAD) && pSocket->GetState() == connected)
								pSocket->OnReceive(0);
							if ((pSocket->m_nPendingEvents&FD_WRITE) && pSocket->GetState() == connected)
								pSocket->OnSend(0);
						}
						pSocket->m_nPendingEvents = 0;
#endif
						break;
					case FD_ACCEPT:
#ifndef NOSOCKETSTATES
						if (pSocket->GetState() != listening && pSocket->GetState() != attached)
							break;
#endif //NOSOCKETSTATES
						if (pSocket->m_lEvent & FD_ACCEPT)
							pSocket->OnAccept(nErrorCode);
						break;
					case FD_CLOSE:
#ifndef NOSOCKETSTATES
						if (pSocket->GetState() != connected && pSocket->GetState() != attached)
							break;

						// If there are still bytes left to read, call OnReceive instead of
						// OnClose and trigger a new OnClose
						DWORD nBytes = 0;
						if (!nErrorCode && pSocket->IOCtl(FIONREAD, &nBytes))
						{
							if (nBytes > 0)
							{
								// Just repeat message.
								pSocket->ResendCloseNotify();
								pSocket->m_SocketData.onCloseCalled = true;
								pSocket->OnReceive(WSAESHUTDOWN);
								break;
							}
						}

						pSocket->SetState(nErrorCode ? aborted : closed);
#endif //NOSOCKETSTATES
						pSocket->OnClose(nErrorCode);
						break;
					}
				}
				else //Dispatch notification to the lowest layer
				{
					if (nEvent == FD_READ)
					{
						// Ignore further FD_READ events after FD_CLOSE has been received
						if (pSocket->m_SocketData.onCloseCalled)
							return 0;

						DWORD nBytes;
						if (!pSocket->IOCtl(FIONREAD, &nBytes))
							nErrorCode = WSAGetLastError();
						if (pSocket->m_pLastLayer)
							pSocket->m_pLastLayer->CallEvent(nEvent, nErrorCode);
					}
					else if (nEvent == FD_CLOSE)
					{
						// If there are still bytes left to read, call OnReceive instead of
						// OnClose and trigger a new OnClose
						DWORD nBytes = 0;
						if (!nErrorCode && pSocket->IOCtl(FIONREAD, &nBytes))
						{
							if (nBytes > 0)
							{
								// Just repeat message.
								pSocket->ResendCloseNotify();
								if (pSocket->m_pLastLayer)
									pSocket->m_pLastLayer->CallEvent(FD_READ, 0);
								return 0;
							}
						}
						pSocket->m_SocketData.onCloseCalled = true;
						if (pSocket->m_pLastLayer)
							pSocket->m_pLastLayer->CallEvent(nEvent, nErrorCode);
					}
					else if (pSocket->m_pLastLayer)
						pSocket->m_pLastLayer->CallEvent(nEvent, nErrorCode);
				}
			}
			return 0;
		}
		else if (message == WM_USER) //Notification event sent by a layer
		{
			//Verify parameters, lookup socket and notification message
			//Verify parameters
			ASSERT(hWnd);
			CAsyncSocketExHelperWindow *pWnd=(CAsyncSocketExHelperWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			ASSERT(pWnd);
			if (!pWnd)
				return 0;

			if (wParam >= static_cast<UINT>(pWnd->m_nWindowDataSize)) //Index is within socket storage
			{
				return 0;
			}

			CAsyncSocketEx *pSocket = pWnd->m_pAsyncSocketExWindowData[wParam].m_pSocket;
			CAsyncSocketExLayer::t_LayerNotifyMsg *pMsg = (CAsyncSocketExLayer::t_LayerNotifyMsg *)lParam;
			if (!pMsg || !pSocket || pSocket->m_SocketData.hSocket != pMsg->hSocket)
			{
				delete pMsg;
				return 0;
			}
			int nEvent=pMsg->lEvent&0xFFFF;
			int nErrorCode=pMsg->lEvent>>16;

			//Dispatch to layer
			if (pMsg->pLayer)
				pMsg->pLayer->CallEvent(nEvent, nErrorCode);
			else
			{
				//Dispatch to CAsyncSocketEx instance
				switch (nEvent)
				{
				case FD_READ:
#ifndef NOSOCKETSTATES
					if (pSocket->GetState() == connecting && !nErrorCode)
					{
						pSocket->m_nPendingEvents |= FD_READ;
						break;
					}
					else if (pSocket->GetState() == attached && !nErrorCode)
						pSocket->SetState(connected);
					if (pSocket->GetState() != connected)
						break;
#endif //NOSOCKETSTATES
					if (pSocket->m_lEvent & FD_READ)
					{
#ifndef NOSOCKETSTATES
						if (nErrorCode)
							pSocket->SetState(aborted);
#endif //NOSOCKETSTATES
						pSocket->OnReceive(nErrorCode);
					}
					break;
				case FD_FORCEREAD: //Forceread does not check if there's data waiting
#ifndef NOSOCKETSTATES
					if (pSocket->GetState() == connecting && !nErrorCode)
					{
						pSocket->m_nPendingEvents |= FD_FORCEREAD;
						break;
					}
					else if (pSocket->GetState() == attached && !nErrorCode)
						pSocket->SetState(connected);
					if (pSocket->GetState() != connected)
						break;
#endif //NOSOCKETSTATES
					if (pSocket->m_lEvent & FD_READ)
					{
#ifndef NOSOCKETSTATES
						if (nErrorCode)
							pSocket->SetState(aborted);
#endif //NOSOCKETSTATES
						pSocket->OnReceive(nErrorCode);
					}
					break;
				case FD_WRITE:
#ifndef NOSOCKETSTATES
					if (pSocket->GetState() == connecting && !nErrorCode)
					{
						pSocket->m_nPendingEvents |= FD_WRITE;
						break;
					}
					else if (pSocket->GetState() == attached && !nErrorCode)
						pSocket->SetState(connected);
					if (pSocket->GetState() != connected)
						break;
#endif //NOSOCKETSTATES
					if (pSocket->m_lEvent & FD_WRITE)
					{
#ifndef NOSOCKETSTATES
						if (nErrorCode)
							pSocket->SetState(aborted);
#endif //NOSOCKETSTATES
						pSocket->OnSend(nErrorCode);
					}
					break;
				case FD_CONNECT:
#ifndef NOSOCKETSTATES
					if (pSocket->GetState() == connecting)
						pSocket->SetState(connected);
					else if (pSocket->GetState() == attached && !nErrorCode)
						pSocket->SetState(connected);
#endif //NOSOCKETSTATES
					if (pSocket->m_lEvent & FD_CONNECT)
						pSocket->OnConnect(nErrorCode);
#ifndef NOSOCKETSTATES
					if (!nErrorCode)
					{
						if (((pSocket->m_nPendingEvents&FD_READ) && pSocket->GetState() == connected) && (pSocket->m_lEvent & FD_READ))
							pSocket->OnReceive(0);
						if (((pSocket->m_nPendingEvents&FD_FORCEREAD) && pSocket->GetState() == connected) && (pSocket->m_lEvent & FD_READ))
							pSocket->OnReceive(0);
						if (((pSocket->m_nPendingEvents&FD_WRITE) && pSocket->GetState() == connected) && (pSocket->m_lEvent & FD_WRITE))
							pSocket->OnSend(0);
					}
					pSocket->m_nPendingEvents = 0;
#endif //NOSOCKETSTATES
					break;
				case FD_ACCEPT:
#ifndef NOSOCKETSTATES
					if ((pSocket->GetState() == listening || pSocket->GetState() == attached) && (pSocket->m_lEvent & FD_ACCEPT))
#endif //NOSOCKETSTATES
					{
						pSocket->OnAccept(nErrorCode);
					}
					break;
				case FD_CLOSE:
#ifndef NOSOCKETSTATES
					if ((pSocket->GetState() == connected || pSocket->GetState() == attached) && (pSocket->m_lEvent & FD_CLOSE))
					{
						pSocket->SetState(nErrorCode?aborted:closed);
#else
					{
#endif //NOSOCKETSTATES
						pSocket->OnClose(nErrorCode);
					}
					break;
				}
			}
			delete pMsg;
			return 0;
		}
		else if (message == WM_USER+1)
		{
			// WSAAsyncGetHostByName reply

			// Verify parameters
			ASSERT(hWnd);
			CAsyncSocketExHelperWindow *pWnd = (CAsyncSocketExHelperWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			ASSERT(pWnd);
			if (!pWnd)
				return 0;

			CAsyncSocketEx *pSocket = NULL;
			for (int i = 0; i < pWnd->m_nWindowDataSize; ++i) {
				pSocket = pWnd->m_pAsyncSocketExWindowData[i].m_pSocket;
				if (pSocket && pSocket->m_hAsyncGetHostByNameHandle &&
					pSocket->m_hAsyncGetHostByNameHandle == (HANDLE)wParam &&
					pSocket->m_pAsyncGetHostByNameBuffer)
					break;
			}
			if (!pSocket || !pSocket->m_pAsyncGetHostByNameBuffer)
				return 0;

			int nErrorCode = lParam >> 16;
			if (nErrorCode) {
				pSocket->OnConnect(nErrorCode);
				return 0;
			}

			SOCKADDR_IN sockAddr{};
			sockAddr.sin_family = AF_INET;
			sockAddr.sin_addr.s_addr = ((LPIN_ADDR)((LPHOSTENT)pSocket->m_pAsyncGetHostByNameBuffer)->h_addr)->s_addr;

			sockAddr.sin_port = htons(pSocket->m_nAsyncGetHostByNamePort);

			BOOL res = pSocket->Connect((SOCKADDR*)&sockAddr, sizeof(sockAddr));
			delete [] pSocket->m_pAsyncGetHostByNameBuffer;
			pSocket->m_pAsyncGetHostByNameBuffer = 0;
			pSocket->m_hAsyncGetHostByNameHandle = 0;

			if (!res)
				if (GetLastError() != WSAEWOULDBLOCK)
					pSocket->OnConnect(GetLastError());
			return 0;
		}
		else if (message == WM_USER + 2)
		{
			//Verify parameters, lookup socket and notification message
			//Verify parameters
			if (!hWnd)
				return 0;

			CAsyncSocketExHelperWindow *pWnd=(CAsyncSocketExHelperWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (!pWnd)
				return 0;

			if (wParam >= static_cast<UINT>(pWnd->m_nWindowDataSize)) //Index is within socket storage
				return 0;

			CAsyncSocketEx *pSocket = pWnd->m_pAsyncSocketExWindowData[wParam].m_pSocket;
			if (!pSocket)
				return 0;

			// Process pending callbacks
			std::list<t_callbackMsg> tmp;
			tmp.swap(pSocket->m_pendingCallbacks);
			pSocket->OnLayerCallback(tmp);

			for (auto & cb : tmp) {
				delete [] cb.str;
			}
		}
		else if (message == WM_TIMER)
		{
			if (wParam != 1)
				return 0;

			ASSERT(hWnd);
			CAsyncSocketExHelperWindow *pWnd=(CAsyncSocketExHelperWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			ASSERT(pWnd && pWnd->m_pThreadData);
			if (!pWnd || !pWnd->m_pThreadData)
				return 0;

			if (pWnd->m_pThreadData->layerCloseNotify.empty())
			{
				KillTimer(hWnd, 1);
				return 0;
			}
			CAsyncSocketEx* socket = pWnd->m_pThreadData->layerCloseNotify.front();
			pWnd->m_pThreadData->layerCloseNotify.pop_front();
			if (pWnd->m_pThreadData->layerCloseNotify.empty())
				KillTimer(hWnd, 1);

			if (socket)
				PostMessage(hWnd, socket->m_SocketData.nSocketIndex + WM_SOCKETEX_NOTIFY, socket->m_SocketData.hSocket, FD_CLOSE);
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	HWND CAsyncSocketExHelperWindow::GetHwnd()
	{
		return m_hWnd;
	}

private:
	HWND m_hWnd;
	struct t_AsyncSocketExWindowData
	{
		CAsyncSocketEx *m_pSocket;
	} *m_pAsyncSocketExWindowData;
	int m_nWindowDataSize;
	int m_nWindowDataPos;
	int m_nSocketCount;
	CAsyncSocketEx::t_AsyncSocketExThreadData* m_pThreadData;
};

//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////

CAsyncSocketEx::~CAsyncSocketEx()
{
	Close();
	FreeAsyncSocketExInstance();
}

bool CAsyncSocketEx::Create(UINT nSocketPort, int nSocketType, long lEvent, LPCTSTR lpszSocketAddress, int nFamily, bool reusable)
{
	ASSERT(GetSocketHandle() == INVALID_SOCKET);

	//Close the socket, although this should not happen
	if (GetSocketHandle() != INVALID_SOCKET) {
		WSASetLastError(WSAEALREADY);
		return false;
	}

	bool res = InitAsyncSocketExInstance();
	ASSERT(res);
	if (!res) {
		WSASetLastError(WSANOTINITIALISED);
		return false;
	}

	m_SocketData.nFamily = nFamily;

	if (m_pFirstLayer) {
		res = m_pFirstLayer->Create(nSocketPort, nSocketType, lEvent, lpszSocketAddress, nFamily, reusable);
#ifndef NOSOCKETSTATES
		if (res) {
			SetState(unconnected);
		}
#endif //NOSOCKETSTATES
		return res;
	}
	else {
		if (m_SocketData.nFamily == AF_UNSPEC) {
#ifndef NOSOCKETSTATES
			SetState(unconnected);
#endif //NOSOCKETSTATES
			m_lEvent = lEvent;

			m_nSocketPort = nSocketPort;

			delete [] m_lpszSocketAddress;
			if (lpszSocketAddress && *lpszSocketAddress) {
				m_lpszSocketAddress = new TCHAR[_tcslen(lpszSocketAddress) + 1];
				_tcscpy(m_lpszSocketAddress, lpszSocketAddress);
			}
			else {
				m_lpszSocketAddress = 0;
			}

			return true;
		}
		else {
			SOCKET hSocket = socket(m_SocketData.nFamily, nSocketType, 0);
			if (hSocket == INVALID_SOCKET) {
				return false;
			}
			AttachHandle(hSocket, m_SocketData.nFamily);

			if (m_pFirstLayer) {
				m_lEvent = lEvent;
				if (WSAAsyncSelect(m_SocketData.hSocket, GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE) ) {
					Close();
					return false;
				}
			}
			else {
				if (!AsyncSelect(lEvent)) {
					Close();
					return false;
				}
			}

			if (reusable && nSocketPort != 0) {
				BOOL value = TRUE;
				SetSockOpt(SO_REUSEADDR, reinterpret_cast<const void*>(&value), sizeof(value));
			}

			if (!Bind(nSocketPort, lpszSocketAddress)) {
				Close();
				return false;
			}

#ifndef NOSOCKETSTATES
			SetState(unconnected);
#endif //NOSOCKETSTATES

			return true;
		}
	}
}

void CAsyncSocketEx::OnReceive(int)
{
}

void CAsyncSocketEx::OnSend(int)
{
}

void CAsyncSocketEx::OnConnect(int)
{
}

void CAsyncSocketEx::OnAccept(int)
{
}

void CAsyncSocketEx::OnClose(int)
{
}

BOOL CAsyncSocketEx::Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress)
{
	delete [] m_lpszSocketAddress;
	if (lpszSocketAddress && *lpszSocketAddress) {
		m_lpszSocketAddress = new TCHAR[_tcslen(lpszSocketAddress) + 1];
		_tcscpy(m_lpszSocketAddress, lpszSocketAddress);
	}
	else {
		m_lpszSocketAddress = 0;
	}
	m_nSocketPort = nSocketPort;

	if (m_SocketData.nFamily == AF_UNSPEC) {
		return TRUE;
	}

	USES_CONVERSION;
	
	LPSTR lpszAscii = (lpszSocketAddress && *lpszSocketAddress) ? T2A((LPTSTR)lpszSocketAddress) : 0;
	
	if ((m_SocketData.nFamily == AF_INET6 || m_SocketData.nFamily == AF_INET) && lpszAscii) {
		addrinfo hints{}, *res0{}, *res{};
		int error{};
		char port[10];
		BOOL ret = FALSE;

		hints.ai_family = m_SocketData.nFamily;
		hints.ai_socktype = SOCK_STREAM;
		_snprintf(port, 9, "%lu", nSocketPort);
		port[9] = 0;
		error = getaddrinfo(lpszAscii, port, &hints, &res0);
		if (error) {
			return FALSE;
		}

		for (res = res0; res; res = res->ai_next) {
			if (Bind(res->ai_addr, res->ai_addrlen)) {
				ret = TRUE;
				break;
			}
		}

		freeaddrinfo(res0);
		return ret;
	}
	else if (!lpszAscii && m_SocketData.nFamily == AF_INET6) {
		SOCKADDR_IN6 sockAddr6{};
		sockAddr6.sin6_family = AF_INET6 ;
		sockAddr6.sin6_addr = in6addr_any ;
		sockAddr6.sin6_port = htons((u_short)nSocketPort);

		return Bind((SOCKADDR*)&sockAddr6, sizeof(sockAddr6));
	}
	else if (!lpszAscii && m_SocketData.nFamily == AF_INET) {
		SOCKADDR_IN sockAddr{};
		sockAddr.sin_family = AF_INET ;
		sockAddr.sin_addr.s_addr = INADDR_ANY ;
		sockAddr.sin_port = htons((u_short)nSocketPort);

		return Bind((SOCKADDR*)&sockAddr, sizeof(sockAddr));
	}
	else {
		return FALSE;
	}
}

BOOL CAsyncSocketEx::Bind(const SOCKADDR* lpSockAddr, int nSockAddrLen)
{
	if (!bind(m_SocketData.hSocket, lpSockAddr, nSockAddrLen))
		return TRUE;
	else
		return FALSE;
}

void CAsyncSocketEx::AttachHandle(SOCKET hSocket, int family)
{
	m_SocketData.hSocket = hSocket;
	m_SocketData.nFamily = family;

	ASSERT(m_pLocalAsyncSocketExThreadData);
	VERIFY(m_pLocalAsyncSocketExThreadData->m_pHelperWindow->AddSocket(this, m_SocketData.nSocketIndex));
#ifndef NOSOCKETSTATES
	SetState(attached);
#endif //NOSOCKETSTATES
}

void CAsyncSocketEx::DetachHandle()
{
	m_SocketData.hSocket = INVALID_SOCKET;

	ASSERT(m_pLocalAsyncSocketExThreadData);
	if (!m_pLocalAsyncSocketExThreadData)
		return;
	ASSERT(m_pLocalAsyncSocketExThreadData->m_pHelperWindow);
	if (!m_pLocalAsyncSocketExThreadData->m_pHelperWindow)
		return;
	VERIFY(m_pLocalAsyncSocketExThreadData->m_pHelperWindow->RemoveSocket(this, m_SocketData.nSocketIndex));
#ifndef NOSOCKETSTATES
	SetState(notsock);
#endif //NOSOCKETSTATES
}

void CAsyncSocketEx::Close()
{
#ifndef NOSOCKETSTATES
	m_nPendingEvents = 0;
#endif //NOSOCKETSTATES
	if (m_pFirstLayer)
		m_pFirstLayer->Close();
	if (m_SocketData.hSocket != INVALID_SOCKET) {
		VERIFY((closesocket(m_SocketData.hSocket) != SOCKET_ERROR));
		DetachHandle();
	}
	if (m_SocketData.addrInfo) {
		freeaddrinfo(m_SocketData.addrInfo);
		m_SocketData.addrInfo = 0;
		m_SocketData.nextAddr = 0;
	}
	m_SocketData.nFamily = AF_UNSPEC;
	delete [] m_lpszSocketAddress;
	m_lpszSocketAddress = 0;
	m_nSocketPort = 0;
	RemoveAllLayers();
	delete [] m_pAsyncGetHostByNameBuffer;
	m_pAsyncGetHostByNameBuffer = NULL;
	if (m_hAsyncGetHostByNameHandle)
		WSACancelAsyncRequest(m_hAsyncGetHostByNameHandle);
	m_hAsyncGetHostByNameHandle = NULL;
	m_SocketData.onCloseCalled = false;
}

bool CAsyncSocketEx::InitAsyncSocketExInstance()
{
	//Check if already initialized
	if (m_pLocalAsyncSocketExThreadData) {
		return true;
	}

	DWORD id = GetCurrentThreadId();

	// Get thread specific data
	if (!thread_local_data) {
		thread_local_data = new t_AsyncSocketExThreadData;
		thread_local_data->m_pHelperWindow = new CAsyncSocketExHelperWindow(thread_local_data);
	}
	m_pLocalAsyncSocketExThreadData = thread_local_data;
	++m_pLocalAsyncSocketExThreadData->nInstanceCount;

	return true;
}

void CAsyncSocketEx::FreeAsyncSocketExInstance()
{
	//Check if already freed
	if (!m_pLocalAsyncSocketExThreadData) {
		return;
	}

	for (std::list<CAsyncSocketEx*>::iterator iter = m_pLocalAsyncSocketExThreadData->layerCloseNotify.begin(); iter != m_pLocalAsyncSocketExThreadData->layerCloseNotify.end(); iter++)
	{
		if (*iter != this)
			continue;

		m_pLocalAsyncSocketExThreadData->layerCloseNotify.erase(iter);
		if (m_pLocalAsyncSocketExThreadData->layerCloseNotify.empty())
			KillTimer(m_pLocalAsyncSocketExThreadData->m_pHelperWindow->GetHwnd(), 1);
		break;
	}

	if (!--m_pLocalAsyncSocketExThreadData->nInstanceCount) {
		m_pLocalAsyncSocketExThreadData = 0;
		delete thread_local_data->m_pHelperWindow;
		delete thread_local_data;
		thread_local_data = 0;
	}
}

int CAsyncSocketEx::Receive(void* lpBuf, int nBufLen, int nFlags /*=0*/)
{
	if (m_pFirstLayer)
		return m_pFirstLayer->Receive(lpBuf, nBufLen, nFlags);
	else
		return recv(m_SocketData.hSocket, (LPSTR)lpBuf, nBufLen, nFlags);
}


int CAsyncSocketEx::Send(const void* lpBuf, int nBufLen, int nFlags /*=0*/)
{
	if (m_pFirstLayer)
		return m_pFirstLayer->Send(lpBuf, nBufLen, nFlags);
	else
		return send(m_SocketData.hSocket, (LPSTR)lpBuf, nBufLen, nFlags);
}

BOOL CAsyncSocketEx::Connect(LPCTSTR lpszHostAddress, UINT nHostPort)
{
	if (m_pFirstLayer) {
		BOOL res = m_pFirstLayer->Connect(lpszHostAddress, nHostPort);
#ifndef NOSOCKETSTATES
		if (res || GetLastError()==WSAEWOULDBLOCK)
			SetState(connecting);
#endif //NOSOCKETSTATES
		return res;
	}
	else if (m_SocketData.nFamily == AF_INET) {
		USES_CONVERSION;

		ASSERT(lpszHostAddress != NULL);

		LPSTR lpszAscii = T2A((LPTSTR)lpszHostAddress);

		SOCKADDR_IN sockAddr{};
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);

		if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
			if (m_pAsyncGetHostByNameBuffer)
				delete [] m_pAsyncGetHostByNameBuffer;
			m_pAsyncGetHostByNameBuffer=new char[MAXGETHOSTSTRUCT];

			m_nAsyncGetHostByNamePort=nHostPort;

			m_hAsyncGetHostByNameHandle=WSAAsyncGetHostByName(GetHelperWindowHandle(), WM_USER+1, lpszAscii, m_pAsyncGetHostByNameBuffer, MAXGETHOSTSTRUCT);
			if (!m_hAsyncGetHostByNameHandle)
				return FALSE;

			WSASetLastError(WSAEWOULDBLOCK);
#ifndef NOSOCKETSTATES
			SetState(connecting);
#endif //NOSOCKETSTATES
			return FALSE;
		}

		sockAddr.sin_port = htons((u_short)nHostPort);

		return CAsyncSocketEx::Connect((SOCKADDR*)&sockAddr, sizeof(sockAddr));
	}
	else {
		USES_CONVERSION;

		ASSERT( lpszHostAddress != NULL );

		if (m_SocketData.addrInfo) {
			freeaddrinfo(m_SocketData.addrInfo);
			m_SocketData.addrInfo = 0;
			m_SocketData.nextAddr = 0;
		}

		int error;
		BOOL ret = TRUE;
		char port[10];

		addrinfo hints{};
		hints.ai_family = m_SocketData.nFamily;
		hints.ai_socktype = SOCK_STREAM;
		_snprintf(port, 9, "%lu", nHostPort);
		port[9] = 0;
		error = getaddrinfo(T2CA(lpszHostAddress), port, &hints, &m_SocketData.addrInfo);
		if (error)
			return FALSE;

		for (m_SocketData.nextAddr = m_SocketData.addrInfo; m_SocketData.nextAddr; m_SocketData.nextAddr = m_SocketData.nextAddr->ai_next) {
			bool newSocket = false;
			if (m_SocketData.nFamily == AF_UNSPEC) {
				newSocket = true;
				SOCKET hSocket = socket(m_SocketData.nextAddr->ai_family, m_SocketData.nextAddr->ai_socktype, m_SocketData.nextAddr->ai_protocol);
				if (hSocket == INVALID_SOCKET)
					continue;

				AttachHandle(hSocket, m_SocketData.nextAddr->ai_family);
				if (!AsyncSelect(m_lEvent)) 	{
					if (newSocket) {
						closesocket(m_SocketData.hSocket);
						DetachHandle();
					}
					continue;
				}
			}
			else if (m_SocketData.hSocket == INVALID_SOCKET)
				continue;

			if (m_pFirstLayer) {
				if (WSAAsyncSelect(m_SocketData.hSocket, GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE)) {
					if (newSocket) {
						m_SocketData.nFamily = AF_UNSPEC;
						closesocket(m_SocketData.hSocket);
						DetachHandle();
					}
					continue;
				}
			}

			if (newSocket) {
				m_SocketData.nFamily = m_SocketData.nextAddr->ai_family;
				if (!Bind(m_nSocketPort, m_lpszSocketAddress)) {
					m_SocketData.nFamily = AF_UNSPEC;
					closesocket(m_SocketData.hSocket);
					DetachHandle();
					continue;
				}
			}

			if (!(ret = CAsyncSocketEx::Connect(m_SocketData.nextAddr->ai_addr, m_SocketData.nextAddr->ai_addrlen)) && GetLastError() != WSAEWOULDBLOCK) {
				if (newSocket) {
					m_SocketData.nFamily = AF_UNSPEC;
					closesocket(m_SocketData.hSocket);
					DetachHandle();
				}
				continue;
			}

			break;
		}

		if (m_SocketData.nextAddr)
			m_SocketData.nextAddr = m_SocketData.nextAddr->ai_next;

		if (!m_SocketData.nextAddr) {
			freeaddrinfo(m_SocketData.addrInfo);
			m_SocketData.nextAddr = 0;
			m_SocketData.addrInfo = 0;
		}

		if (m_SocketData.hSocket == INVALID_SOCKET || !ret)
			return FALSE;
		else
			return TRUE;
	}
}

BOOL CAsyncSocketEx::Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen)
{
	BOOL res;
	if (m_pFirstLayer)
		res = SOCKET_ERROR!=m_pFirstLayer->Connect(lpSockAddr, nSockAddrLen);
	else
		res = SOCKET_ERROR!=connect(m_SocketData.hSocket, lpSockAddr, nSockAddrLen);

#ifndef NOSOCKETSTATES
	if (res || GetLastError()==WSAEWOULDBLOCK)
		SetState(connecting);
#endif //NOSOCKETSTATES
	return res;
}

BOOL CAsyncSocketEx::GetPeerName(CStdString& rPeerAddress, UINT& rPeerPort)
{
	sockaddr_storage sockAddr{ 0 };
	BOOL bResult = GetPeerName(sockAddr);

	if (bResult) {
		if (sockAddr.ss_family == AF_INET6) {
			sockaddr_in6 &in = reinterpret_cast<sockaddr_in6&>(sockAddr);
			rPeerPort = ntohs(in.sin6_port);
			rPeerAddress = Inet6AddrToString(in.sin6_addr);
		}
		else if (m_SocketData.nFamily == AF_INET) {
			sockaddr_in &in = reinterpret_cast<sockaddr_in&>(sockAddr);
			rPeerPort = ntohs(in.sin_port);
			rPeerAddress = inet_ntoa(in.sin_addr);
		}
		else {
			WSASetLastError(WSAEPFNOSUPPORT);
			return FALSE;
		}
	}

	return bResult;
}

BOOL CAsyncSocketEx::GetPeerName(sockaddr_storage & sockAddr)
{
	int len = sizeof(sockAddr);
	if (!getpeername(m_SocketData.hSocket, reinterpret_cast<sockaddr*>(&sockAddr), &len))
		return TRUE;
	else
		return FALSE;
}

BOOL CAsyncSocketEx::GetSockName(CStdString& rSocketAddress, UINT& rSocketPort)
{
	sockaddr_storage sockAddr{0};
	BOOL bResult = GetSockName(sockAddr);

	if (bResult) {
		if (sockAddr.ss_family == AF_INET6) {
			sockaddr_in6 &in = reinterpret_cast<sockaddr_in6&>(sockAddr);
			rSocketPort = ntohs(in.sin6_port);
			rSocketAddress = Inet6AddrToString(in.sin6_addr);
		}
		else if (m_SocketData.nFamily == AF_INET) {
			sockaddr_in &in = reinterpret_cast<sockaddr_in&>(sockAddr);
			rSocketPort = ntohs(in.sin_port);
			rSocketAddress = inet_ntoa(in.sin_addr);
		}
		else {
			WSASetLastError(WSAEPFNOSUPPORT);
			return FALSE;
		}
	}

	return bResult;
}

BOOL CAsyncSocketEx::GetSockName(sockaddr_storage& sockAddr)
{
	int len = sizeof(sockAddr);
	if (!getsockname(m_SocketData.hSocket, reinterpret_cast<sockaddr*>(&sockAddr), &len))
		return TRUE;
	else
		return FALSE;
}

BOOL CAsyncSocketEx::ShutDown()
{
	if (m_pFirstLayer) {
		return m_pFirstLayer->ShutDown();
	}
	else {
		if (!shutdown(m_SocketData.hSocket, SD_SEND))
			return TRUE;
		else
			return FALSE;
	}
}

SOCKET CAsyncSocketEx::Detach()
{
	SOCKET socket = m_SocketData.hSocket;
	DetachHandle();
	m_SocketData.nFamily = AF_UNSPEC;
	return socket;
}

BOOL CAsyncSocketEx::Attach(SOCKET hSocket, long lEvent /*= FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE*/)
{
	if (hSocket == INVALID_SOCKET || !hSocket)
		return FALSE;

	VERIFY(InitAsyncSocketExInstance());

	sockaddr_storage addr{};
	int len = sizeof(addr);
	if (getsockname(hSocket, reinterpret_cast<sockaddr*>(&addr), &len) != 0)
		return FALSE;

	AttachHandle(hSocket, addr.ss_family);

	if (m_pFirstLayer) {
		m_lEvent = lEvent;
		return !WSAAsyncSelect(m_SocketData.hSocket, GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
	}
	else {
		return AsyncSelect(lEvent);
	}
}

BOOL CAsyncSocketEx::AsyncSelect( long lEvent /*= FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE*/ )
{
	ASSERT(m_pLocalAsyncSocketExThreadData);
	m_lEvent = lEvent;
	if (m_pFirstLayer)
		return TRUE;
	else {
		if (m_SocketData.hSocket == INVALID_SOCKET && m_SocketData.nFamily == AF_UNSPEC)
			return true;

		if ( !WSAAsyncSelect(m_SocketData.hSocket, GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, lEvent) )
			return TRUE;
		else
			return FALSE;
	}
	return TRUE;
}

BOOL CAsyncSocketEx::Listen( int nConnectionBacklog /*=5*/ )
{
	if (m_pFirstLayer)
		return m_pFirstLayer->Listen(nConnectionBacklog);

	if (!listen(m_SocketData.hSocket, nConnectionBacklog)) {
#ifndef NOSOCKETSTATES
		SetState(listening);
#endif //NOSOCKETSTATES
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CAsyncSocketEx::Accept( CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr /*=NULL*/, int* lpSockAddrLen /*=NULL*/ )
{
	ASSERT(rConnectedSocket.m_SocketData.hSocket == INVALID_SOCKET);
	if (m_pFirstLayer) {
		return m_pFirstLayer->Accept(rConnectedSocket, lpSockAddr, lpSockAddrLen);
	}
	else {
		SOCKET hTemp = accept(m_SocketData.hSocket, lpSockAddr, lpSockAddrLen);

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

BOOL CAsyncSocketEx::IOCtl( long lCommand, DWORD* lpArgument )
{
	return ioctlsocket(m_SocketData.hSocket, lCommand, lpArgument) != SOCKET_ERROR;
}

int CAsyncSocketEx::GetLastError()
{
	return WSAGetLastError();
}

BOOL CAsyncSocketEx::TriggerEvent(long lEvent)
{
	if (m_SocketData.hSocket == INVALID_SOCKET)
		return FALSE;

	ASSERT(m_pLocalAsyncSocketExThreadData);
	ASSERT(m_pLocalAsyncSocketExThreadData->m_pHelperWindow);
	ASSERT(m_SocketData.nSocketIndex!=-1);

	if (m_pFirstLayer) {
		CAsyncSocketExLayer::t_LayerNotifyMsg *pMsg = new CAsyncSocketExLayer::t_LayerNotifyMsg;
		pMsg->hSocket = m_SocketData.hSocket;
		pMsg->lEvent= lEvent & 0xFFFF;
		pMsg->pLayer=0;
		BOOL res=PostMessage(GetHelperWindowHandle(), WM_USER, (WPARAM)m_SocketData.nSocketIndex, (LPARAM)pMsg);
		if (!res)
			delete pMsg;
		return res;
	}
	else
		return PostMessage(GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, m_SocketData.hSocket, lEvent & 0xFFFF);
}

SOCKET CAsyncSocketEx::GetSocketHandle()
{
	return m_SocketData.hSocket;
}

HWND CAsyncSocketEx::GetHelperWindowHandle()
{
	if (!m_pLocalAsyncSocketExThreadData)
		return 0;
	if (!m_pLocalAsyncSocketExThreadData->m_pHelperWindow)
		return 0;
	return m_pLocalAsyncSocketExThreadData->m_pHelperWindow->GetHwnd();
}

BOOL CAsyncSocketEx::AddLayer(CAsyncSocketExLayer *pLayer)
{
	ASSERT(pLayer);

	pLayer->m_nFamily = m_SocketData.nFamily;

	if (m_pFirstLayer)
	{
		ASSERT(m_pLastLayer);
		m_pLastLayer=m_pLastLayer->AddLayer(pLayer, this);
		return m_pLastLayer?TRUE:FALSE;
	}
	else
	{
		ASSERT(!m_pLastLayer);
		pLayer->Init(0, this);
		m_pFirstLayer=pLayer;
		m_pLastLayer=m_pFirstLayer;
		if (m_SocketData.hSocket != INVALID_SOCKET)
			if (WSAAsyncSelect(m_SocketData.hSocket, GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE))
				return FALSE;
	}

	return TRUE;
}

void CAsyncSocketEx::RemoveAllLayers()
{
	for (std::list<t_callbackMsg>::iterator iter = m_pendingCallbacks.begin(); iter != m_pendingCallbacks.end(); iter++)
		delete [] iter->str;
	m_pendingCallbacks.clear();

	m_pFirstLayer = 0;
	m_pLastLayer = 0;

	if (!m_pLocalAsyncSocketExThreadData)
		return;
	if (!m_pLocalAsyncSocketExThreadData->m_pHelperWindow)
		return;
	m_pLocalAsyncSocketExThreadData->m_pHelperWindow->RemoveLayers(this);
}

int CAsyncSocketEx::OnLayerCallback(std::list<t_callbackMsg> const& callbacks)
{
	return 0;
}

BOOL CAsyncSocketEx::IsLayerAttached() const
{
	return m_pFirstLayer ? TRUE : FALSE;
}

BOOL CAsyncSocketEx::GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel /*=SOL_SOCKET*/)
{
	return (SOCKET_ERROR != getsockopt(m_SocketData.hSocket, nLevel, nOptionName, (LPSTR)lpOptionValue, lpOptionLen));
}

BOOL CAsyncSocketEx::SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel /*=SOL_SOCKET*/)
{
	return (SOCKET_ERROR != setsockopt(m_SocketData.hSocket, nLevel, nOptionName, (LPSTR)lpOptionValue, nOptionLen));
}

#ifndef NOSOCKETSTATES

int CAsyncSocketEx::GetState() const
{
	return m_nState;
}

void CAsyncSocketEx::SetState(int nState)
{
	m_nState = nState;
}

#endif //NOSOCKETSTATES

int CAsyncSocketEx::GetFamily() const
{
	return m_SocketData.nFamily;
}

bool CAsyncSocketEx::SetFamily(int nFamily)
{
	if (m_SocketData.nFamily != AF_UNSPEC)
		return false;

	m_SocketData.nFamily = nFamily;
	return true;
}

bool CAsyncSocketEx::TryNextProtocol()
{
	closesocket(m_SocketData.hSocket);
	DetachHandle();

	BOOL ret = FALSE;
	for (; m_SocketData.nextAddr; m_SocketData.nextAddr = m_SocketData.nextAddr->ai_next) {
		SOCKET hSocket = socket(m_SocketData.nextAddr->ai_family, m_SocketData.nextAddr->ai_socktype, m_SocketData.nextAddr->ai_protocol);
		if (hSocket == INVALID_SOCKET)
			continue;

		AttachHandle(hSocket, m_SocketData.nextAddr->ai_family);
		if (!AsyncSelect(m_lEvent)) {
			closesocket(m_SocketData.hSocket);
			DetachHandle();
			continue;
		}

		if (m_pFirstLayer) {
			if (WSAAsyncSelect(m_SocketData.hSocket, GetHelperWindowHandle(), m_SocketData.nSocketIndex+WM_SOCKETEX_NOTIFY, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE)) {
				closesocket(m_SocketData.hSocket);
				DetachHandle();
				continue;
			}
		}

		if (!Bind(m_nSocketPort, m_lpszSocketAddress)) {
			closesocket(m_SocketData.hSocket);
			DetachHandle();
			continue;
		}

		ret = CAsyncSocketEx::Connect(m_SocketData.nextAddr->ai_addr, m_SocketData.nextAddr->ai_addrlen);
		if (!ret && GetLastError() != WSAEWOULDBLOCK) {
			closesocket(m_SocketData.hSocket);
			DetachHandle();
			continue;
		}

		ret = true;
		break;
	}

	if (m_SocketData.nextAddr)
		m_SocketData.nextAddr = m_SocketData.nextAddr->ai_next;

	if (!m_SocketData.nextAddr) {
		freeaddrinfo(m_SocketData.addrInfo);
		m_SocketData.nextAddr = 0;
		m_SocketData.addrInfo = 0;
	}

	if (m_SocketData.hSocket == INVALID_SOCKET || !ret)
		return FALSE;
	else
		return TRUE;
}

void CAsyncSocketEx::AddCallbackNotification(const t_callbackMsg& msg)
{
	m_pendingCallbacks.push_back(msg);

	if(m_pendingCallbacks.size() == 1 && m_SocketData.nSocketIndex != -1)
		PostMessage(GetHelperWindowHandle(), WM_USER + 2, (WPARAM)m_SocketData.nSocketIndex, 0);
}

void CAsyncSocketEx::ResendCloseNotify()
{
	for (std::list<CAsyncSocketEx*>::iterator iter = m_pLocalAsyncSocketExThreadData->layerCloseNotify.begin(); iter != m_pLocalAsyncSocketExThreadData->layerCloseNotify.end(); iter++)
	{
		if (*iter == this)
			return;
	}
	m_pLocalAsyncSocketExThreadData->layerCloseNotify.push_back(this);
	if (m_pLocalAsyncSocketExThreadData->layerCloseNotify.size() == 1)
	{
		SetTimer(m_pLocalAsyncSocketExThreadData->m_pHelperWindow->GetHwnd(), 1, 10, 0);
	}
}

bool CAsyncSocketEx::SetNodelay(bool nodelay)
{
	BOOL value = nodelay ? TRUE : FALSE;
	return SetSockOpt(TCP_NODELAY, &value, sizeof(value), IPPROTO_TCP) == TRUE;
}



CStdString Inet6AddrToString(in6_addr& addr)
{
	TCHAR buf[512];

	_sntprintf(buf, 512, _T("%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x"),
		addr.s6_bytes[0], addr.s6_bytes[1], addr.s6_bytes[2], addr.s6_bytes[3],
		addr.s6_bytes[4], addr.s6_bytes[5], addr.s6_bytes[6], addr.s6_bytes[7],
		addr.s6_bytes[8], addr.s6_bytes[9], addr.s6_bytes[10], addr.s6_bytes[11],
		addr.s6_bytes[12], addr.s6_bytes[13], addr.s6_bytes[14], addr.s6_bytes[15]);

	return buf;
}
