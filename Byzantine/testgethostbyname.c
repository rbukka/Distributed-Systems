 /* lookup.c:
  *   * Example of gethostbyname(3):
  *     * gethostbyname example
  *       * gethostbyname example code
  *         * supply the hostname as argument you want to lookup
  *           * see the sample output below the code
  *             */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

extern int h_errno;

int main(int argc,char **argv) {
				int x, x2;
				struct hostent *hp;

				for ( x=1; x<argc; ++x ) {
								/*
								 *    * Look up the hostname:
								 *       */
								printf("%s",argv[x]);
								hp = gethostbyname(argv[x]);
								if ( !hp ) { //Report lookup failure
												fprintf(stderr,
																				"%s: host '%s'\n",
																				hstrerror(h_errno),
																				argv[x]);
												continue;
								}

								/*  
								 *     * Report the findings:
								 *        */
								printf("Host %s : \n" ,argv[x]);
								printf(" Officially:\t%s\n", hp->h_name);
								fputs(" Aliases:\t",stdout);
								for ( x2=0; hp->h_aliases[x2]; ++x2 ) {
												if ( x2 ) {
																fputs(", ",stdout);
												}
												fputs(hp->h_aliases[x2],stdout);
								}     
								fputc('\n',stdout);
								printf(" Type:\t\t%s\n",
																hp->h_addrtype == AF_INET
																? "AF_INET" : "AF_INET6");
								if ( hp->h_addrtype == AF_INET ) {
												for ( x2=0; hp->h_addr_list[x2]; ++x2 ) {
																printf(" Address:\t%s\n",
																								inet_ntoa( *(struct in_addr *)
																												hp->h_addr_list[x2]));
												}
								}
								putchar('\n');
				}
				return 0;
}

/*
 *  * OUTPUT
 *   *
 *   [sgupta@rhel55x86 chap9] gcc -c -D_GNU_SOURCE -Wall -Wreturn-type lookup.c
 *   gcc lookup.o -o lookup
 *   [sgupta@rhel55x86 chap9]$ ./lookup www.lwn.net sunsite.unc.edu ftp.redhat.com
 *   Host www.lwn.net :
 *   Officially: lwn.net
 *   Aliases: www.lwn.net
 *   Type: AF_INET
 *   Address: 206.168.112.90
 *   Host sunsite.unc.edu :
 *   Officially: sunsite.unc.edu
 *   Aliases:
 *   Type: AF_INET
 *   Address: 152.2.254.81
 *   Host ftp.redhat.com :
 *   Officially: ftp.redhat.com
 *   Aliases:
 *   Type: AF_INET
 *   Address: 206.132.41.212
 *   Address: 208.178.165.228
 *   [sgupta@rhel55x86 chap9] $
 *    */

