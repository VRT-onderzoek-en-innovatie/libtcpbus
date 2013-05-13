#ifndef __LIBTCPBUS_H__
#define __LIBTCPBUS_H__

#include <ev.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C"{
#endif

struct TcpBus_bus;

/* Initialize a new TCP-bus.
 *
 * @loop is the libev-loop to use
 * @socket is the listening socket
 *
 * returns a pointer to an TcpBus_bus structure which represents this bus.
 * or NULL if an error occured
 *
 * Note that you should probably set SIGPIPE to be ignored if you want to
 * catch overflows on the Tx buffer.
 */
struct TcpBus_bus *TcpBus_init(EV_P_ int socket);

/* shut down the bus
 * all open connections will be closed
 * Note that the listening socket will NOT be closed
 */
void TcpBus_terminate(struct TcpBus_bus *bus);

/* Send data to the bus
 * returns -1 on failure
 */
int TcpBus_send(const struct TcpBus_bus *bus, const char *data, size_t len);


/* Callbacks
 ************/

typedef void (*TcpBus_callback_rx_t)(const struct TcpBus_bus *bus,
                                     const char *data, size_t len);
typedef void (*TcpBus_callback_newcon_t)(const struct TcpBus_bus *bus,
                                         const struct sockaddr *addr,
                                         socklen_t addr_len);
typedef void (*TcpBus_callback_error_t)(const struct TcpBus_bus *bus,
                                        const struct sockaddr *addr,
                                        socklen_t addr_len,
                                        int err_no);
typedef void (*TcpBus_callback_disconnect_t)(const struct TcpBus_bus *bus,
                                             const struct sockaddr *addr,
                                             socklen_t addr_len);

/* Register functions
 * add the given callback function to the list of callbacks
 *
 * Returns 0 on success, -1 on failure
 */
int TcpBus_callback_rx_add(struct TcpBus_bus *bus,
                           TcpBus_callback_rx_t f);
int TcpBus_callback_newcon_add(struct TcpBus_bus *bus,
                               TcpBus_callback_newcon_t f);
int TcpBus_callback_error_add(struct TcpBus_bus *bus,
                              TcpBus_callback_error_t f);
int TcpBus_callback_disconnect_add(struct TcpBus_bus *bus,
                                   TcpBus_callback_disconnect_t f);

/* Remove a previously added callback
 *
 * Returns 0 on success
 */
int TcpBus_callback_rx_remove(struct TcpBus_bus *bus,
                              TcpBus_callback_rx_t f);
int TcpBus_callback_newcon_remove(struct TcpBus_bus *bus,
                                  TcpBus_callback_newcon_t f);
int TcpBus_callback_eror_remove(struct TcpBus_bus *bus,
                                TcpBus_callback_error_t f);
int TcpBus_callback_disconnect_remove(struct TcpBus_bus *bus,
                                      TcpBus_callback_disconnect_t f);



#ifdef __cplusplus
}
#endif

#endif // defined __LIBTCPBUS_H__
