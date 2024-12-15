#include "wsserver.hpp"

int main ()
{
    WSServer server;

    server.Listen ();

    while (true);

    return 0;
}