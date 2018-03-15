#include "net.h"
#include "msec.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <endian.h>
#include <poll.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define LISTEN_QUEUE_SIZE	1024

int net_tcp_listen(const char *ip, unsigned short port){
	int sock = net_tcp_bind(ip, port);
	if (sock >= 0){
		if (listen(sock, LISTEN_QUEUE_SIZE) == -1){
			close(sock);
			return -1;
		}
	}
	return sock;
}

int net_unix_listen(const char *pathname){
	int sock = net_unix_bind(pathname);
	if (sock >= 0){
		if (listen(sock, LISTEN_QUEUE_SIZE) == -1){
			close(sock);
			return -1;
		}
	}
	return sock;
}

int net_tcp_bind(const char *ip, unsigned short port){
        struct sockaddr_in addr;
	int sock = -1;
        int on = 1;

	if (ip == NULL || ip[0] == 0)
		ip = "0.0.0.0";

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &addr.sin_addr);
        addr.sin_port = htons(port);

        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1)
		goto error;

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		goto error;

	return sock;
error:
	if (sock != -1)
		close(sock);
	return -1;
}

int net_udp_bind(const char *ip, unsigned short port){
        struct sockaddr_in addr;
	int sock = -1;

	if (ip == NULL || ip[0] == 0)
		ip = "0.0.0.0";

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &addr.sin_addr);
        addr.sin_port = htons(port);

        sock = socket(PF_INET, SOCK_DGRAM, 0);
        if (sock == -1)
		goto error;

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		goto error;

	return sock;
error:
	if (sock != -1)
		close(sock);
	return -1;
}

int net_udp_connect_sockaddr(const struct sockaddr *addr, socklen_t addrlen){
	int sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1)
		goto error;

	if (connect(sock, addr, addrlen) == -1){
		goto error;
	}

	return sock;
error:
	if (sock != -1)
		close(sock);
	return -1;
}

int net_udp_connect(const char *host, unsigned short port){
	struct sockaddr_in addr;

	if (net_ip_sockaddr(host, port, &addr) < 0)
		return -1;

	return net_udp_connect_sockaddr((struct sockaddr *)&addr, sizeof(addr));
}

int net_socketpair(int sv[2]){
	return socketpair(AF_LOCAL, SOCK_STREAM, 0, sv);
}

int net_ip_sockaddr(const char *host, unsigned short port, struct sockaddr_in *addr){
	int errcode = 0;

	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);

	if (host && host[0]){
		struct in_addr ipaddr;

		if (inet_aton(host, &ipaddr)){
			addr->sin_addr = ipaddr;
		}
		else if (strcasecmp(host, "localhost") == 0){
			addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		}
		else{
			struct addrinfo hints = {0}, *res = NULL;

			/* Don't use gethostbyname(), it's not thread-safe. 
			   Use getaddrinfo() instead.
			 */
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			errcode = getaddrinfo(host, NULL, &hints, &res);
			if (errcode != 0){
				return -1;
			}
			else{
				assert(res->ai_family == AF_INET && res->ai_socktype == SOCK_STREAM);
				assert(res->ai_addrlen == sizeof(*addr));

				memcpy(addr, (struct sockaddr_in *)res->ai_addr, sizeof(*addr));
			}

			if (res)
				freeaddrinfo(res);
		}
	}

	addr->sin_port = htons(port);
	return errcode ? -1 : 0;
}

