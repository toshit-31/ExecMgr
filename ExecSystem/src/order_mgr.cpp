#include "order_mgr.hpp"

OrderMgr * OrderMgr::GetInstance ()
{
        static OrderMgr order_mgr;

    return &order_mgr;
}

OrderMgr::OrderMgr ()
{
    vAuthed = false;
    vOrders = nullptr;
    
    vDBClient.OnMessage (&OrderMgr::OnDBResponseCallback, this);
    vDBClient.OnError   (&OrderMgr::OnDBErrorCallback, this);

    vWSClient.OnMessage (&OrderMgr::OnWSResponseCallback, this);
    vWSClient.OnError   (&OrderMgr::OnWSErrorCallback, this);

    vSubSymbols = nullptr;
}

stOrder * OrderMgr::FindOrderByOrderId (CStrPtr pOrderId)
{
        stOrder * temp = vOrders;

    while (temp) {
        if (strcmp (pOrderId, temp->uOrderId) == 0) {
            return temp;
        }

        temp = temp->uNext;
    }

    return nullptr;
}

void OrderMgr::Auth ()
{
        Long    id = 1;
        String  ep = EP_AUTH;
        Json    params;

    params["grant_type"]    = "client_credentials";
    params["client_id"]     = CLIENT_ID;
    params["client_secret"] = CLIENT_SECRET;

    vDBClient.Send (ep, id, params);

    WaitForAuth ();
}

void OrderMgr::OnDBResponseCallback (Long pId, Json & pResp, VPtr pObj)
{
        OrderMgr * inst = (OrderMgr *) pObj; 
        Json temp;

    switch (pId) {
        
        case 1: // public/auth
        {
            inst->vAccessToken  = pResp["access_token"];
            inst->vRefreshToken = pResp["refresh_token"];
            inst->vAuthed = true;
            break;
        }

        default:
        {
            cout << pResp.dump (4) << endl;
        }
    }
}

void OrderMgr::OnDBErrorCallback (Int pErrId, CString & pMsg, VPtr pObj)
{
    printf ("OrderMgr::Error | %d : %s\n", pErrId, pMsg.c_str ());
}

void OrderMgr::OnWSResponseCallback (Long pId, Json & pResp, VPtr pObj)
{
    OrderMgr * inst = (OrderMgr *) pObj; 
    Json temp;

    switch (pId) {

        case 0: 
        {
                stOrderBookNotif * st;

            temp = pResp["params"]["data"];

            if (!MAP_CONTAINS (inst->vSubSymbols, temp["instrument_name"])) {
                break;
            }

            st = (*inst->vSubSymbols)[temp["instrument_name"]];

            st->uAsks = move (temp["asks"]);
            st->uBids = move (temp["bids"]);
            st->uCallback (st->uAsks, st->uBids);

            break;
        }

        default:
        {
            cout << pResp.dump (4) << endl;
        }
    }
}

void OrderMgr::OnWSErrorCallback (Int pErrId, CString & pMsg, VPtr pObj)
{
    printf ("OrderMgr::Error %d : %s\n", pErrId, pMsg.c_str ());
}

OrderMgr::~OrderMgr ()
{
    delete vSubSymbols;
}

void OrderMgr::Prepare (bool pConnectToLocalServer)
{
    vDBClient.Connect (DB_BASE_URL);
    if (pConnectToLocalServer) vWSClient.Connect (WS_BASE_URL);
    Auth ();

    vSubSymbols = new map<String, stOrderBookNotif*>;
}

Json OrderMgr::GetOrderBook (CString & pInstrName, Long pDepth)
{
        Json   order_book; // response
        Json   params;     // request params
        String ep = EP_ORDER_BOOK;
        Long   id = 2;

    params["instrument_name"] = pInstrName;
    params["depth"] = pDepth;

    vDBClient.Send (ep, id, params, order_book);

    return order_book;
}

Json OrderMgr::PlaceBuyOrder (CString & pInstrName, double pAmt, double pPrice, CString & pLabel)
{
        Json   resp;    // response
        Json   params;  // request params
        Json   temp;
        String ep = EP_TRADE_BUY;
        Long   id = 3;

    params["instrument_name"] = pInstrName;
    params["amount"] = pAmt;
    params["price"] = pPrice;
    params["label"] = pLabel;

    params["type"] = TRADE_TYPE_LIMIT;

    if (vDBClient.Send (ep, id, params, resp)) {
            String     s;
            stOrder * ord = new stOrder;
            temp  = resp["order"];

        cout << resp.dump (4) << endl;

        ord->uIsBuy = true;
        ord->uAmt   = pAmt;
        ord->uPrice = pPrice;

        s = temp["order_id"];
        strcpy_s (ord->uOrderId, MAX_ORDER_STR_LEN, s.c_str ());

        s = temp["order_state"];
        strcpy_s (ord->uState, MAX_ORDER_STR_LEN, s.c_str ());

        ord->uNext = vOrders;
        vOrders = ord;
    };

    return resp;
}

