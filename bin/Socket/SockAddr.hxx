#ifndef __SOCKADDR_HPP__
#define __SOCKADDR_HPP__

#include "../config.h"
#include <netinet/in.h>
#include <memory>
#include <string>
#include <boost/ptr_container/ptr_vector.hpp>

#include "Errno.hxx"

namespace SockAddr {

class SockAddr {
protected:
	SockAddr() throw() {}

public:
	virtual ~SockAddr() throw() {};

	virtual operator struct sockaddr const*() const throw() =0;
	virtual socklen_t const addr_len() const throw() =0;

	virtual std::string string() const throw(std::runtime_error) = 0;

	virtual int const proto_family() const throw() =0;
	virtual int const addr_family() const throw() =0;
};

std::auto_ptr<SockAddr> create(const struct sockaddr *addr) throw(std::invalid_argument);
inline std::auto_ptr<SockAddr> create(const struct sockaddr_in *addr) throw(std::invalid_argument) {
	return create( reinterpret_cast<const struct sockaddr *>(addr) );
}
#ifdef ENABLE_IPV6
inline std::auto_ptr<SockAddr> create(const struct sockaddr_in6 *addr) throw(std::invalid_argument) {
	return create( reinterpret_cast<const struct sockaddr *>(addr) );
}
#endif

std::auto_ptr<SockAddr> translate(std::string const &host, unsigned short const port) throw(std::invalid_argument);

std::auto_ptr< boost::ptr_vector< SockAddr > > resolve(std::string const &host, std::string const &service, int const family = 0, int const socktype = 0, int const protocol = 0, bool const v4_mapped = false);


class Inet4 : public SockAddr {
protected:
	struct sockaddr_in m_addr;

public:
	Inet4(struct sockaddr_in const &addr) throw();
	virtual ~Inet4() throw() {};

	socklen_t const addr_len() const throw() { return sizeof(m_addr); }

	virtual operator struct sockaddr const*() const throw() { return reinterpret_cast<struct sockaddr const*>(&m_addr); }

	virtual std::string string() const throw(Errno);

	virtual int const proto_family() const throw() { return PF_INET; }
	virtual int const addr_family() const throw() { return AF_INET; }
};


#ifdef ENABLE_IPV6
class Inet6 : public SockAddr {
protected:
	struct sockaddr_in6 m_addr;

public:
	Inet6(struct sockaddr_in6 const &addr) throw();
	virtual ~Inet6() throw() {}

	socklen_t const addr_len() const throw() { return sizeof(m_addr); }

	virtual operator struct sockaddr const*() const throw() { return reinterpret_cast<struct sockaddr const*>(&m_addr); }

	virtual std::string string() const throw(std::runtime_error);

	virtual int const proto_family() const throw() { return PF_INET6; }
	virtual int const addr_family() const throw() { return AF_INET6; }
};
#endif

} // namespace

#endif // __SOCKADDR_HPP__