static int _tcp_connect_sockaddr(const struct sockaddr *addr, socklen_t addrlen, int nonblock){
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		goto error;

	if (nonblock){
		int flags = fcntl(sock, F_GETFL, 0);
		if (flags == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
			goto error;
	}

	if (connect(sock, addr, addrlen) == -1){
		if (!nonblock || errno != EINPROGRESS)
			goto error;
	}

	return sock;
error:
	if (sock != -1)
		close(sock);
	return -1;
}

int net_tcp_connect_sockaddr(const struct sockaddr *addr, socklen_t addrlen){
	return _tcp_connect_sockaddr(addr, addrlen, 0);
}

int net_tcp_connect_sockaddr_nonblock(const struct sockaddr *addr, socklen_t addrlen){
	return _tcp_connect_sockaddr(addr, addrlen, 1);
}

int net_tcp_connect(const char *host, unsigned short port){
	struct sockaddr_in addr;

	if (net_ip_sockaddr(host, port, &addr) < 0)
		return -1;

	return _tcp_connect_sockaddr((struct sockaddr *)&addr, sizeof(addr), 0);
}

int net_tcp_connect_nonblock(const char *host, unsigned short port){
	struct sockaddr_in addr;

	if (net_ip_sockaddr(host, port, &addr) < 0)
		return -1;

	return _tcp_connect_sockaddr((struct sockaddr *)&addr, sizeof(addr), 1);
}

int net_unix_bind(const char *pathname){
	struct sockaddr_un addr;
	int sock = -1;
	mode_t mode = 0;

	sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock == -1)
		goto error;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, pathname, sizeof(addr.sun_path) - 1);

	unlink(pathname);
	mode = umask(0);

	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		goto error;

	umask(mode);
	return sock;
error:
	if (sock != -1)
		close(sock);
	if (mode)
		umask(mode);
	return -1;
}

int net_unix_connect(const char *pathname){
	struct sockaddr_un addr;
	int sock = -1;

	sock = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (sock == -1)
		goto error;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, pathname, sizeof(addr.sun_path) - 1);

	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		goto error;

	return sock;
error:
	if (sock != -1)
		close(sock);
	return -1;
}

ssize_t net_read_n(int fd, void *buf, size_t len){
	ssize_t cur = 0;

	if ((ssize_t)len <= 0)
		return len;

	do {
		ssize_t need = len - cur;
		ssize_t n = read(fd, (char *)buf + cur, need);
		if (n == 0)
			break;
		if (n == -1){
			if (errno == EINTR)
				n = 0;
			else
				return -1;
		}
		cur += n;
	} while (cur < (ssize_t)len);
	return cur;
}

ssize_t net_write_n(int fd, const void *buf, size_t len){
	ssize_t cur = 0;

	if ((ssize_t)len <= 0)
		return len;

	do {
		ssize_t need = len - cur;
		ssize_t n = write(fd, (char *)buf + cur, need);
		if (n == -1){
			if (errno == EINTR)
				n = 0;
			else
				return -1;
		}
		cur += n;
	} while (cur < (ssize_t)len);
	return cur;
}


int net_read_resid(int fd, void *buf, size_t *resid, int *timeout_ms){
	int ret = 0;
	ssize_t n;

	if ((ssize_t)*resid <= 0)
		return ret;

	n = net_read_nonblock(fd, buf, *resid);
	if (n < 0)
		return n;

	*resid -= n;
	if (*resid > 0){
		uint64_t start;
		int timeout = timeout_ms ? *timeout_ms : -1;
		if (timeout == 0)
			return -3;

		start = timeout > 0 ? exact_mono_msec() : 0;
		buf = (char *)buf + n;
		while (1){
			int rc;
			struct pollfd fds[1];
			fds[0].fd = fd;
			fds[0].events = POLLIN | POLLPRI;

			rc = poll(fds, 1, timeout);
			if (rc < 0 && errno != EINTR){
				ret = -1;
				goto done;
			}

			if (rc > 0){
				n = net_read_nonblock(fd, buf, *resid);
				if (n < 0){
					ret = n;
					goto done;
				}

				if (n > 0){
					*resid -= n;
					if (*resid == 0)
						goto done;
					buf = (char *)buf + n;
				}
			}

			if (timeout > 0){
				uint64_t now = exact_mono_msec();
				timeout -= (now - start);
				if (timeout <= 0){
					*timeout_ms = 0;
					return -3;
				}
				start = now;
			}
		}
	done:
		if (timeout > 0){
			uint64_t now = exact_mono_msec();
			timeout -= (now - start);
			*timeout_ms = timeout > 0 ? timeout : 0;
		}
	}

	return ret;
}

