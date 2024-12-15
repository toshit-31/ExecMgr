#ifndef CONST_HPP
#define CONST_HPP

#include "types.h"

#define CLIENT_ID           "phQzMBQj"
#define CLIENT_SECRET       "TGyKQGMvYIzf1DdjVbDuxE4G8nuXfYad_aKjIljJTf4"

#define EP_COUNT            11

#define DB_BASE_URL         "wss://test.deribit.com/ws/api/v2"
#define WS_BASE_URL         "ws://127.0.0.1:8001"

#define EP_AUTH             "public/auth"
#define EP_SUBSCRIBE        "public/subscribe"
#define EP_UNSUBSCRIBE      "public/unsubscribe"

#define EP_ORDER_BOOK       "public/get_order_book"
#define EP_ORDER_STATUS     "private/get_order_state"
#define EP_POSITION         "private/get_position"
#define EP_ALL_POSITIONS    "private/get_positions"

#define EP_TRADE_BUY        "private/buy"
#define EP_TRADE_SELL       "private/sell"
#define EP_TRADE_EDIT       "private/edit"
#define EP_TRADE_CANCEL     "private/cancel"

#define TRADE_TYPE_LIMIT    "limit"
#define TRADE_TYPE_MARKET   "market"

#define OS_CANCEL           "cancelled"
#define OS_OPEN             "open"
#define OS_FILLED           "filled"

#endif // !CONST_HPP
