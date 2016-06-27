#include "looper.h"
#include "server.h"

int main() {
    Looper *looper;
    Server *server;
    int state;

    looper = new_looper();
    server = new_server(looper);

    state = looper_run(looper);

    destroy_server(server);
    destroy_looper(looper);

    return 0;
}
