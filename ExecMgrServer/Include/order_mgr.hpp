#ifndef ORDER_MGR
#define ORDER_MGR

#include "const.hpp"
#include "wssclient.hpp"
#include "wsclient.hpp"

#define MAX_ORDER_STR_LEN 64
#define MAP_CONTAINS(m,k) (m->find (k) != m->end ())

typedef void (*OrderBookNotifCallback) (Json & pAsks, Json & pBids);

struct stOrder {
    StrPtr    uOrderId;
    bool      uIsBuy;
    double    uAmt;
    double    uPrice;
    StrPtr    uState;
    stOrder * uNext;

    inline stOrder () {
        uOrderId = (StrPtr) malloc (MAX_ORDER_STR_LEN);
        uState   = (StrPtr) malloc (MAX_ORDER_STR_LEN);
    };

    inline ~stOrder () {
        free ((VPtr) uOrderId);
        free ((VPtr) uState);
    };
};

struct stOrderBookNotif {
    Json uAsks;
    Json uBids;
    OrderBookNotifCallback uCallback;

    inline stOrderBookNotif (OrderBookNotifCallback pCallback) {
        uCallback = pCallback;
    };
};

/**
* @Note 
* Not thread-safe
* Requires a single web socket client to run on a single thread
*/
class OrderMgr {

public:

                ~OrderMgr ();

        void    Prepare  ();

        Json    PlaceBuyOrder     (CString & pInstrName, double pAmt, double pPrice, CString & pLabel);
        Json    PlaceSellOrder    (CString & pInstrName, double pAmt, double pPrice, CString & pLabel);
        Json    CancelOrder       (CString & pOrderId);
        Json    ModifyOrder       (CString & pOrderId, double pAmt, double pPrice);

        stOrder * GetOrderStatus  (CString & pOrderId);

        Json    GetOrderBook      (CString & pInstrName, Long pDepth);
        Json    ViewPosition      (CString & pInstrName);
        Json    ViewPosition      (CString & pCurrency, bool & pIsFuture);

        bool    SubscribeTo       (CString & pInstrName, Long pDepth, OrderBookNotifCallback pCallback);
        bool    UnsubscribeFrom   (CString & pInstrName);

static  OrderMgr * GetInstance ();

private:

                OrderMgr ();

        stOrder * FindOrderByOrderId (CStrPtr pOrderId);

inline  void    WaitForAuth () { while (!vAuthed); };
        void    Auth        ();

static  void    OnDBResponseCallback (Long pId, Json & pResp, VPtr pObj);
static  void    OnDBErrorCallback    (Int pId, CString & pMsg, VPtr pObj);

static  void    OnWSResponseCallback (Long pId, Json & pResp, VPtr pObj);
static  void    OnWSErrorCallback    (Int pId, CString & pMsg, VPtr pObj);

        bool        vAuthed;
        stOrder *   vOrders;
        String      vAccessToken;
        String      vRefreshToken;
        WSSClient   vDBClient;
        WSClient    vWSClient;

        map<String, stOrderBookNotif*> * vSubSymbols;
};

#endif // ORDER_MGR