#ifndef NET_H_
#define NET_H_
#include <sys/uio.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int net_tcp_listen(const char *ip/*NULL*/, unsigned short port);

int net_tcp_bind(const char *ip/*NULL*/, unsigned short port);

int net_tcp_connect(const char *host, unsigned short port);

int net_tcp_connect_nonblock(const char *host, unsigned short port);

int net_tcp_connect_sockaddr(const struct sockaddr *addr, socklen_t addrlen);

int net_tcp_connect_sockaddr_nonblock(const struct sockaddr *addr, socklen_t addrlen);


int net_unix_listen(const char *pathname);

int net_unix_bind(const char *pathname);

int net_unix_connect(const char *pathname);


int net_udp_bind(const char *ip/*NULL*/, unsigned short port);

int net_udp_connect(const char *host, unsigned short port);

int net_udp_connect_sockaddr(const struct sockaddr *addr, socklen_t addrlen);


int net_socketpair(int sv[2]);


int net_ip_sockaddr(const char *host, unsigned short port, struct sockaddr_in *addr);


/* Same as read(), except that it will restart read() when EINTR.
 */
ssize_t net_read_n(int fd, void *buf, size_t len);


/* Same as write(), except that it will restart write() when EINTR.
 */
ssize_t net_write_n(int fd, const void *buf, size_t len);



/*
 * 0 for success, -1 for error, -2 for end-of-file, -3 for timeout.
 */
int net_read_resid(int fd, void *buf, size_t *resid, int *timeout_ms/*NULL*/);

int net_readv_resid(int fd, struct iovec **iov, int *count, int *timeout_ms/*NULL*/);


int net_write_resid(int fd, const void *buf, size_t *resid, int *timeout_ms/*NULL*/);

int net_writev_resid(int fd, struct iovec **iov, int *count, int *timeout_ms/*NULL*/);



/* Non-block read. 
 * The return value has a different meanings than read() function.
 * 0 for would-block, -1 for error, -2 for end-of-file (read() returning 0).
 */ 
ssize_t net_read_nonblock(int fd, void *buf, size_t len);

ssize_t net_readv_nonblock(int fd, const struct iovec *iov, int count);


/* Non-block peek.
 * The return value: 
 * 0 for would-block, -1 for error, -2 for end-of-file (read() returning 0).
 */
ssize_t net_peek_nonblock(int fd, void *buf, size_t len);


/* Non-block write.
   The return value has a different meaning than write() function.
   0 for would-block, -1 for error, -2 for end-of-file (EPIPE).
 */
ssize_t net_write_nonblock(int fd, const void *buf, size_t len);

ssize_t net_writev_nonblock(int fd, const struct iovec *iov, int count);


int net_adjust_iovec(struct iovec **piov, int iov_count, size_t writen);



int net_get_so_error(int fd);


int net_set_tcp_nodelay(int fd);
int net_set_keepalive(int fd);

int net_set_send_buffer_size(int fd, int size);
int net_get_send_buffer_size(int fd);

int net_set_recv_buffer_size(int fd, int size);
int net_get_recv_buffer_size(int fd);

int net_set_reuse_address(int fd);
int net_clear_reuse_address(int fd);


int net_set_nonblock(int fd);
int net_set_block(int fd);

char* ipv4_ntoa(uint32_t ip, char str[]);

uint32_t ipv4_aton(const char *str);

bool ipv4_is_loopback(uint32_t ip);
bool ipv4_is_internal(uint32_t ip);
bool ipv4_is_external(uint32_t ip);

bool ipv4_is_loopback_s(const char *ip);
bool ipv4_is_internal_s(const char *ip);
bool ipv4_is_external_s(const char *ip);

int net_ipv4_get_all(uint32_t ips[], int num);

/* Return the strlen() of the ip, or 0 if no internal ip.
If 0 is returned, the ip[0] is set to '\0'.
*/
int net_get_internal_ip(char ip[]);;
int net_get_external_ip(char ip[]);

#ifdef __cplusplus
}
#endif

#endif
