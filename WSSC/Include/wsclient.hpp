#ifndef WSCLIENT_HPP
#define WSCLIENT_HPP

#include "const.hpp"

#include <jsonrpcpp.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <mutex>

using namespace std;
using namespace jsonrpcpp;

typedef websocketpp::client<websocketpp::config::asio_client> Client;
typedef websocketpp::connection_hdl                           Handle;
typedef websocketpp::config::asio_client::message_type::ptr   MsgPtr;
typedef websocketpp::lib::error_code                          ErrCode;

typedef void (*MsgCallback) (Long pId, Json & pResp, VPtr pObj);
typedef void (*ErrCallback) (Int pErrId, CString & pMsg, VPtr pObj);

class WSClient {

public:

                WSClient    ();
                ~WSClient   ();

        bool    Connect             (CStrPtr pURL);
inline  bool    IsConnected ();

        bool    Send                (String & pEndpoint, Long pId, Json & pParam);
        bool    Send                (String & pEndpoint, Long pId, Json & pParam, Json & pResp);

inline  void    OnMessage           (MsgCallback pCB, VPtr pObj);
inline  void    OnError             (ErrCallback pCB, VPtr pObj);


private:

        void    OnOpen    (Handle pHdl);
        void    OnClose   (Handle pHdl);
        void    OnMsgRecv (Handle pHdl, MsgPtr pMsg);

        void    Run         ();
        void    Stop        ();

inline  void    WaitForConnection   ();

        bool         vIsBusy;
        bool         vConnected;
        Handle       vHandle;
        MsgCallback  vMsgCb;
        VPtr         vMsgParam;
        ErrCallback  vErrCb;
        VPtr         vErrParam;
        Json *       vResponses;
        mutex        vLock;
        Client       vSocket;
};

#include "wsclient.hxx"

#endif // !WSCLIENT_HPP