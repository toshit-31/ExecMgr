#include "jsonrpcpp.hpp"
#include "order_mgr.hpp"

void OnBTCNotif (Json & pAsks, Json & pBids)
{
    printf ("%s\n", "BTC");
    printf ("%s\n", pAsks.dump (4).c_str ());
    printf ("%s\n", pBids.dump (4).c_str ());
    printf ("-----------------\n");
}

int main ()
{
    String btc = "BTC_USDT";

    OrderMgr * m = OrderMgr::GetInstance ();
    m->Prepare (true);

    m->SubscribeTo ("BTC_USDT", 0, OnBTCNotif);

    Json r = m->PlaceBuyOrder (btc, 0.0001, 101550, "asd");

    cout << r.dump (4) << endl;

    if (!r.is_null ()) {
        cout << m->GetOrderStatus (r["order"]["order_id"])->uState << endl;

    } else {
        cout << "error from deribit" << endl;
    }

    while (true);

    return 0;
}