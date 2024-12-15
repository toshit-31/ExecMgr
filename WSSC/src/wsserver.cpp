#include "wsserver.hpp"

WSServer::WSServer ()
{
    vServer.init_asio ();

    vServer.clear_access_channels (websocketpp::log::alevel::all);
    vServer.clear_error_channels (websocketpp::log::elevel::all); 

    vServer.set_message_handler (bind(&WSServer::OnMsgRecv, this, std::placeholders::_1, std::placeholders::_2));

    vClient.OnMessage (&WSServer::OnAPIMessage, this);
    vClient.OnError   (&WSServer::OnAPIError, this);

    vClient.Connect (DB_BASE_URL);

    vBroadcastingState = true;
    vMapOrderBook      = new MapOrderBook;
}

WSServer::~WSServer ()
{
    vBroadcastingState = false;

    vServer.stop_listening ();
    vServer.stop ();

    delete vMapOrderBook;
}

void WSServer::Listen ()
{
    vListeningThread = thread (&WSServer::StartListening, this);
}

void WSServer::SendError (Handle pHdl, WSServer::error pErr)
{
        Json err;

    err["error"]["code"] = (Int) pErr;
    
    switch (pErr) {
        case error::NO_METHOD:
            err["error"]["message"] = "NO METHOD";
            break;

        case error::INVALID_PARAMS:
            err["error"]["message"] = "INVALID PARAMS";
            break;

        case error::SERVER_CALL_FAILED:
            err["error"]["message"] = "SERVER CALL FAILED";
            break;
    }
}

bool WSServer::SendResponse (Handle pHdl, Int pId)
{
        Json    resp;
        ErrCode ec;

    resp["id"] = pId;
    resp["result"]["success"] = true;
    
    vServer.send (pHdl, resp.dump (), frame::opcode::text, ec);

    return !ec;
}

void WSServer::Subscribe (Handle pHdl, CString & pInstrName)
{
            stServerOrderBook * order_book;

    while (!vSubEditLock.try_lock ());

    // update the mapping INSTRUMENT_NAME : CONNECTION
    //                    INSTRUMENT_NAME : ORDER_BOOK
    if (MAP_CONTAINS (vMapOrderBook, pInstrName)) {

        order_book = (*vMapOrderBook)[pInstrName];

        // check if the connection is subscribed to it
        // if already scubcribed do nothing
        if (order_book->uConnList->find (pHdl) != order_book->uConnList->end ()) {
            goto unlock_sub;

        }

        order_book->uConnList->insert (pHdl);
        order_book->uSubCount++;

    } else {
        order_book = new stServerOrderBook; 
        order_book->uSubCount = 1;
        order_book->uConnList = new SetHandle({pHdl});

        (*vMapOrderBook)[pInstrName] = order_book;

        // run the broadcaster in a separate thread
        // save the thread in the orderbook mapping against the instrument name
        order_book->uThread = thread (&WSServer::BroadcastToHandles, this, pInstrName);
    };

unlock_sub:
    vSubEditLock.unlock ();
}

void WSServer::Unsubscribe (Handle pHdl, CString & pInstrName, bool pTakeLock)
{
    while (pTakeLock && !vSubEditLock.try_lock ());

    if (!MAP_CONTAINS (vMapOrderBook, pInstrName)) {
        if (pTakeLock) vSubEditLock.unlock ();
        return;

    }

        stServerOrderBook * order_book = (*vMapOrderBook)[pInstrName];

    // either the order book doesnt exists that is on one subscribed to it
    // or the connection isn't subscribed to it
    if (!order_book || order_book->uConnList->find (pHdl) == order_book->uConnList->end ()) {
        if (pTakeLock) vSubEditLock.unlock ();
        return;

    }

    order_book->uConnList->erase (pHdl);
    order_book->uSubCount--;

    if (order_book->uSubCount == 0) {
        
        // if the order book is not used by anyone
        // unsubscribe it from the server itself
        UnsubscribeFrom (pInstrName);

        // remove from order book mapping
        vMapOrderBook->erase (pInstrName);
    }

    if (pTakeLock) vSubEditLock.unlock ();
    
    // delete the order book
    delete order_book;
}

bool WSServer::SubscribeTo (CString & pInstrName)
{
        Json       params;  // request params
        String     ep = EP_SUBSCRIBE;
        Long       id = 10;
        String     ch;

    ch = "book." + pInstrName + ".none.1.100ms";
    params["channels"][0] = ch;

    // non blocking send
    return vClient.Send (ep, id, params);
}

