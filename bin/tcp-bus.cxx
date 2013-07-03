#include <config.h>

#include <string>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysexits.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdexcept>
#include <boost/ptr_container/ptr_list.hpp>
#include <ev.h>
#include <memory>
#include <typeinfo>
#include <assert.h>
#include <signal.h>
#include <sstream>

#include <liblog.h>
#include <libtcpbus.h>
#include "Socket/Socket.hxx"

static const int MAX_CONN_BACKLOG = 32;

std::string logfilename;
FILE *logfile;
Socket s_listen;



void received_sigint(EV_P_ ev_signal *w, int revents) throw() {
	LogInfo("Received SIGINT, exiting");
	ev_break(EV_A_ EVUNLOOP_ALL);
}
void received_sigterm(EV_P_ ev_signal *w, int revents) throw() {
	LogInfo("Received SIGTERM, exiting");
	ev_break(EV_A_ EVUNLOOP_ALL);
}

void received_sighup(EV_P_ ev_signal *w, int revents) throw() {
	LogInfo("Received SIGHUP, closing this logfile");
	if( logfilename.size() > 0 ) {
		fclose(logfile);
		logfile = fopen(logfilename.c_str(), "a");
		LogSetOutputFile(NULL, logfile);
	} /* else we're still logging to stderr, which doesn't need reopening */
	LogInfo("Received SIGHUP, (re)opening this logfile");
}


void received_newcon(const struct TcpBus_bus *bus,
                     const struct sockaddr *addr, socklen_t addr_len) {
	try {
		std::auto_ptr<SockAddr::SockAddr> a( SockAddr::create(addr) );
		LogInfo("%s : new connection", a->string().c_str());
	} catch( std::invalid_argument &e ) {
		LogWarn("UNPARSABLE ADDRESS : new connection");
	}
}

void received_error(const struct TcpBus_bus *bus,
                    const struct sockaddr *addr, socklen_t addr_len, int err) {
	try {
		std::auto_ptr<SockAddr::SockAddr> a( SockAddr::create(addr) );
		LogInfo("%s : error: %s", a->string().c_str(), strerror(err));
	} catch( std::invalid_argument &e ) {
		LogWarn("UNPARSABLE ADDRESS : error: %s", strerror(err));
	}
}

void received_disconnect(const struct TcpBus_bus *bus,
                         const struct sockaddr *addr, socklen_t addr_len) {
	try {
		std::auto_ptr<SockAddr::SockAddr> a( SockAddr::create(addr) );
		LogInfo("%s : disconnect", a->string().c_str());
	} catch( std::invalid_argument &e ) {
		LogWarn("UNPARSABLE ADDRESS : disconnect");
	}
}

