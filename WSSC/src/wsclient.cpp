#include "wsclient.hpp"

WSClient::WSClient ()
{
    vSocket.init_asio();

    vSocket.clear_access_channels (websocketpp::log::alevel::all);
    vSocket.clear_error_channels  (websocketpp::log::elevel::all); 

    // Set event handlers
    vSocket.set_open_handler    (bind (&WSClient::OnOpen, this, std::placeholders::_1));
    vSocket.set_close_handler   (bind (&WSClient::OnClose, this, std::placeholders::_1));
    vSocket.set_message_handler (bind (&WSClient::OnMsgRecv, this, std::placeholders::_1, std::placeholders::_2));

    vIsBusy = false;
}

WSClient::~WSClient ()
{
    Stop ();
    delete[] vResponses;
}

bool WSClient::Connect (CStrPtr pURL)
{
    ErrCode ec;
    Client::connection_ptr con = vSocket.get_connection (pURL, ec);

    if (ec) {
        return false;
    }

    vHandle = con->get_handle ();
    vSocket.connect (con);

    thread t (&WSClient::Run, this);
    t.detach ();

    WaitForConnection ();

    // number of response elements can be adjusted based on the number of different operation that we have
    vResponses = new Json[EP_COUNT + 1];

    return true;
}

bool WSClient::Send (String & pEndpoint, Long pId, Json & pParam)
{
    ErrCode ec;
    Request r (pId, pEndpoint, pParam);

    vSocket.send (vHandle, r.to_json ().dump (4), websocketpp::frame::opcode::text, ec);

    if (ec) {
        return false;
    }

    return true;
}

bool WSClient::Send (String & pEndpoint, Long pId, Json & pParam, Json & pResp)
{
    ErrCode ec;
    Request r (pId, pEndpoint, pParam);

    vSocket.send (vHandle, r.to_json ().dump (4), websocketpp::frame::opcode::text, ec);

    if (ec) {
        return false;
    }

    vIsBusy = true;

    while (vIsBusy);

    pResp = vResponses[pId];

    return !pResp.empty ();
}

void WSClient::OnOpen (Handle pHdl)
{
    vConnected = true;
    printf ("Connected\n");
}

void WSClient::OnClose (Handle pHdl)
{
    vConnected = false;
}

void WSClient::OnMsgRecv (Handle pHdl, MsgPtr pMsg)
{
    //lock_guard<mutex> lock (vLock);

    Json res = Json::parse (pMsg->get_payload ().c_str ());
    Json result;
    Int  res_id;

    if (!res.contains ("id")) {
        result = res;
        vMsgCb (0, result, vMsgParam);
        vIsBusy = false;
        return;
    }

    res_id = res["id"];

    if (res.contains ("error")) {
            Int    err_code = res["error"]["code"];
            String msg      = res["error"]["string"];

        vResponses[res_id].clear ();
        vErrCb (err_code, msg, vErrParam);

    } else {
        result = res["result"];

        vResponses[res_id].clear ();
        vResponses[res_id] = result;
        vMsgCb (res_id, vResponses[res_id], vMsgParam);

    }

    // set the state to free
    vIsBusy = false;
}

void WSClient::Run ()
{
    vSocket.run ();
}

void WSClient::Stop ()
{
    vSocket.stop ();
}