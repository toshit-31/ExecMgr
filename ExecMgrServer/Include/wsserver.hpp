#ifndef WSSERVER_HPP
#define WSSERVER_HPP

#include <thread>
#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "wssclient.hpp"

#define PORT        8001
#define MAP_CONTAINS(m,k) (m->find (k) != m->end ())

using namespace std;
using namespace websocketpp;

typedef websocketpp::server<websocketpp::config::asio>  Server;
typedef websocketpp::connection_hdl                     Handle;
typedef Server::message_ptr                             SVMsgPtr; 

typedef void (*OrderBookNotifCallback) (Json & pAsks, Json & pBids);

typedef set<Handle, owner_less<Handle>> SetHandle;

struct stServerOrderBook {
    UInt        uSubCount;  // maintain the references like shared_ptr
                            // will be used for cleanup when value becomes 0
    Json        uAsks;
    Json        uBids;
    thread      uThread;
    SetHandle * uConnList;

    inline ~stServerOrderBook ()
    {
        // clear the connection list
        uConnList->clear ();
        delete uConnList;

        // join the thread before destructing
        uThread.join ();
    }
};

typedef map<String, stServerOrderBook*> MapOrderBook;

/*
* Implement
* SendError
* Change OrderMgr to have two clients
* Add a OnMessage,OnError Callback for vClient probably copy from OrderMgr
*/
class WSServer {

public:

                WSServer ();
                ~WSServer ();

        void    Listen ();

private:

        enum class error {
            NO_METHOD = 1001,
            INVALID_PARAMS,
            SERVER_CALL_FAILED
        };

        void    SendError       (Handle pHdl, WSServer::error pErr);
        bool    SendResponse    (Handle pHdl, Int pId);

        void    Subscribe       (Handle pHdl, CString & pInstrName);
        void    Unsubscribe     (Handle pHdl, CString & pInstrName, bool pTakeLock = true);

        bool    SubscribeTo     (CString & pInstrName);
        bool    UnsubscribeFrom (CString & pInstrName);

        /**
            broadcasting is done on the basis of instrument names
            for instrument name we maintain map of clients subscribed to it
            each instrument has it's own thread and this thread is responsible
            for sending the order book updates to each client in a synchronous fashion
        */ 
        void    BroadcastToHandles      (CString pInstrName);

        void    StartListening  ();

        void    OnMsgRecv       (Handle pHdl, SVMsgPtr pMsg);

static  void    OnAPIMessage    (Long pId, Json & pResp, VPtr pObj);
static  void    OnAPIError      (Int pErrId, CString & pMsg, VPtr pObj);

        bool        vBroadcastingState;
        thread      vListeningThread;

        MapOrderBook * vMapOrderBook;
        mutex          vSubEditLock;
        
        Server      vServer;
        WSSClient   vClient;



    friend void OnAPIMessage (Long pId, Json & pResp, VPtr pObj);
};

#endif // WSSERVER_HPP