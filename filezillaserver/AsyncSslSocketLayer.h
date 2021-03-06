/*           CAsyncSslSocketLayer by Tim Kosse 
          mailto: tim.kosse@filezilla-project.org)
                 Version 2.0 (2005-02-27)
-------------------------------------------------------------

Introduction
------------

CAsyncSslSocketLayer is a layer class for CAsyncSocketEx which allows you to establish SSL secured
connections. Support for both client and server side is provided.

How to use
----------

Using this class is really simple. In the easiest case, just add an instance of
CAsyncSslSocketLayer to your socket and call InitClientSsl after creation of the socket.

This class only has a couple of public functions:
- int InitSSLConnection(bool clientMode);
  This functions establishes an SSL connection. The clientMode parameter specifies wether the SSL connection
  is in server or in client mode.
  Most likely you want to call this function right after calling Create for the socket.
  But sometimes, you'll need to call this function later. One example is for an FTP connection
  with explicit SSL: In this case you would have to call InitSSLConnection after receiving the reply
  to an 'AUTH SSL' command.
  InitSSLConnection returns 0 on success, else an error code as described below under SSL_FAILURE
- Is UsingSSL();
  Returns true if you've previously called InitClientSsl()
- SetNotifyReply(SetNotifyReply(int nID, int nCode, int result);
  You can call this function only after receiving a layerspecific callback with the SSL_VERIFY_CERT
  id. Set result to 1 if you trust the certificate and 0 if you don't trust it.
  nID has to be the priv_data element of the t_SslCertData structure and nCode has to be SSL_VERIFY_CERT.
- CreateSslCertificate(LPCTSTR filename, int bits, unsigned char* country, unsigned char* state,
			unsigned char* locality, unsigned char* organization, unsigned char* unit, unsigned char* cname,
			unsigned char *email, CString& err);
  Creates a new self-signed SSL certificate and stores it in the given file
- SendRaw(const void* lpBuf, int nBufLen)
  Sends a raw, unencrypted message. This may be useful after successful initialization to tell the other
  side that can use SSL.

This layer sends some layerspecific notifications to your socket instance, you can handle them in
OnLayerCallback of your socket class.
Valid notification IDs are:
- SSL_INFO 0
  There are two possible values for param2:
	SSL_INFO_ESTABLISHED 0 - You'll get this notification if the SSL negotiation was successful
	SSL_INFO_SHUTDOWNCOMPLETE 1 - You'll get this notification if the SSL connection has been shut
                                  down successfully. See below for details.
- SSL_FAILURE 1
  This notification is sent if the SSL connection could not be established or if an existing
  connection failed. Valid values for param2 are:
  - SSL_FAILURE_NONE 0 - Everything OK
  - SSL_FAILURE_UNKNOWN 1 - Details may have been sent with a SSL_VERBOSE_* notification.
  - SSL_FAILURE_ESTABLISH 2 - Problem during SSL negotiation
  - SSL_FAILURE_LOADDLLS 4
  - SSL_FAILURE_INITSSL 8
  - SSL_FAILURE_VERIFYCERT 16 - The remote SSL certificate was invalid
  - SSL_FAILURE_CERTREJECTED 32 - The remote SSL certificate was rejected by user
- SSL_VERBOSE_WARNING 3
  SSL_VERBOSE_INFO 4
  This two notifications contain some additional information. The value given by param2 is a
  pointer to a null-terminated char string (char *) with some useful information.
- SSL_VERIFY_CERT 2
  This notification is sent each time a remote certificate has to be verified.
  param2 is a pointer to a t_SslCertData structure which contains some information
  about the remote certificate.
  You have to set the reply to this message using the SetNotifyReply function.

Be careful with closing the connection after sending data, not all data may have been sent already.
Before closing the connection, you should call Shutdown() and wait for the SSL_INFO_SHUTDOWNCOMPLETE
notification. This assures that all encrypted data really has been sent.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you want to use this class in a commercial application, a short message
to tim.kosse@filezilla-project.org would be appreciated but is not required.

This product includes software developed by the OpenSSL Project
for use in the OpenSSL Toolkit. (http://www.openssl.org/)

Version history
---------------

Version 2.0:
- Add server support
- a lot of bug fixes

*/

#ifndef ASYNCSSLSOCKETLEAYER_INCLUDED
#define ASYNCSSLSOCKETLEAYER_INCLUDED

#ifndef _AFX
#define CString CStdString
#endif

#include "AsyncSocketExLayer.h"
#include <openssl/ssl.h>
#include "misc/dll.h"

#include <memory>
#include <mutex>

// Details of SSL certificate, can be used by app to verify if certificate is valid
struct t_SslCertData final
{
	struct t_Contact
	{
		TCHAR Organization[256];
		TCHAR Unit[256];
		TCHAR CommonName[256];
		TCHAR Mail[256];
		TCHAR Country[256];
		TCHAR StateProvince[256];
		TCHAR Town[256];
		TCHAR Other[1024];
	} subject, issuer;