int net_write_resid(int fd, const void *buf, size_t *resid, int *timeout_ms){
	int ret = 0;
	ssize_t n;

	if ((ssize_t)*resid <= 0)
		return ret;

	n = net_write_nonblock(fd, buf, *resid);
	if (n < 0)
		return n;

	*resid -= n;
	if (*resid > 0){
		uint64_t start;
		int timeout = timeout_ms ? *timeout_ms : -1;
		if (timeout == 0)
			return -3;

		buf = (char *)buf + n;
		start = timeout > 0 ? exact_mono_msec() : 0;
		while (1){
			int rc;
			struct pollfd fds[1];
			fds[0].fd = fd;
			fds[0].events = POLLOUT | POLLHUP;

			rc = poll(fds, 1, timeout);
			if (rc < 0 && errno != EINTR){
				ret = -1;
				goto done;
			}

			if (rc > 0){
				n = net_write_nonblock(fd, buf, *resid);
				if (n < 0){
					ret = n;
					goto done;
				}

				if (n > 0){
					*resid -= n;
					if (*resid == 0)
						goto done;
					buf = (char *)buf + n;
				}
			}

			if (timeout > 0){
				uint64_t now = exact_mono_msec();
				timeout -= (now - start);
				if (timeout <= 0){
					*timeout_ms = 0;
					return -3;
				}
				start = now;
			}
		}
	done:
		if (timeout > 0){
			uint64_t now = exact_mono_msec();
			timeout -= (now - start);
			*timeout_ms = timeout > 0 ? timeout : 0;
		}
	}

	return ret;
}

int net_readv_resid(int fd, struct iovec **iov, int *count, int *timeout_ms){
	int ret = 0;
	ssize_t n;

	if (*count <= 0)
		return ret;

	n = net_readv_nonblock(fd, *iov, *count);
	if (n < 0)
		return n;

	if (n > 0)
		*count = net_adjust_iovec(iov, *count, n);

	if (*count > 0){
		uint64_t start;
		int timeout = timeout_ms ? *timeout_ms : -1;
		if (timeout == 0)
			return -3;

		start = timeout > 0 ? exact_mono_msec() : 0;
		while (1){
			int rc;
			struct pollfd fds[1];
			fds[0].fd = fd;
			fds[0].events = POLLIN | POLLPRI;

			rc = poll(fds, 1, timeout);
			if (rc < 0 && errno != EINTR){
				ret = -1;
				goto done;
			}

			if (rc > 0){
				n = net_readv_nonblock(fd, *iov, *count);
				if (n < 0){
					ret = n;
					goto done;
				}

				if (n > 0){
					*count = net_adjust_iovec(iov, *count, n);
					if (*count == 0)
						goto done;
				}
			}

			if (timeout > 0){
				uint64_t now = exact_mono_msec();
				timeout -= (now - start);
				if (timeout <= 0){
					*timeout_ms = 0;
					return -3;
				}
				start = now;
			}
		}
	done:
		if (timeout > 0){
			uint64_t now = exact_mono_msec();
			timeout -= (now - start);
			*timeout_ms = timeout > 0 ? timeout : 0;
		}
	}

	return ret;
}

int net_writev_resid(int fd, struct iovec **iov, int *count, int *timeout_ms){
	int ret = 0;
	ssize_t n;

	if (*count <= 0)
		return ret;

	n = net_writev_nonblock(fd, *iov, *count);
	if (n < 0)
		return n;

	if (n > 0)
		*count = net_adjust_iovec(iov, *count, n);

	if (*count > 0){
		uint64_t start;
		int timeout = timeout_ms ? *timeout_ms : -1;
		if (timeout == 0)
			return -3;

		start = timeout > 0 ? exact_mono_msec() : 0;
		while (1){
			int rc;
			struct pollfd fds[1];
			fds[0].fd = fd;
			fds[0].events = POLLOUT | POLLHUP;

			rc = poll(fds, 1, timeout);
			if (rc < 0 && errno != EINTR){
				ret = -1;
				goto done;
			}

			if (rc > 0){
				n = net_writev_nonblock(fd, *iov, *count);
				if (n < 0){
					ret = n;
					goto done;
				}

				if (n > 0){
					*count = net_adjust_iovec(iov, *count, n);
					if (*count == 0)
						goto done;
				}
			}

			if (timeout > 0){
				uint64_t now = exact_mono_msec();
				timeout -= (now - start);
				if (timeout <= 0){
					*timeout_ms = 0;
					return -3;
				}
				start = now;
			}
		}
	done:
		if (timeout > 0){
			uint64_t now = exact_mono_msec();
			timeout -= (now - start);
			*timeout_ms = timeout > 0 ? timeout : 0;
		}
	}

	return ret;
}

