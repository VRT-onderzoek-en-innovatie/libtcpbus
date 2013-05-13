#include <config.h>
#include "../include/libtcpbus.h"

#include <netinet/in.h>
#include <ev.h>
#include <stdio.h>

static const int MAX_CONN_BACKLOG = 32;

int s_listen;


void received_sigint(EV_P_ ev_signal *w, int revents) {
	fprintf(stderr, "Received SIGINT, exiting");
	ev_break(EV_A_ EVUNLOOP_ALL);
}
void received_sigterm(EV_P_ ev_signal *w, int revents) {
	fprintf(stderr, "Received SIGTERM, exiting");
	ev_break(EV_A_ EVUNLOOP_ALL);
}


int main(int argc, char* argv[]) {
	struct TcpBus_bus *bus;

	fprintf(stderr, "%s version %s (%s) starting up", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_GITREVISION);

	{ // Open listening socket
		struct sockaddr_in addr;
		int rv;
		socklen_t addr_len = sizeof(addr);

		s_listen = socket(AF_INET, SOCK_STREAM, 0);

		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(10000);

		rv = bind(s_listen, &addr, addr_len);
		fprintf(stderr, "bind(): %d", rv);
		rv = listen(s_listen, MAX_CONN_BACKLOG);
		fprintf(stderr, "listen(): %d", rv);
		fprintf(stderr, "Listening on [127.0.0.1]:10000");
	}

	{
		ev_signal ev_sigint_watcher;
		ev_signal_init( &ev_sigint_watcher, received_sigint, SIGINT);
		ev_signal_start( EV_DEFAULT_ &ev_sigint_watcher);
		ev_signal ev_sigterm_watcher;
		ev_signal_init( &ev_sigterm_watcher, received_sigterm, SIGTERM);
		ev_signal_start( EV_DEFAULT_ &ev_sigterm_watcher);

		bus = TcpBus_init(EV_DEFAULT_ s_listen);

		fprintf(stderr, "Setup done, starting event loop");

		ev_run(EV_DEFAULT_ 0);
	}

	TcpBus_terminate(bus);

	fprintf(stderr, "Cleaning up");

	return 0;
}