	tm validFrom{};
	tm validUntil{};

	unsigned char hash[20];

	int verificationResult;
	int verificationDepth;

	int priv_data; //Internal data, do not modify
};

enum class ShutDownState
{
	none,
	shuttingDown,
	shutDown
};

class CAsyncSslSocketLayer final : public CAsyncSocketExLayer
{
public:
	BOOL SetCertStorage(CString const& file);
	CAsyncSslSocketLayer(int minTlsVersion);
	virtual ~CAsyncSslSocketLayer();

	void SetNotifyReply(int nID, int nCode, int result);
	BOOL GetPeerCertificateData(t_SslCertData &SslCertData);

	bool IsUsingSSL();
	int InitSSLConnection(bool clientMode, CAsyncSslSocketLayer* primarySocket = 0, bool require_session_reuse = false);

	static bool CreateSslCertificate(LPCTSTR filename, int bits, const unsigned char* country, const unsigned char* state,
			const unsigned char* locality, const unsigned char* organization, const unsigned char* unit, const unsigned char* cname,
			const unsigned char *email, CString& err);

	int SetCertKeyFile(CString const& cert, CString const& key, CString const& pass, CString* error = 0, bool checkExpired = false);

	// Send raw text, useful to send a confirmation after the ssl connection
	// has been initialized
	int SendRaw(const void* lpBuf, int nBufLen);

	CStdString GenerateDiffieHellmanParameters();

	// If needed, loads the library and does global initialization.
	// Return 0 on success
	int InitSSL();

	std::string SHA512(unsigned char const* buf, size_t len);

private:
	virtual void Close();
	virtual BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort );
	virtual BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen );
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	virtual BOOL ShutDown();
	BOOL DoShutDown();

	bool CreateContext();
	void ResetSslSession();
	void PrintSessionInfo();
	void UnloadSSL();
	void DoUnloadLibrary();
	int GetLastSslError(CStdString& e);
	bool PrintLastErrorMsg();
	void ClearErrors();

	void TriggerEvents();

	int LoadCertKeyFile(const char* cert, const char* key, CString* error, bool checkExpired);

	bool SetDiffieHellmanParameters(CStdString const& params);

	// Will be called from the OpenSSL library
	static void apps_ssl_info_callback(const SSL *s, int where, int ret);
	static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx);
	static int pem_passwd_cb(char *buf, int size, int rwflag, void *userdata);

	bool m_bUseSSL{};
	BOOL m_bFailureSent{};

	//Mutex for thread synchronization
	static std::recursive_mutex m_mutex;

	// Status variables
	static int m_nSslRefCount;
	bool m_bSslInitialized{};
	DWORD m_nNetworkError{};
	int m_nSslAsyncNotifyId{};
	BOOL m_bBlocking{};
	bool m_bSslEstablished{};
	CString m_CertStorage;
	int m_nVerificationResult{};
	int m_nVerificationDepth{};

	// Handles to the SLL libraries
	static DLL m_sslDll1;
	static DLL m_sslDll2;

	static DH* m_dh;

	// SSL data
	std::shared_ptr<SSL_CTX> m_ssl_ctx{}; // SSL context

	SSL* m_ssl{};			// current session handle

	// Data channels for encrypted/unencrypted data
	BIO* m_nbio{};   //Network side, sends/received encrypted data
	BIO* m_ibio{};   //Internal side, won't be used directly
	BIO* m_sslbio{}; //The data to encrypt / the decrypted data has to go though this bio

	//Send buffer
	char* m_pNetworkSendBuffer{};
	int m_nNetworkSendBufferLen{};
	int m_nNetworkSendBufferMaxLen{};

	char* m_pRetrySendBuffer{};
	int m_nRetrySendBufferLen{};

	bool m_mayTriggerRead{true};
	bool m_mayTriggerWrite{true};
	bool m_mayTriggerReadUp{true};
	bool m_mayTriggerWriteUp{true};

	bool m_onCloseCalled{};

	char* m_pKeyPassword{};

	bool m_require_session_reuse{};
	CAsyncSslSocketLayer* m_primarySocket{};

	ShutDownState shutDownState = ShutDownState::none;

	int m_minTlsVersion;
};

#define SSL_INFO 0
#define SSL_FAILURE 1
#define SSL_VERIFY_CERT 2
#define SSL_VERBOSE_WARNING 3
//#define SSL_VERBOSE_INFO 4

#define SSL_INFO_ESTABLISHED 0
#define SSL_INFO_SHUTDOWNCOMPLETE 1

#define SSL_FAILURE_UNKNOWN 0
#define SSL_FAILURE_ESTABLISH 1
#define SSL_FAILURE_LOADDLLS 2
#define SSL_FAILURE_INITSSL 4
#define SSL_FAILURE_VERIFYCERT 8
#define SSL_FAILURE_CERTREJECTED 0x10
#define SSL_FAILURE_NO_SESSIONREUSE 0x20

#endif // ASYNCSSLSOCKETLEAYER_INCLUDED
