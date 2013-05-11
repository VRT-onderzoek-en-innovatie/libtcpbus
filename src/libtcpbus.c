#include "../config.h"

#include "../include/libtcpbus.h"

#include "list.h"
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <liblog.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

ev_io e_listen;
EV_P;

struct connection {
	struct list_head list;
	int socket;
	char *addr_str;
	ev_io read_ready;
};
LIST_HEAD(connections);

struct callback_rx_t {
	struct list_head list;
	TcpBus_rx_callback_t f;
};
LIST_HEAD(callback_rx);


static void insert_connection(struct connection *new) {
	list_add(&new->list, &connections);
}

static void kill_connection(struct connection *c) {
	ev_io_stop(EV_A_ &c->read_ready);
	close(c->socket);
	free(c->addr_str);
	list_del(&c->list);
	free(c);
}



static void stringify_sockaddr(char *out, size_t out_len,
                               const struct sockaddr *sa, socklen_t sa_len) {
	char *p = out;

	switch(sa->sa_family) {
	case AF_INET:
		*(p++) = '['; out_len--;

		if( inet_ntop(AF_INET, &((struct sockaddr_in*)sa)->sin_addr, p, out_len) == NULL ) {
			strcpy(out, "inet_ntop() failed"); // Fits in 1+15+1+1+5+\0 = 23+\0
			return;
		}

		out_len -= strlen(p); p += strlen(p);

		*(p++) = ']'; out_len--;
		*(p++) = ':'; out_len--;

		if( -1 == snprintf(p, out_len, "%d", ntohs( ((struct sockaddr_in*)sa)->sin_port )) ) {
			strcpy(out, "snprintf() failed"); // Fits in 1+15+1+1+5+\0 = 23+\0
			return;
		}
		break;

#ifdef ENABLE_IPV6
	case AF_INET6:
		*(p++) = '['; out_len--;

		if( inet_ntop(AF_INET6, &((struct sockaddr_in6*)sa)->sin6_addr, p, out_len) == NULL ) {
			strcpy(out, "inet_ntop() failed"); // Fits in 1+15+1+1+5+\0 = 23+\0
			return;
		}

		out_len -= strlen(p); p += strlen(p);

		*(p++) = ']'; out_len--;
		*(p++) = ':'; out_len--;

		if( -1 == snprintf(p, out_len, "%d", ntohs( ((struct sockaddr_in6*)sa)->sin6_port )) ) {
			strcpy(out, "snprintf() failed"); // Fits in 1+15+1+1+5+\0 = 23+\0
			return;
		}
		break;
#endif

	default:
		strcpy(out, "Unknown address family"); // Fits in 1+15+1+1+5+\0 = 23+\0
	}
}

static void send_data(const char *data, size_t len, struct connection *skip) {
	struct connection *i, *tmp;
	list_for_each_entry_safe(i, tmp, &connections, list) {
		int rv;

		if( i == skip ) continue; // Don't loop to self

		rv = send(i->socket, data, len, 0);
		if( rv == -1 ) {
			LogError("%s : could not send(): %s", i->addr_str, strerror(errno));
			kill_connection(i); // Removes from list
		}
	}
}

static void ready_to_read(EV_P_ ev_io *w, int revents) {
	struct connection *con = w->data;
	char buf[4096];
	ssize_t rx_len;
	struct callback_rx_t *i;

	rx_len = recv(con->socket, buf, sizeof(buf), 0);
	if( rx_len == -1 ) {
		LogError("%s : could not recv() : %s", con->addr_str, strerror(errno));
		kill_connection(con);
		return;
	}
	if( rx_len == 0 ) { // EOF
		LogInfo("%s : disconnect", con->addr_str);
		kill_connection(con);
		return;
	}

	send_data(buf, rx_len, con);
	list_for_each_entry(i, &callback_rx, list) {
		i->f(buf, rx_len);
	}
}

static void incomming_connection(EV_P_ ev_io *w, int revents) {
	struct connection *con;
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof(addr);
	int flags, rv;

	con = malloc(sizeof(struct connection));
	if( con == NULL ) {
		LogError("Could not malloc()");
		return;
	}
	INIT_LIST_HEAD(&con->list);

#ifdef ENABLE_IPV6
	size_t addr_str_len = 1+INET6_ADDRSTRLEN+1+1+5; // [::]:12345 (terminating \0 included in constant)
#else
	size_t addr_str_len = 1+INET_ADDRSTRLEN+1+1+5; // [0.0.0.0]:12345 (terminating \0 included in constant)
#endif
	con->addr_str = malloc(addr_str_len);
	if( con->addr_str == NULL ) {
		LogError("Could not malloc()");
		return;
	}

	con->socket = accept(w->fd, (struct sockaddr*)&addr, &addr_len);
	if( con->socket == -1 ) {
		LogError("Could not accept(): %s", strerror(errno));
		free(con->addr_str);
		return;
	}

	stringify_sockaddr(con->addr_str, addr_str_len, (struct sockaddr*)&addr, addr_len);

	LogInfo("%s : Connection opened", con->addr_str);

	flags = fcntl(con->socket, F_GETFL);
	if( flags == -1 ) {
		LogError("%s : fcntl(, F_GETFL) failed: %s", con->addr_str, strerror(errno));
		goto cleanup;
	}
	rv = fcntl(con->socket, F_SETFL, flags | O_NONBLOCK);
	if( rv == -1 ) {
		LogError("%s : fcntl(, F_SETFL) failed: %s", con->addr_str, strerror(errno));
		goto cleanup;
	}

	ev_io_init( &con->read_ready, ready_to_read, con->socket, EV_READ);
	con->read_ready.data = con; // Could be replaced with offset_of magic
	ev_io_start(EV_A_ &con->read_ready);

	insert_connection(con);
	return;

cleanup:
	close(con->socket);
	free(con->addr_str);
}

int TcpBus_init(
#ifdef EV_MULTIPLICITY
		struct ev_loop *init_loop,
#endif
		int socket) {
	struct sigaction act;

	loop = init_loop;

	if( sigaction(SIGPIPE, NULL, &act) == -1) {
		LogError("Can not get signal handler for SIGPIPE");
		return -1;
	}
	act.sa_handler = SIG_IGN; // Ignore SIGPIPE (we'll handle the write()-error)
	if( sigaction(SIGPIPE, &act, NULL) == -1 ) {
		LogError("Can not set signal handler for SIGPIPE");
		return -1;
	}

	ev_io_init(&e_listen, incomming_connection, socket, EV_READ);
	ev_io_start(EV_A_ &e_listen);
	return 0;
}

void TcpBus_terminate() {
	struct connection *i, *tmp;

	ev_io_stop(EV_A_ &e_listen);

	list_for_each_entry_safe(i, tmp, &connections, list) {
		kill_connection(i);
	}
}

int TcpBus_rx_callback_add(TcpBus_rx_callback_t f) {
	struct callback_rx_t *cb;

	cb = malloc(sizeof(struct callback_rx_t));
	if( cb == NULL ) return -1;
	cb->f = f;

	list_add(&cb->list, &callback_rx);
	return 0;
}

int TcpBus_rx_callback_remove(TcpBus_rx_callback_t f) {
	struct callback_rx_t *i, *tmp;
	list_for_each_entry_safe(i, tmp, &callback_rx, list) {
		if( i->f == f ) {
			list_del(&i->list);
			free(i);
		}
	}
	return 0;
}

int TcpBus_send(const char *data, size_t len) {
	send_data(data, len, NULL);
	return 0;
}
