#ifndef WSCLIENT_HXX
#define WSCLIENT_HXX

inline void WSClient::WaitForConnection ()
{
    while (!vConnected);
}

inline void WSClient::OnMessage (MsgCallback pCB, VPtr pObj)
{
    vMsgCb    = pCB;
    vMsgParam = pObj;
}

inline void WSClient::OnError (ErrCallback pCB, VPtr pObj)
{
    vErrCb    = pCB;
    vErrParam = pObj;
}

inline bool WSClient::IsConnected ()
{
    return vConnected;
}

#endif // !WSCLIENT_HXX
