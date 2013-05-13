#ifndef __LIBTCPBUS_H__
#define __LIBTCPBUS_H__

#include <ev.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C"{
#endif

/* initialize the event listeners
 * socket should be set up in listening state
 * Note that this sets up the SIGPIPE signal to be ignored.
 *
 * Returns 0 on success, -1 on error
 */
int TcpBus_init(EV_P_ int socket);

/* shut down the bus
 * all open connections will be closed
 * Note that the listening socket will NOT be closed
 */
void TcpBus_terminate();

/* Send data to the bus
 * returns -1 on failure
 */
int TcpBus_send(const char *data, size_t len);


/* Callbacks
 ************/

typedef void (*TcpBus_callback_rx_t)(const char *data, size_t len);
typedef void (*TcpBus_callback_newcon_t)(const struct sockaddr *addr,
                                         socklen_t addr_len);
typedef void (*TcpBus_callback_error_t)(const struct sockaddr *addr,
                                        socklen_t addr_len,
                                        int err_no);
typedef void (*TcpBus_callback_disconnect_t)(const struct sockaddr *addr,
                                             socklen_t addr_len);

/* Register functions
 * add the given callback function to the list of callbacks
 *
 * Returns 0 on success, -1 on failure
 */
int TcpBus_callback_rx_add(TcpBus_callback_rx_t f);
int TcpBus_callback_newcon_add(TcpBus_callback_newcon_t f);
int TcpBus_callback_error_add(TcpBus_callback_error_t f);
int TcpBus_callback_disconnect_add(TcpBus_callback_disconnect_t f);

/* Remove a previously added callback
 *
 * Returns 0 on success
 */
int TcpBus_callback_rx_remove(TcpBus_callback_rx_t f);
int TcpBus_callback_newcon_remove(TcpBus_callback_newcon_t f);
int TcpBus_callback_eror_remove(TcpBus_callback_error_t f);
int TcpBus_callback_disconnect_remove(TcpBus_callback_disconnect_t f);



#ifdef __cplusplus
}
#endif

#endif // defined __LIBTCPBUS_H__
