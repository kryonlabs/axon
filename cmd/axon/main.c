/*
 * AXON Server - Main Entry Point
 * Encyclopedia Filesystem for TaijiOS
 */

#include <lib9.h>
#include <9p.h>
#include <thread.h>
#include "axon.h"
#include "axonfs.h"

char *addr = nil;
int port = 17020;
char *data_path = "./data";

/*
 * Print usage
 */
void
usage(void)
{
    fprint(2, "usage: axon [-a address] [-p port] [-d datadir]\n");
    exits("usage");
}

/*
 * Thread main entry point
 */
void
threadmain(int argc, char **argv)
{
    Axon *axon;
    Srv *srv;
    int fd;
    char *addr_str;
    int i;

    ARGBEGIN {
    case 'a':
        addr = EARGF(usage());
        break;
    case 'p':
        port = atoi(EARGF(usage()));
        break;
    case 'd':
        data_path = EARGF(usage());
        break;
    default:
        usage();
    } ARGEND;

    /* Initialize AXON */
    axon = axon_init(data_path);
    if(axon == nil) {
        fprint(2, "axon: failed to initialize\n");
        exits("init");
    }

    /* Create 9P server */
    srv = axon_create_srv(axon);
    if(srv == nil) {
        fprint(2, "axon: failed to create server\n");
        exits("server");
    }

    /* Create address string for announce */
    if(addr != nil)
        addr_str = smprint("tcp!%s!%d", addr, port);
    else
        addr_str = smprint("tcp!*!%d", port);

    /* Listen on socket */
    fd = announce(addr_str);
    free(addr_str);

    if(fd < 0) {
        fprint(2, "axon: failed to announce on port %d\n", port);
        exits("announce");
    }

    /* Start serving */
    fprint(2, "axon: serving on port %d, data=%s\n", port, data_path);

    /*
     * Set up file descriptors for the 9P server
     * Note: In this simple implementation, we use stdin/stdout
     * for a single-connection model. A full implementation would
     * handle multiple connections.
     */
    srv->infd = fd;
    srv->outfd = fd;

    /* Run the server (blocking call) */
    srv(srv);

    /* Cleanup */
    axon_cleanup(axon);

    exits(nil);
}
