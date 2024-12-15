#ifndef WSSCLIENT_HXX
#define WSSCLIENT_HXX

inline void WSSClient::WaitForConnection ()
{
    while (!vConnected);
}

inline void WSSClient::OnMessage (MsgCallback pCB, VPtr pObj)
{
    vMsgCb    = pCB;
    vMsgParam = pObj;
}

inline void WSSClient::OnError (ErrCallback pCB, VPtr pObj)
{
    vErrCb    = pCB;
    vErrParam = pObj;
}

bool WSSClient::IsConnected ()
{
    return vConnected;
}

#endif // !WSSCLIENT_HXX
