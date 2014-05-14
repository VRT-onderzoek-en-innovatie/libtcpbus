#ifndef __LIBTCPBUS_H__
#define __LIBTCPBUS_H__

#include <ev.h>
#include <sys/socket.h>

#ifndef __GNUC__
#  define  __attribute__(x)  /*NOTHING*/
#endif


#ifdef __cplusplus
extern "C"{
#endif

/* This datastructure represents a TCP-bus
 */
struct TcpBus_bus;


/* Initialize a new TCP-bus.
 *
 * @loop is the libev-loop to use (if MULTIPLICITY is used).
 * @socket is a socket opened in listening mode.
 *
 * returns a pointer to an TcpBus_bus structure which represents this bus.
 * or NULL if an error occured
 *
 * Note that you should probably set SIGPIPE to be ignored if you want to
 * catch overflows on the Tx buffer.
 */
struct TcpBus_bus *TcpBus_init(EV_P_ int socket)
                              __attribute__((__malloc__,warn_unused_result));


/* shut down the bus
 * all open connections will be closed, and all resources free'd.
 * You should not use bus-pointer after this.
 *
 * @bus is the bus to shut down
 *
 * Note that the listening socket will NOT be closed
 */
void TcpBus_terminate(struct TcpBus_bus *bus) __attribute__((nonnull(1)));


/* Send data to the bus
 *
 * @bus is the bus to send the data to.
 * @data is the data to send of length @len
 *
 * returns -1 on failure
 *
 * Note that the bus normally cuts off connections that are too slow for the
 * bus, instead of holding back the whole bus to wait for them. (this may
 * become a setting in the future)
 */
int TcpBus_send(const struct TcpBus_bus *bus, const char *data, size_t len)
               __attribute__((nonnull(1,2)));


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
 * @bus is the bus this callback should be registered on
 * @f the callback function to register
 *
 * Returns 0 on success, -1 on failure
 *
 * Note that (as far as this library is concerned), it's perfectly fine to
 * register the same function twice (or more). In that case, it will get called
 * twice (or more).
 */
int TcpBus_callback_rx_add(struct TcpBus_bus *bus,
                           TcpBus_callback_rx_t f)
                          __attribute__((nonnull(1,2)));
int TcpBus_callback_newcon_add(struct TcpBus_bus *bus,
                               TcpBus_callback_newcon_t f)
                              __attribute__((nonnull(1,2)));
int TcpBus_callback_error_add(struct TcpBus_bus *bus,
                              TcpBus_callback_error_t f)
                             __attribute__((nonnull(1,2)));
int TcpBus_callback_disconnect_add(struct TcpBus_bus *bus,
                                   TcpBus_callback_disconnect_t f)
                                  __attribute__((nonnull(1,2)));


/* Remove a previously added callback
 * @bus is the bus this callback should be removed from
 * @f the callback function to remove
 *
 * Returns the number of callbacks removed, which might be 0
 * or -1 on error.
 */
int TcpBus_callback_rx_remove(struct TcpBus_bus *bus,
                              TcpBus_callback_rx_t f)
                             __attribute__((nonnull(1,2)));
int TcpBus_callback_newcon_remove(struct TcpBus_bus *bus,
                                  TcpBus_callback_newcon_t f)
                                 __attribute__((nonnull(1,2)));
int TcpBus_callback_eror_remove(struct TcpBus_bus *bus,
                                TcpBus_callback_error_t f)
                               __attribute__((nonnull(1,2)));
int TcpBus_callback_disconnect_remove(struct TcpBus_bus *bus,
                                      TcpBus_callback_disconnect_t f)
                                     __attribute__((nonnull(1,2)));



#ifdef __cplusplus
}
#endif

#endif // defined __LIBTCPBUS_H__