ssize_t net_read_nonblock(int fd, void *buf, size_t len){
    ssize_t n;
again:
    n = read(fd, buf, len);
    if (n == -1){
        switch (errno){
        case EINTR: goto again; break;
        case EAGAIN: n = 0; break;
        }
    }
    else if (n == 0)
        return -2;
    return n;
}

ssize_t net_readv_nonblock(int fd, const struct iovec *vec, int count){
    ssize_t n;
again:
    n = readv(fd, vec, count);
    if (n == -1){
        switch (errno){
        case EINTR: goto again; break;
        case EAGAIN: n = 0; break;
        case EPIPE: return -2; break;
        }
    }
    return n;
}

ssize_t net_peek_nonblock(int fd, void *buf, size_t len){
    ssize_t n;
again:
    n = recv(fd, buf, len, MSG_PEEK | MSG_DONTWAIT);
    if (n == -1){
        switch (errno){
        case EINTR: goto again; break;
        case EAGAIN: n = 0; break;
        }
    }
    else if (n == 0)
        return -2;
    return n;
}

ssize_t net_write_nonblock(int fd, const void *buf, size_t len){
    ssize_t n;
again:
    n = write(fd, buf, len);
    if (n == -1){
        switch (errno){
        case EINTR: goto again; break;
        case EAGAIN: n = 0; break;
        case EPIPE: return -2; break;
        }
    }
    return n;
}

ssize_t net_writev_nonblock(int fd, const struct iovec *vec, int count){
    ssize_t n;
again:
    n = writev(fd, vec, count);
    if (n == -1){
        switch (errno){
        case EINTR: goto again; break;
        case EAGAIN: n = 0; break;
        case EPIPE: return -2; break;
        }
    }
    return n;
}

int net_adjust_iovec(struct iovec **piov, int iov_count, size_t writen){
    if ((ssize_t)writen > 0){
        ssize_t n = 0;
        int i;

        for (i = 0; i < iov_count; ++i){
            ssize_t k = n + (*piov)[i].iov_len;
            if (k > (ssize_t)writen)
                break;
            n = k;
        }

        *piov += i;
        iov_count -= i;
        if (iov_count){
            ssize_t k = writen - n;
            (*piov)->iov_base = (char *)(*piov)->iov_base + k;
            (*piov)->iov_len -= k;
        }
    }
    return iov_count;
}

int net_get_so_error(int fd){
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) == -1)
        return -1;
    return so_error;
}

int net_set_tcp_nodelay(int fd){
    int flag = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

int net_set_keepalive(int fd){
    int flag = 1;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
}

int net_set_send_buffer_size(int fd, int size){
    return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

int net_get_send_buffer_size(int fd){
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, &len) == 0 && (size_t)len == sizeof(size))
        return size;
    return -1;
}

int net_set_recv_buffer_size(int fd, int size){
    return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

int net_get_recv_buffer_size(int fd){
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, &len) == 0 && (size_t)len == sizeof(size))
        return size;
    return -1;
}

