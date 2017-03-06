
#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc, const char **argv)
{
    struct sockaddr_in *addr;
    struct addrinfo *haddr, hints;
    int i, lport = 9999, fd, len;
    char c = 1, res[1024];
    const char *hname = "localhost", *spec = "UNHALTED_CORE_CYCLES,ICACHE:HIT";

    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-') break;
	switch (argv[i][1]) {
	    case 'h': hname = argv[++i];
		      continue;
	    default:
		break;
	}
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(hname, NULL, &hints, &haddr);

    fd = socket(AF_INET, SOCK_STREAM, 0);

    /*addr.sin_family = AF_INET;
    addr.sin_port = htons(lport);
    addr.sin_addr = haddr.ai_addr;*/
    addr = (struct sockaddr_in *)haddr->ai_addr;
    addr->sin_port = htons(lport);

    if (connect(fd, addr, sizeof(struct sockaddr_in)) == -1) {
	fprintf(stderr, "Error connecting to %s: %s.\n", hname, strerror(errno));
	exit (-1);
    }
    //write(fd, &c, 1);
    for (; i < argc; i++) {
	spec = argv[i];
    len = strlen(spec) + 1;
    write(fd, &len, 4);
    fprintf(stderr, "writing %i and %i.\n", c, len);
    write(fd, spec, len);

    read(fd, &len, sizeof(int));
    read(fd, res, len);
    fprintf(stderr, "reading %i %s.\n", len, res);
    fprintf(stderr, "%s\n", res);
    }
    return 0;
}