int main(int argc, char* argv[]) {
	// Defaults
	struct {
		bool fork;
		std::string pid_file;
		std::string bind_addr;
	} options = {
		/* fork = */ true,
		/* pid_file = */ "",
#ifdef ENABLE_IPV6
		/* bind_addr = */ "[::1]:[10000]"
#else
		/* bind_addr = */ "[127.0.0.1]:[10000]"
#endif
		};

	{ // Parse options
		char optstring[] = "hVfp:s:b:l:";
		struct option longopts[] = {
			{"help",			no_argument, NULL, 'h'},
			{"version",			no_argument, NULL, 'V'},
			{"forgeground",		no_argument, NULL, 'f'},
			{"pid-file",		required_argument, NULL, 'p'},
			{"bind",			required_argument, NULL, 'b'},
			{"log",				required_argument, NULL, 'l'},
			{NULL, 0, 0, 0}
		};
		int longindex;
		int opt;
		while( (opt = getopt_long(argc, argv, optstring, longopts, &longindex)) != -1 ) {
			switch(opt) {
			case '?':
			case 'h':
				std::cerr <<
				//  >---------------------- Standard terminal width ---------------------------------<
					"Options:\n"
					"  --help, -h, -?                  Displays this help message and exits\n"
					"  --version, -V                   Show the version\n"
					"  --foreground, -f                Don't fork into the background after init\n"
					"  --pid-file, -p file             The file to write the PID to, especially\n"
					"                                  usefull when running as a daemon\n"
					"  --bind, -b host:port            Bind to the specified address\n"
					"                                  host and port resolving can be bypassed by\n"
					"                                  placing [] around them\n"
					"  --log, -l file                  Log to file\n"
					;
				if( opt == '?' ) { exit(EX_USAGE); }
				exit(EX_OK);
			case 'V':
				std::cout << PACKAGE_NAME << " version " << PACKAGE_VERSION
				          << " (" << PACKAGE_GITREVISION << ")\n";
				exit(EX_OK);
			case 'f':
				options.fork = false;
				break;
			case 'p':
				options.pid_file = optarg;
				break;
			case 'b':
				options.bind_addr = optarg;
				break;
			case 'l':
				logfilename = optarg;
				logfile = fopen(logfilename.c_str(), "a");
				LogSetOutputFile(NULL, logfile);
				break;
			}
		}
	}

	LogInfo("%s version %s (%s) starting up", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_GITREVISION);


	{ // Open listening socket
		std::string host, port;

		/* Address format is
		 *   - hostname:portname
		 *   - [numeric ip]:portname
		 *   - hostname:[portnumber]
		 *   - [numeric ip]:[portnumber]
		 */
		size_t c = options.bind_addr.rfind(":");
		if( c == std::string::npos ) {
			std::cerr << "Invalid bind string \"" << options.bind_addr << "\": could not find ':'\n";
			exit(EX_DATAERR);
		}
		host = options.bind_addr.substr(0, c);
		port = options.bind_addr.substr(c+1);

		std::auto_ptr< boost::ptr_vector< SockAddr::SockAddr> > bind_sa
			= SockAddr::resolve( host, port, 0, SOCK_STREAM, 0);
		if( bind_sa->size() == 0 ) {
			std::cerr << "Can not bind to \"" << options.bind_addr << "\": Could not resolve\n";
			exit(EX_DATAERR);
		} else if( bind_sa->size() > 1 ) {
			// TODO: allow this
			std::cerr << "Can not bind to \"" << options.bind_addr << "\": Resolves to multiple entries:\n";
			for( typeof(bind_sa->begin()) i = bind_sa->begin(); i != bind_sa->end(); i++ ) {
				std::cerr << "  " << i->string() << "\n";
			}
			exit(EX_DATAERR);
		}
		s_listen = Socket::socket( (*bind_sa)[0].proto_family() , SOCK_STREAM, 0);
		s_listen.set_reuseaddr();
		s_listen.bind((*bind_sa)[0]);
		s_listen.listen(MAX_CONN_BACKLOG);
		LogInfo("Listening on %s", (*bind_sa)[0].string().c_str());
	}

	{
		/* Open pid-file before fork()
		 * That way, failing to open the pid-file will cause a pre-fork-abort
		 */
		std::ofstream pid_file;
		if( options.pid_file.length() > 0 ) pid_file.open( options.pid_file.c_str() );

		if( options.fork ) {
			pid_t child = fork();
			if( child == -1 ) {
				char error_descr[256];
				strerror_r(errno, error_descr, sizeof(error_descr));
				LogFatal("Could not fork: %s", error_descr);
				exit(EX_OSERR);
			} else if( child == 0 ) {
				// We are the child
				// continue on with the program
			} else {
				// We are the parent
				LogInfo("Forked; child PID %d", child);
				exit(0);
			}
		}

		if( options.pid_file.length() > 0 ) pid_file << getpid();
	}

	{
		ev_signal ev_sigint_watcher;
		ev_signal_init( &ev_sigint_watcher, received_sigint, SIGINT);
		ev_signal_start( EV_DEFAULT_ &ev_sigint_watcher);
		ev_signal ev_sigterm_watcher;
		ev_signal_init( &ev_sigterm_watcher, received_sigterm, SIGTERM);
		ev_signal_start( EV_DEFAULT_ &ev_sigterm_watcher);

		ev_signal ev_sighup_watcher;
		ev_signal_init( &ev_sighup_watcher, received_sighup, SIGHUP);
		ev_signal_start( EV_DEFAULT_ &ev_sighup_watcher);

		struct TcpBus_bus *bus = TcpBus_init(EV_DEFAULT_ s_listen);
		TcpBus_callback_newcon_add(bus, received_newcon);
		TcpBus_callback_error_add(bus, received_error);
		TcpBus_callback_disconnect_add(bus, received_disconnect);

		LogInfo("Setup done, starting event loop");

		ev_run(EV_DEFAULT_ 0);
	}

	LogInfo("Cleaning up");
	if( options.pid_file.length() > 0 ) remove( options.pid_file.c_str() );

	return 0;
}