bool WSServer::UnsubscribeFrom (CString & pInstrName)
{
        Json       params;  // request params
        String     ep = EP_UNSUBSCRIBE;
        Long       id = 11;
        String     ch;

    ch = "book." + pInstrName + ".none.1.100ms";
    params["channels"][0] = ch;

    // non blocking send
    return vClient.Send (ep, id, params);
}

// TODO
// Proper error handling and termination if connection closed
void WSServer::BroadcastToHandles (CString pInstrName)
{
        ErrCode ec;
        Json    payload;
        SetHandle to_remove;

    payload["params"]["data"]["instrument_name"] = pInstrName;

    while (true) {

        while (!vSubEditLock.try_lock ());

        // instrument name doesn't exists
        if (!MAP_CONTAINS (vMapOrderBook, pInstrName)) {

            vSubEditLock.unlock ();
            break;
        }

            stServerOrderBook * order_book = (*vMapOrderBook)[pInstrName];
            SetHandle::iterator it = order_book->uConnList->begin ();

        while (it != order_book->uConnList->end ()) {
            payload["params"]["data"]["asks"] = order_book->uAsks;
            payload["params"]["data"]["bids"] = order_book->uBids;

            cout << payload.dump () << endl;

            vServer.send (*it, payload.dump (), frame::opcode::text, ec);

            // if for some reason the send function returns an error
            // clear the handle from handle set/list
            if (ec) {
                    Handle h = *it;
                to_remove.insert (h);
            }

            it++;
        }

        it = to_remove.begin ();
        while (it != to_remove.end ()) {
            order_book->uSubCount--;
            order_book->uConnList->erase (*it);

            if (order_book->uSubCount == 0) {
                vMapOrderBook->erase (pInstrName);
            }

            it++;
        }
        to_remove.clear ();

        vSubEditLock.unlock ();
        
        if (!order_book || order_book->uSubCount == 0) {
            break;
        }
        
        // TODO
        // make this thread pause a defined time
        this_thread::sleep_for (std::chrono::milliseconds (200));
    }
}

void WSServer::StartListening ()
{
    vServer.listen (PORT);
    vServer.start_accept ();
    vServer.run ();
}

void WSServer::OnAPIMessage (Long pId, Json & pResp, VPtr pObj)
{
        WSServer * server = (WSServer *) pObj;
        Json temp;

    switch (pId) {

        // used for notifications
        case 0: 
        {
                stServerOrderBook * st;
            
            temp = pResp["params"]["data"];

            if (!MAP_CONTAINS (server->vMapOrderBook, temp["instrument_name"])) {
                break;
            }

            st = (* server->vMapOrderBook)[temp["instrument_name"]];

            st->uAsks = move (temp["asks"]);
            st->uBids = move (temp["bids"]);

            break;
        }

        default:
        {
            cout << pResp.dump (4) << endl;
        }
    }
}

void WSServer::OnAPIError (Int pErrId, CString & pMsg, VPtr pObj)
{
    printf ("WSServer::Error %d : %s\n", pErrId, pMsg.c_str ());
}

void WSServer::OnMsgRecv (Handle pHdl, SVMsgPtr pMsg)
{
        Json   req = Json::parse (pMsg->get_payload ());
        String method;
        String ch;

    if (!req.contains ("method")) {
        SendError (pHdl, error::NO_METHOD);
        return;

    }
        
    method = req["method"];

    if (method == EP_SUBSCRIBE) {

        if (req["params"].is_null () || req["params"]["instrument_name"].is_null ()) {
            SendError (pHdl, error::INVALID_PARAMS);
            return;

        }

        ch = req["params"]["instrument_name"];

        if (SubscribeTo (ch)) {
            Subscribe (pHdl, ch);
            SendResponse (pHdl, 10);
            return;
        }

        SendError (pHdl, error::SERVER_CALL_FAILED);

    } else if (method == EP_UNSUBSCRIBE) {
        
        if (req["params"].is_null () || req["params"]["instrument_name"].is_null ()) {
            SendError (pHdl, error::INVALID_PARAMS);
            return;

        }

        ch = req["params"]["instrument_name"];

        // irrespective of the response from the server
        // data stream publishing to the client should stop
        Unsubscribe (pHdl, ch);

        SendResponse (pHdl, 11);
    }
}
