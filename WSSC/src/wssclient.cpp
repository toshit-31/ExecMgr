#include "wssclient.hpp"

WSSClient::WSSClient ()
{
    vSocket.init_asio();

    vSocket.clear_access_channels (websocketpp::log::alevel::all);
    vSocket.clear_error_channels (websocketpp::log::elevel::all); 

    // Set event handlers
    vSocket.set_tls_init_handler(bind (&WSSClient::OnTLSInit, this, std::placeholders::_1));
    vSocket.set_open_handler    (bind (&WSSClient::OnOpen, this, std::placeholders::_1));
    vSocket.set_close_handler   (bind (&WSSClient::OnClose, this, std::placeholders::_1));
    vSocket.set_message_handler (bind (&WSSClient::OnMsgRecv, this, std::placeholders::_1, std::placeholders::_2));

    vIsBusy = false;
}

WSSClient::~WSSClient ()
{
    Stop ();
    delete[] vResponses;
}

bool WSSClient::Connect (CStrPtr pURL)
{
        ErrCode ec;
        SClient::connection_ptr con = vSocket.get_connection (pURL, ec);

    if (ec) {
        return false;
    }

    vHandle = con->get_handle ();
    vSocket.connect (con);

    thread t (&WSSClient::Run, this);
    t.detach ();

    WaitForConnection ();

    // number of response elements can be adjusted based on the number of different operation that we have
    vResponses = new Json[EP_COUNT + 1];

    return true;
}

bool WSSClient::Send (String & pEndpoint, Long pId, Json & pParam)
{
        ErrCode ec;
        Request r (pId, pEndpoint, pParam);

    vSocket.send (vHandle, r.to_json ().dump (4), websocketpp::frame::opcode::text, ec);

    if (ec) {
        return false;
    }

    return true;
}

bool WSSClient::Send (String & pEndpoint, Long pId, Json & pParam, Json & pResp)
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

void WSSClient::OnOpen (Handle pHdl)
{
    vConnected = true;
    printf ("Connected\n");
}

void WSSClient::OnClose (Handle pHdl)
{
    vConnected = false;
}

void WSSClient::OnMsgRecv (Handle pHdl, MsgPtr pMsg)
{
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
            String msg      = res["error"]["message"];

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

void WSSClient::Run ()
{
    vSocket.run ();
}

void WSSClient::Stop ()
{
    vSocket.stop ();
}

CtxPtr WSSClient::OnTLSInit (Handle pHdl)
{
    CtxPtr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    ctx->set_options(boost::asio::ssl::context::default_workarounds |
        boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3 |
        boost::asio::ssl::context::single_dh_use);

    return ctx;
}
