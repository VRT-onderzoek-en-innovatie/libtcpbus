#include "../include/libtcpbus.h"

int main() {
	TcpBus_init(EV_DEFAULT_ 0);

	TcpBus_terminate(0);

	TcpBus_send(0, 0, 0);

	TcpBus_callback_newcon_add(0, 0);

	return 0;
}
