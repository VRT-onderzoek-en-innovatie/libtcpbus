#include <config.h>
#include "../include/libtcpbus.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <ev.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>
#include <getopt.h>

#include "../Socket/Socket.hxx"

static const int MAX_CONN_BACKLOG = 32;

Socket s_listen;


void received_sigint(EV_P_ ev_signal *w, int revents) {
	fprintf(stderr, "Received SIGINT, exiting\n");
	ev_break(EV_A_ EVUNLOOP_ALL);
}
void received_sigterm(EV_P_ ev_signal *w, int revents) {
	fprintf(stderr, "Received SIGTERM, exiting\n");
	ev_break(EV_A_ EVUNLOOP_ALL);
}

void received_newcon(const struct TcpBus_bus *bus,
                     const struct sockaddr *addr, socklen_t addr_len) {
	std::auto_ptr<SockAddr::SockAddr> a(
		SockAddr::create(reinterpret_cast<const struct sockaddr_storage*>(addr))
	);

	fprintf(stderr, "new connection: %s\n", a->string().c_str());
}

void received_error(const struct TcpBus_bus *bus,
                    const struct sockaddr *addr, socklen_t addr_len, int err) {
	std::auto_ptr<SockAddr::SockAddr> a(
		SockAddr::create(reinterpret_cast<const struct sockaddr_storage*>(addr))
	);

	fprintf(stderr, "error in %s : %s\n", a->string().c_str(), strerror(err));
}

void received_disconnect(const struct TcpBus_bus *bus,
                         const struct sockaddr *addr, socklen_t addr_len) {
	std::auto_ptr<SockAddr::SockAddr> a(
		SockAddr::create(reinterpret_cast<const struct sockaddr_storage*>(addr))
	);

	fprintf(stderr, "disconnect: %s\n", a->string().c_str());
}


int main(int argc, char* argv[]) {
	fprintf(stderr, "%s version %s (%s) starting up\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_GITREVISION);

	// Default options
	struct {
		std::string bind_addr_listen;
	} options = {
		/* bind_addr_listen = */ "[127.0.0.1]:[10000]",
		};

	{ // Parse options
		char optstring[] = "hVfp:b:B:l:";
		struct option longopts[] = {
			{"help",      no_argument,       NULL, 'h'},
			{"version",   no_argument,       NULL, 'V'},
			{"bind",      required_argument, NULL, 'b'},
			{NULL, 0, 0, 0}
		};
		int longindex;
		int opt;
		while( (opt = getopt_long(argc, argv, optstring, longopts, &longindex)) != -1 ) {
			switch(opt) {
			case 'h':
			case '?':
				std::cerr <<
				//	>---------------------- Standard terminal width ---------------------------------<
					"Options:\n"
					"  -h --help                       Displays this help message and exits\n"
					"  -V --version                    Displays the version and exits\n"
					"  --bind -b host:port             Bind to the specified address for incomming\n"
					"                                  connections.\n"
					"                                  host and port resolving can be bypassed by\n"
					"                                  placing [] around them\n"
					;
				if( opt == '?' ) exit(EX_USAGE);
				exit(EX_OK);
			case 'V':
				printf("%1$s version %2$s\n"
				       " configured with: %3$s\n"
				       " CFLAGS=\"%4$s\" CXXFLAGS=\"%5$s\" CPPFLAGS=\"%6$s\"\n"
				       " Options:\n"
				       "   IPv6: %7$s\n"
				       "\n",
					 PACKAGE_NAME, PACKAGE_VERSION " (" PACKAGE_GITREVISION ")",
				         CONFIGURE_ARGS,
				         CFLAGS, CXXFLAGS, CPPFLAGS,
#ifdef ENABLE_IPV6
				         "yes"
#else
				         "no"
#endif
				         );
				exit(EX_OK);
			case 'b':
				options.bind_addr_listen = optarg;
				break;
			}
		}
	}

	{ // Open listening socket
		std::string host, port;

		/* Address format is
		 *   - hostname:portname
		 *   - [numeric ip]:portname
		 *   - hostname:[portnumber]
		 *   - [numeric ip]:[portnumber]
		 */
		size_t c = options.bind_addr_listen.rfind(":");
		if( c == std::string::npos ) {
			/* TRANSLATORS: %1$s contains the string passed as option
			 */
			fprintf(stderr, "Invalid bind string \"%1$s\": could not find ':'\n", options.bind_addr_listen.c_str());
			exit(EX_DATAERR);
		}
		host = options.bind_addr_listen.substr(0, c);
		port = options.bind_addr_listen.substr(c+1);

		std::auto_ptr< boost::ptr_vector< SockAddr::SockAddr> > bind_sa
			= SockAddr::resolve( host, port, 0, SOCK_STREAM, 0);
		if( bind_sa->size() == 0 ) {
			fprintf(stderr, "Can not bind to \"%1$s\": Could not resolve\n", options.bind_addr_listen.c_str());
			exit(EX_DATAERR);
		} else if( bind_sa->size() > 1 ) {
			// TODO: allow this
			fprintf(stderr, "Can not bind to \"%1$s\": Resolves to multiple entries:\n", options.bind_addr_listen.c_str());
			for( typeof(bind_sa->begin()) i = bind_sa->begin(); i != bind_sa->end(); i++ ) {
				std::cerr << "  " << i->string() << "\n";
			}
			exit(EX_DATAERR);
		}

		s_listen = Socket::socket( (*bind_sa)[0].proto_family() , SOCK_STREAM, 0);
		s_listen.set_reuseaddr();
		s_listen.bind((*bind_sa)[0]);
		s_listen.listen(MAX_CONN_BACKLOG);

		std::auto_ptr<SockAddr::SockAddr> bound_addr( s_listen.getsockname() );
		fprintf(stderr, "Listening on %s\n", bound_addr->string().c_str());
	}

	{
		struct sigaction act;
		if( sigaction(SIGPIPE, NULL, &act) == -1) {
			fprintf(stderr, "sigaction() failed: %s", strerror(errno));
			return -1;
		}
		act.sa_handler = SIG_IGN; // Ignore SIGPIPE (we'll handle the write()-error)
		if( sigaction(SIGPIPE, &act, NULL) == -1 ) {
			fprintf(stderr, "sigaction() failed: %s", strerror(errno));
			return -1;
		}
	}

	struct TcpBus_bus *bus;
	{
		ev_signal ev_sigint_watcher;
		ev_signal_init( &ev_sigint_watcher, received_sigint, SIGINT);
		ev_signal_start( EV_DEFAULT_ &ev_sigint_watcher);
		ev_signal ev_sigterm_watcher;
		ev_signal_init( &ev_sigterm_watcher, received_sigterm, SIGTERM);
		ev_signal_start( EV_DEFAULT_ &ev_sigterm_watcher);

		bus = TcpBus_init(EV_DEFAULT_ s_listen);
		TcpBus_callback_newcon_add(bus, received_newcon);
		TcpBus_callback_error_add(bus, received_error);
		TcpBus_callback_disconnect_add(bus, received_disconnect);

		fprintf(stderr, "Setup done, starting event loop\n");

		ev_run(EV_DEFAULT_ 0);
	}

	TcpBus_terminate(bus);

	fprintf(stderr, "Cleaning up\n");

	return 0;
}
