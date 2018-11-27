/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jerryscript-debugger-transport.h"
#include "jerryscript-ext/debugger.h"
#include "jext-common.h"

#ifdef JERRY_BT_DEBUGGER

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

/**
 * Implementation of transport over bluetooth.
 */
typedef struct
{
  jerry_debugger_transport_header_t header; /**< transport header */
  int bluetooth_socket; /**< bluetooth socket */
} jerryx_debugger_transport_bluetooth_t;

/**
 * Log bluetooth error message.
 */
static void
jerryx_debugger_bt_log_error (int err_val)
{
  JERRYX_ERROR_MSG ("bluetooth Error: %s\n", strerror (err_val));
} /* jerryx_debugger_bt_log_error */

/**
 * Close a bluetooth connection.
 */
static void
jerryx_debugger_bt_close (jerry_debugger_transport_header_t *header_p) /**< bluetooth implementation */
{
  JERRYX_ASSERT (!jerry_debugger_transport_is_connected ());

  jerryx_debugger_transport_bluetooth_t *bluetooth_p = (jerryx_debugger_transport_bluetooth_t *) header_p;

  JERRYX_DEBUG_MSG ("bluetooth connection closed\n");

  close (bluetooth_p->bluetooth_socket);

  jerry_heap_free ((void *) header_p, sizeof (jerryx_debugger_transport_bluetooth_t));
} /* jerryx_debugger_bt_close */

/**
 * Send data over a bluetooth connection.
 *
 * @return true - if the data has been sent successfully
 *         false - otherwise
 */
static bool
jerryx_debugger_bt_send (jerry_debugger_transport_header_t *header_p, /**< bluetooth implementation */
                         uint8_t *message_p, /**< message to be sent */
                         size_t message_length) /**< message length in bytes */
{
  JERRYX_ASSERT (jerry_debugger_transport_is_connected ());

  jerryx_debugger_transport_bluetooth_t *bluetooth_p = (jerryx_debugger_transport_bluetooth_t *) header_p;

  do
  {
    ssize_t is_err = recv (bluetooth_p->bluetooth_socket, NULL, 0, MSG_PEEK);

    if (is_err == 0 && errno != EWOULDBLOCK)
    {
      int err_val = errno;
      jerry_debugger_transport_close ();
      jerryx_debugger_bt_log_error (err_val);
      return false;
    }
    ssize_t sent_bytes = send (bluetooth_p->bluetooth_socket, message_p, message_length, 0);

    if (sent_bytes < 0)
    {
      if (errno == EWOULDBLOCK)
      {
        continue;
      }

      int err_val = errno;
      jerry_debugger_transport_close ();
      jerryx_debugger_bt_log_error (err_val);
      return false;
    }

    message_p += sent_bytes;
    message_length -= (size_t) sent_bytes;
  }
  while (message_length > 0);

  return true;
} /* jerryx_debugger_bt_send */

/**
 * Receive data from a bluetooth connection.
 */
static bool
jerryx_debugger_bt_receive (jerry_debugger_transport_header_t *header_p, /**< bluetooth implementation */
                            jerry_debugger_transport_receive_context_t  *receive_context_p) /**< receive context */
{
  jerryx_debugger_transport_bluetooth_t *bluetooth_p = (jerryx_debugger_transport_bluetooth_t *) header_p;

  uint8_t *buffer_p = receive_context_p->buffer_p + receive_context_p->received_length;
  size_t buffer_size = JERRY_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE - receive_context_p->received_length;

  ssize_t length = recv (bluetooth_p->bluetooth_socket, buffer_p, buffer_size, 0);

  if (length <= 0)
  {
    if (errno != EWOULDBLOCK || length == 0)
    {
      int err_val = errno;
      jerry_debugger_transport_close ();
      jerryx_debugger_bt_log_error (err_val);
      return false;
    }
    length = 0;
  }

  receive_context_p->received_length += (size_t) length;

  if (receive_context_p->received_length > 0)
  {
    receive_context_p->message_p = receive_context_p->buffer_p;
    receive_context_p->message_length = receive_context_p->received_length;
  }

  return true;
} /* jerryx_debugger_bt_receive */

/**
 * Create a bluetooth connection.
 *
 * @return true if successful,
 *         false otherwise
 */
bool
jerryx_debugger_bt_create (uint16_t port) /**< listening port */
{
  struct sockaddr_rc loc_addr;
  int server_socket;
  char buf[1024] = { 0 };
  socklen_t bluetooth_size = sizeof (struct sockaddr_rc);

  loc_addr.rc_family = AF_BLUETOOTH;
  loc_addr.rc_bdaddr = *BDADDR_ANY;
  loc_addr.rc_channel = (uint8_t) port;

  if ((server_socket = socket (AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM)) == -1)
  {
    jerryx_debugger_bt_log_error (errno);
    return false;
  }

  int opt_value = 1;

  if (setsockopt (server_socket, SOL_SOCKET, SO_REUSEADDR, &opt_value, sizeof (int)) == -1)
  {
    close (server_socket);
    jerryx_debugger_bt_log_error (errno);
    return false;
  }

  if (bind (server_socket, (struct sockaddr *)&loc_addr, sizeof (loc_addr)) == -1)
  {
    close (server_socket);
    jerryx_debugger_bt_log_error (errno);
    return false;
  }

  if (listen (server_socket, port) == -1)
  {
    close (server_socket);
    jerryx_debugger_bt_log_error (errno);
    return false;
  }

  JERRYX_DEBUG_MSG ("Waiting for client connection\n");

  int bluetooth_socket = accept (server_socket, (struct sockaddr *)&loc_addr, &bluetooth_size);

  close (server_socket);

  if (bluetooth_socket == -1)
  {
    jerryx_debugger_bt_log_error (errno);
    return false;
  }

  /* Set non-blocking mode. */
  int socket_flags = fcntl (bluetooth_socket, F_GETFL, 0);
  if (socket_flags < 0)
  {
    close (bluetooth_socket);
    return false;
  }

  if (fcntl (bluetooth_socket, F_SETFL, socket_flags | O_NONBLOCK) == -1)
  {
    close (bluetooth_socket);
    return false;
  }
  ba2str (&loc_addr.rc_bdaddr, buf);
  JERRYX_DEBUG_MSG ("Connected from: %s\n",buf);
  size_t size = sizeof (jerryx_debugger_transport_bluetooth_t);
  jerry_debugger_transport_header_t *header_p;
  header_p = (jerry_debugger_transport_header_t *) jerry_heap_alloc (size);

  if (!header_p)
  {
    close (bluetooth_socket);
    return false;
  }

  header_p->close = jerryx_debugger_bt_close;
  header_p->send = jerryx_debugger_bt_send;
  header_p->receive = jerryx_debugger_bt_receive;

  ((jerryx_debugger_transport_bluetooth_t *) header_p)->bluetooth_socket = bluetooth_socket;

  jerry_debugger_transport_add (header_p,
                                0,
                                JERRY_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE,
                                0,
                                JERRY_DEBUGGER_TRANSPORT_MAX_BUFFER_SIZE);

  return true;

} /* jerryx_debugger_bt_create */

#else /* !JERRY_BT_DEBUGGER */
/**
 * Dummy function when bluetooth debugger is disabled.
 *
 * @return false
 */
bool
jerryx_debugger_bt_create (uint16_t port)
{
  JERRYX_ERROR_MSG ("support for Bluetooth debugging is disabled.\n");
  JERRYX_UNUSED (port);
  return false;
} /* jerryx_debugger_bt_create */

#endif /* JERRY_BT_DEBUGGER */