int net_set_reuse_address(int fd){
    int flag = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

int net_clear_reuse_address(int fd){
    int flag = 0;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
}

int net_set_nonblock(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int net_set_block(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

inline char* ipv4_ntoa(uint32_t ip, char str[]){
    int b0 = (ip>>24);
    int b1 = (ip>>16) & 0xFF;
    int b2 = (ip>>8) & 0xFF;
    int b3 = ip & 0xFF;
    sprintf(str, "%d.%d.%d.%d", b0, b1, b2, b3);
	return str;
}

inline uint32_t ipv4_aton(const char *str){
    struct in_addr addr;

    if (inet_aton(str, &addr)){
        return ntohl(addr.s_addr);
    }

    return 0;
}

inline bool ipv4_is_loopback(uint32_t ip){
    return ((ip >> 24) == 127);
}

inline bool ipv4_is_internal(uint32_t ip){
    int b0, b1;

    b0 = (ip >> 24);
    if (b0 == 10)
        return true;

    b1 = (ip >> 16) & 0xFF;
    if (b0 == 172){
        if (b1 >= 16 && b1 < 32)
            return true;
    }
    else if (b0 == 192){
        if (b1 == 168)
            return true;
    }
    return false;
}

inline bool ipv4_is_external(uint32_t ip){
    return !ipv4_is_loopback(ip) && !ipv4_is_internal(ip);
}

inline bool ipv4_is_loopback_s(const char *ip){
    return ipv4_is_loopback(ipv4_aton(ip));
}

inline bool ipv4_is_internal_s(const char *ip){
    return ipv4_is_internal(ipv4_aton(ip));
}

inline bool ipv4_is_external_s(const char *ip){
    return ipv4_is_external(ipv4_aton(ip));
}

/* XXX: this has only tested on Linux */
int net_ipv4_get_all(uint32_t ips[], int num)
{
    struct ifconf ifc;
    struct ifreq *ifr, ifrcopy;
    int sock;
    int len, lastlen;
    char *buf, *ptr;
    char lastname[IFNAMSIZ];
    struct sockaddr_in *sinptr;
    int flags;
    int k = 0;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;
    lastlen = 0;
    len = 100 * sizeof(struct ifreq);
    buf = (char *)malloc(len);
	for (;;)
    {
        ifc.ifc_len = len;
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
        {
            if (errno != EINVAL || lastlen != 0)
            {
                free(buf);
                close(sock);
                return -1;
            }
        }
        else
        {
            if (ifc.ifc_len == lastlen)
                break;
            lastlen = ifc.ifc_len;
        }
        len += 10 * sizeof(struct ifreq);
        buf = (char *)realloc(buf, len);
    }
	for (ptr = buf; ptr < buf + ifc.ifc_len; )
    {
        char *cptr;
        int b0;

        ifr = (struct ifreq *)ptr;
/*
        switch (ifr->ifr_addr.sa_family)
        {
        case AF_INET6:
            len = sizeof(struct sockaddr_in6);
            break;
        case AF_INET:
        default:
            len = sizeof(struct sockaddr);
            break;
        }
        ptr += sizeof(ifr->ifr_name) + len;
*/

		ptr += sizeof(struct ifreq);
        if (ifr->ifr_addr.sa_family != AF_INET)
            continue;

        if ((cptr = strchr(ifr->ifr_name, ':')) != NULL)
            *cptr = '\0';
        if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)
            continue;
        memcpy(lastname, ifr->ifr_name, IFNAMSIZ);
        ifrcopy = *ifr;
        ioctl(sock, SIOCGIFFLAGS, &ifrcopy);
        flags = ifrcopy.ifr_flags;
        if ((flags & IFF_UP) == 0)
            continue;

        sinptr = (struct sockaddr_in *)&ifr->ifr_addr;
        b0 = *(unsigned char *)&sinptr->sin_addr.s_addr;
        if (b0 == 127)
            continue;

        if (k < num)
            ips[k] = ntohl(sinptr->sin_addr.s_addr);
        ++k;
    }
    free(buf);
    close(sock);
    return k;
}

int net_get_internal_ip(char ip[])
{
    uint32_t ips[256];
    int n = net_ipv4_get_all(ips, 256);
    if (n > 0)
    {
        int i;
        int num = (n < 256) ? n : 256;
        for (i = 0; i < num; ++i)
        {
            if (ipv4_is_internal(ips[i])){
				ipv4_ntoa(ips[i], ip);
				return 0;
			}
        }
    }

    ip[0] = 0;
    return 0;
}

int net_get_external_ip(char ip[])
{
    uint32_t ips[256];
    int n = net_ipv4_get_all(ips, 256);
    if (n > 0)
    {
        int i;
        int num = (n < 256) ? n : 256;
        for (i = 0; i < num; ++i)
        {
            if (!ipv4_is_internal(ips[i])){
				ipv4_ntoa(ips[i], ip);
				return 0;
			}
        }
    }

    ip[0] = 0;
    return 0;
}
