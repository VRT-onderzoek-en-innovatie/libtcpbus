#include "../config.h"

#include "../include/libtcpbus.h"

#include "list.h"
#include <errno.h>
#include <string.h>
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
	struct sockaddr_storage addr;
	socklen_t addr_len;
	ev_io read_ready;
};
LIST_HEAD(connections);


#define callback_list(type) \
	struct callback_ ## type ## _t { \
		struct list_head list; \
		TcpBus_callback_ ## type ## _t f; \
	}; \
	LIST_HEAD(callback_ ## type);
callback_list(rx);
callback_list(newcon);
callback_list(error);
callback_list(disconnect);

#define callback_add_remove(type) \
	int TcpBus_callback_ ## type ## _add(TcpBus_callback_ ## type ## _t f) { \
		struct callback_ ## type ## _t *cb; \
		\
		cb = malloc(sizeof *cb); \
		if( cb == NULL ) return -1; \
		cb->f = f; \
		\
		list_add(&cb->list, &callback_ ## type); \
		return 0; \
	} \
	int TcpBus_callback_ ## type ## _remove(TcpBus_callback_ ## type ## _t f) { \
		struct callback_ ## type ## _t *i, *tmp; \
		list_for_each_entry_safe(i, tmp, &callback_ ## type, list) { \
			if( i->f == f ) { \
				list_del(&i->list); \
				free(i); \
			} \
		} \
		return 0; \
	}
callback_add_remove(rx)
callback_add_remove(newcon)
callback_add_remove(error)
callback_add_remove(disconnect)

static inline void callback_rx_call(const char *buf, size_t rx_len) {
	struct callback_rx_t *i;
	list_for_each_entry(i, &callback_rx, list) {
		i->f(buf, rx_len);
	}
}

static inline void callback_newcon_call(const struct sockaddr_storage *addr, socklen_t addr_len) {
	struct callback_newcon_t *i;
	list_for_each_entry(i, &callback_newcon, list) {
		i->f((struct sockaddr*)addr, addr_len);
	}
}

static inline void callback_error_call(const struct sockaddr_storage *addr, socklen_t addr_len, int err) {
	struct callback_error_t *i;
	list_for_each_entry(i, &callback_error, list) {
		i->f((struct sockaddr*)addr, addr_len, err);
	}
}

static inline void callback_disconnect_call(const struct sockaddr_storage *addr, socklen_t addr_len) {
	struct callback_disconnect_t *i;
	list_for_each_entry(i, &callback_disconnect, list) {
		i->f((struct sockaddr*)addr, addr_len);
	}
}



static void kill_connection(struct connection *c) {
	ev_io_stop(EV_A_ &c->read_ready);
	close(c->socket);
	list_del(&c->list);
	free(c);
}


static void send_data(const char *data, size_t len, struct connection *skip) {
	struct connection *i, *tmp;
	list_for_each_entry_safe(i, tmp, &connections, list) {
		int rv;

		if( i == skip ) continue; // Don't loop to self

		rv = send(i->socket, data, len, 0);
		if( rv == -1 ) {
			callback_error_call(&i->addr, i->addr_len, errno);
			kill_connection(i); // Removes from list
		}
	}
}

static void ready_to_read(EV_P_ ev_io *w, int revents) {
	struct connection *con = w->data;
	char buf[4096];
	ssize_t rx_len;

	rx_len = recv(con->socket, buf, sizeof(buf), 0);
	if( rx_len == -1 ) {
		callback_error_call(&con->addr, con->addr_len, errno);
		kill_connection(con);
		return;
	}
	if( rx_len == 0 ) { // EOF
		callback_disconnect_call(&con->addr, con->addr_len);
		kill_connection(con);
		return;
	}

	send_data(buf, rx_len, con);
	callback_rx_call(buf, rx_len);
}

static void incomming_connection(EV_P_ ev_io *w, int revents) {
	struct connection *con;
	int flags, rv;

	con = malloc(sizeof(struct connection));
	if( con == NULL ) {
		callback_error_call(NULL, 0, ENOMEM);
		return;
	}
	INIT_LIST_HEAD(&con->list);
	con->addr_len = sizeof(con->addr);

	con->socket = accept(w->fd, (struct sockaddr*)&con->addr, &con->addr_len);
	if( con->socket == -1 ) {
		callback_error_call(NULL, 0, errno);
		return;
	}

	callback_newcon_call(&con->addr, con->addr_len);

	flags = fcntl(con->socket, F_GETFL);
	if( flags == -1 ) {
		callback_error_call(&con->addr, con->addr_len, errno);
		goto cleanup;
	}
	rv = fcntl(con->socket, F_SETFL, flags | O_NONBLOCK);
	if( rv == -1 ) {
		callback_error_call(&con->addr, con->addr_len, errno);
		goto cleanup;
	}

	ev_io_init( &con->read_ready, ready_to_read, con->socket, EV_READ);
	con->read_ready.data = con; // Could be replaced with offset_of magic
	ev_io_start(EV_A_ &con->read_ready);

	list_add(&con->list, &connections);
	return;

cleanup:
	close(con->socket);
	free(con);
}

int TcpBus_init(
#ifdef EV_MULTIPLICITY
		struct ev_loop *init_loop,
#endif
		int socket) {
	struct sigaction act;

	loop = init_loop;

	if( sigaction(SIGPIPE, NULL, &act) == -1) {
		callback_error_call(NULL, 0, errno);
		return -1;
	}
	act.sa_handler = SIG_IGN; // Ignore SIGPIPE (we'll handle the write()-error)
	if( sigaction(SIGPIPE, &act, NULL) == -1 ) {
		callback_error_call(NULL, 0, errno);
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




int TcpBus_send(const char *data, size_t len) {
	send_data(data, len, NULL);
	return 0;
}