Json OrderMgr::PlaceSellOrder (CString & pInstrName, double pAmt, double pPrice, CString & pLabel)
{
        Json   resp;    // response
        Json   params;  // request params
        String ep = EP_TRADE_SELL;
        Long   id = 4;

    params["instrument_name"] = pInstrName;
    params["amount"] = pAmt;
    params["price"] = pPrice;
    params["label"] = pLabel;

    params["type"] = TRADE_TYPE_LIMIT;

    if (vDBClient.Send (ep, id, params, resp)) {
            String     s;
            stOrder * ord = new stOrder;

        ord->uIsBuy = false;
        ord->uAmt   = pAmt;
        ord->uPrice = pPrice;

        s = resp["order"]["order_id"];
        strcpy_s (ord->uOrderId, MAX_ORDER_STR_LEN, s.c_str ());

        s = resp["order"]["order_state"];
        strcpy_s (ord->uState, MAX_ORDER_STR_LEN, s.c_str ());

        ord->uNext = vOrders;
        vOrders = ord;
    };

    return resp;
}

Json OrderMgr::CancelOrder (CString & pOrderId)
{
        stOrder * ord = FindOrderByOrderId (pOrderId.c_str ());
        Json       resp;    // response
        Json       params;  // request params
        String     ep = EP_TRADE_CANCEL;
        Long       id = 5;

    if (!ord) {
        return nullptr;

    }

    params["order_id"] = pOrderId; 

    if (vDBClient.Send (ep, id, params, resp)) {
        strcpy_s (ord->uState, MAX_ORDER_STR_LEN, OS_CANCEL);

    }

    return resp;
}

Json OrderMgr::ModifyOrder (CString & pOrderId, double pAmt, double pPrice)
{
        stOrder * ord = FindOrderByOrderId (pOrderId.c_str ());
        Json       resp;    // response
        Json       params;  // request params
        String     ep = EP_TRADE_EDIT;
        Long       id = 6;

    if (!ord) {
        return nullptr;

    }

    params["order_id"] = pOrderId;
    params["amount"] = pAmt;
    params["price"] = pPrice;

    if (vDBClient.Send (ep, id, params, resp)) {
        ord->uAmt   = pAmt;
        ord->uPrice = pPrice;

    }

    return resp;
}

stOrder * OrderMgr::GetOrderStatus (CString & pOrderId)
{
        stOrder * ord = FindOrderByOrderId (pOrderId.c_str ());
        Json       resp;    // response
        Json       params;  // request params
        String     ep = EP_ORDER_STATUS;
        Long       id = 7;
        String     state;

    // invalid order id
    if (!ord) {
        return nullptr;

    }

    params["order_id"] = pOrderId;

    if (vDBClient.Send (ep, id, params, resp)) {
        state = resp["order_state"];
        strcpy_s (ord->uState, MAX_ORDER_STR_LEN, state.c_str ());

    }

    return ord;
}

Json OrderMgr::ViewPosition (CString & pInstrName)
{
        Json       resp;    // response
        Json       params;  // request params
        String     ep = EP_POSITION;
        Long       id = 8;
        String     state;

    params["instrument_name"] = pInstrName;

    vDBClient.Send (ep, id, params, resp);

    return resp;
}

Json OrderMgr::ViewPosition (CString & pCurrency, bool & pIsFuture)
{
        Json       resp;    // response
        Json       params;  // request params
        String     ep = EP_ALL_POSITIONS;
        Long       id = 9;
        String     state;

    params["currency"] = pCurrency;
    params["kind"] = pIsFuture ? "future" : "option";

    vDBClient.Send (ep, id, params, resp);

    return resp;
}

bool OrderMgr::SubscribeTo (CString & pInstrName, Long /*pDepth*/, OrderBookNotifCallback pCallback)
{
        Json       params;  // request params
        String     ep = EP_SUBSCRIBE;
        Long       id = 10;
        
    if (!pCallback) {
        return false;

    }

    params["instrument_name"] = pInstrName;

    // non blocking send
    // subscription call successful
    if (vWSClient.Send (ep, id, params)) {
        vSubSymbols->insert (make_pair (pInstrName, new stOrderBookNotif (pCallback)));
        return true;

    }

    return false;
}

bool OrderMgr::UnsubscribeFrom (CString & pInstrName)
{
    Json       params;  // request params
    String     ep = EP_UNSUBSCRIBE;
    Long       id = 11;
    
    params["instrument_name"] = pInstrName;

    // non blocking send
    // subscription call successful
    if (vWSClient.Send (ep, id, params)) {
        vSubSymbols->erase (pInstrName);
        return true;

    }

    return false;
}