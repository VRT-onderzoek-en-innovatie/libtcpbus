#ifndef __LIBTCPBUS_H__
#define __LIBTCPBUS_H__

#include <ev.h>

#ifdef __cplusplus
extern "C"{
#endif

/* initialize the event listeners
 * socket should be set up in listening state
 */
void TcpBus_init(EV_P_ int socket);

/* Callback function prototypes
 */
typedef void (*TcpBus_rx_callback_t)(const char *data, size_t len);

/* Register an (additional) callback function to be called when receiving
 * data.
 *
 * Return an opaque handle to this callback,
 * or -1 on failure
 */
int TcpBus_rx_callback(TcpBus_rx_callback_t f);

/* Send data to the bus
 * returns -1 on failure
 */
int TcpBus_send(const char *data, size_t len);


#ifdef __cplusplus
}
#endif

#endif // defined __LIBTCPBUS_H__
