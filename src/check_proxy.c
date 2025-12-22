/**************************************************************************
* Copyright (c) 2002                                                      *
*      The staff of Balderdash MUD.  All rights reserved.                 *
*                                                                         *
* Redistribution and use in source and binary forms, with or without      *
* modification, are permitted provided that the following conditions      *
* are met:                                                                *
* 1. Redistributions of source code must retain the above copyright       *
*    notice, this list of conditions and the following disclaimer.        *
* 2. Redistributions in binary form must reproduce the above copyright    *
*    notice, this list of conditions and the following disclaimer in the  *
*    documentation and/or other materials provided with the distribution. *
* 3. Neither the name of Balderdash MUD the names of its contributors may *
*    be used to endorse or promote products derived from this software    *
*    without specific prior written permission.                           *
*                                                                         *
* THIS SOFTWARE IS PROVIDED BY BALDERDASH MUD AND CONTRIBUTORS ``AS       *
* IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          *
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS       *
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE         *
* STAFF OF FATAL DIMENSIONS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,     *
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES      *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR      *
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)      *
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,     *
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)           *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED     *
* OF THE POSSIBILITY OF SUCH DAMAGE.                                      *
***************************************************************************/

/* Version 0.1                                                    */
/* Alexey V. Antipovsky aka Kemm aka Ssart                        */

#include <sys/types.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#include "merc.h"
#include "config.h"
#include "telnet.h"


void _perror(const char *str);

#define SOCKS_PORT 1080
/*
 * CONNECT request byte order for SOCKS4:
 *            +----+----+----+----+----+----+----+----+----+----+....+----+
 *            | VN | CC | DSTPORT |       DST IP      | USER ID      |NULL|
 *            +----+----+----+----+----+----+----+----+----+----+....+----+
 * # of bytes:  1    1       2              4           variable       1
 *
 * VN = Version (4), CC = Command Code (1 is connect request)
 * Empty USER ID means anonymous connection
 */
#define SOCKS4_LEN 9
char socks4[SOCKS4_LEN];

/*
 * CONNECT request byte order for SOCKS5:
 *            +----+----+----+
 *            | VN | NM |METH|
 *            +----+----+----+
 * VN = Version (4), NM = Number of Methods, METH = Method.
 * Method 0 is 'No authentication requires'
 */
#define SOCKS5_LEN 3
char socks5[SOCKS5_LEN];

int http_ports[] = { 80, 81, 3128, 8080, 0 };

/*
 * CONNECT request for HTTP proxies
 *
 * CONNECT <ip>:<port> HTTP/1.0
 */
#define HTTP_LEN 42
char http[HTTP_LEN];


#define	ABUSEHOST	"whois.abuse.net"
#define	NICHOST		"whois.crsnic.net"
#define	INICHOST	"whois.networksolutions.com"
#define	DNICHOST	"whois.nic.mil"
#define	GNICHOST	"whois.nic.gov"
#define	ANICHOST	"whois.arin.net"
#define	LNICHOST	"whois.lacnic.net"
#define	KNICHOST	"whois.krnic.net"
#define	RNICHOST	"whois.ripe.net"
#define	PNICHOST	"whois.apnic.net"
#define	RUNICHOST	"whois.ripn.net"
#define	MNICHOST	"whois.ra.net"
#define	QNICHOST_TAIL	".whois-servers.net"
#define	SNICHOST	"whois.6bone.net"
#define	BNICHOST	"whois.registro.br"
#define NORIDHOST	"whois.norid.no"
#define	IANAHOST	"whois.iana.org"
#define GERMNICHOST	"de.whois-servers.net"
#define	WHOIS_SERVER_ID	"Whois Server: "
#define	WHOIS_ORG_SERVER_ID	"Registrant Street1:Whois Server:"

#define NONE_COUNTRY "NONE"

#define ishost(h) (isalnum((unsigned char)h) || h == '.' || h == '-')

const char *ip_whois[] = { LNICHOST, RNICHOST, PNICHOST, BNICHOST, NULL };

//#ifndef NOTHREAD
//struct addrinfo *gethostinfo(char const *host);
////char *whois(const char *, const char *);
//
//struct cp_list
//{
//    struct cp_list *next;
//    unsigned int address;
//    int state;
//    char *country;
//};
//
//struct cp_list *cp_list_head;
//int cp_terminate = 0;
//
//int cp_check_socks4(struct cp_list *list);
//int cp_check_socks5(struct cp_list *list);
//int cp_check_http(struct cp_list *list);
////int cp_check_whois(struct cp_list *list);
//
//pthread_mutex_t cp_mutex;
//pthread_t cp_thread;
//
//void
//cp_lookup()
//{
//    struct cp_list *list;
//    int ret;
//    sigset_t newmask;
//
//    sigfillset(&newmask);
//    if (pthread_sigmask(SIG_BLOCK, &newmask, NULL) < 0)
//	cp_terminate = TRUE;
//
//    while (!cp_terminate)
//    {
//    	pthread_mutex_lock(&cp_mutex);
//	for (list = cp_list_head; list; list = list->next)
//	{
//	    if (list->state < CP_IS_SOCKS4)
//		break;
//	}
//    	pthread_mutex_unlock(&cp_mutex);
//
//	if (list)
//	{
//	    ret = cp_check_whois(list);
//	    if (ret < CP_IS_SOCKS4)
//	    {
//		ret = cp_check_socks4(list);
//		if (ret < CP_IS_SOCKS4)
//		{
//		    ret = cp_check_socks5(list);
//		    if (ret < CP_IS_SOCKS4)
//		       ret = cp_check_http(list);
//		}
//	    }
//
//	    pthread_mutex_lock(&cp_mutex);
//	    list->state = ret;
//	    pthread_mutex_unlock(&cp_mutex);
//
//	    continue;
//	}
//
//	sleep(1);
//    }
//}
//
//int
//cp_check_socks4(struct cp_list *list)
//{
//    struct sockaddr_in sa;
//    unsigned char buf[256];
//    int sock;
//
//    bzero(&sa, sizeof(struct sockaddr_in));
//
//    sa.sin_family = AF_INET;
//    sa.sin_port = htons(SOCKS_PORT);
//
//    pthread_mutex_lock(&cp_mutex);
//    sa.sin_addr.s_addr = list->address;
//    pthread_mutex_unlock(&cp_mutex);
//
//    sock = socket(PF_INET, SOCK_STREAM, 0);
//    if (sock < 0)
//    {
////	_perror("cp_check_socks4(): socket()");
//	return CP_CHK_SOCKS5;
//    }
//
//    if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
//    {
//	switch (errno)
//	{
//		case ETIMEDOUT:
//	    /* FALLTHROUGH */
//		case ECONNREFUSED:
//			close(sock);
//			return CP_CHK_SOCKS5;
//
//
//		default:
//	//	    _perror("cp_chk_socks4(): connect()");
//			return CP_CHK_SOCKS5;
//
//	}
//    }
//
//    if (send(sock, socks4, SOCKS4_LEN, 0) < 0)
//    {
////	_perror("cp_check_socks4(): send()");
//	close(sock);
//	return CP_CHK_SOCKS5;
//    }
//
//    bzero(buf, 256);
//    if (recv(sock, buf, 256, 0) < 0)
//    {
////	_perror("cp_chk_socks4(): recv()");
//	close(sock);
//	return CP_CHK_SOCKS5;
//    }
//
//    if ((buf[0] == 0x00 || buf[0] == 0x04)
//	&& buf[1] == 90) /* Is it right? */
//    {
//	return CP_IS_SOCKS4;
//    }
//
//    return CP_CHK_SOCKS5;
//}
//
//int
//cp_check_socks5(struct cp_list *list)
//{
//    struct sockaddr_in sa;
//    unsigned char buf[256];
//    int sock;
//
//    bzero(&sa, sizeof(struct sockaddr_in));
//
//    sa.sin_family = AF_INET;
//    sa.sin_port = htons(SOCKS_PORT);
//
//    pthread_mutex_lock(&cp_mutex);
//    sa.sin_addr.s_addr = list->address;
//    pthread_mutex_unlock(&cp_mutex);
//
//    sock = socket(PF_INET, SOCK_STREAM, 0);
//    if (sock < 0)
//    {
////	_perror("cp_check_socks5(): socket()");
//	return CP_CHK_HTTP;
//    }
//
//    if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
//    {
//	switch (errno)
//	{
//	case ETIMEDOUT:
//	    /* FALLTHROUGH */
//	case ECONNREFUSED:
//	    close(sock);
//	    return CP_CHK_HTTP;
//
//	default:
////	    _perror("cp_chk_socks5(): connect()");
//	    return CP_CHK_HTTP;
//
//	}
//    }
//
//    if (send(sock, socks5, SOCKS5_LEN, 0) < 0)
//    {
////	_perror("cp_check_socks5(): send()");
//	close(sock);
//	return CP_CHK_HTTP;
//    }
//
//    bzero(buf, 256);
//    if (recv(sock, buf, 256, 0) < 0)
//    {
////	_perror("cp_chk_socks5(): recv()");
//	close(sock);
//	return CP_CHK_HTTP;
//    }
//
//    if (buf[0] == 0x05
//	&& buf[1] == 0x00)
//    {
//	return CP_IS_SOCKS5;
//    }
//
//    return CP_CHK_HTTP;
//}
//
//int
//cp_check_http(struct cp_list *list)
//{
//    struct sockaddr_in sa;
//    unsigned char buf[256];
//    int sock;
//    int i;
//
//    bzero(&sa, sizeof(struct sockaddr_in));
//
//    for (i = 0; http_ports[i] != 0; i++)
//    {
//	bool failed = FALSE;
//
//	sa.sin_family = AF_INET;
//	sa.sin_port = htons(http_ports[i]);
//
//	pthread_mutex_lock(&cp_mutex);
//	sa.sin_addr.s_addr = list->address;
//	pthread_mutex_unlock(&cp_mutex);
//
//	sock = socket(PF_INET, SOCK_STREAM, 0);
//	if (sock < 0)
//	{
////    	    _perror("cp_check_http(): socket()");
//    	    continue;
//       	}
//
//	if (connect(sock, (struct sockaddr *)&sa, sizeof(struct sockaddr_in))<0)
//	{
//    	    switch (errno)
//    	    {
//    	    case ETIMEDOUT:
//    		/* FALLTHROUGH */
//    	    case ECONNREFUSED:
//    		close(sock);
//    		failed = TRUE;
//    		break;
//
//    	    default:
////    		_perror("cp_chk_http(): connect()");
//    		failed = TRUE;
//    		break;
//    	    }
//       	}
//
//	if (failed)
//	    continue;
//
//	if (send(sock, http, HTTP_LEN, 0) < 0)
//	{
////    	    _perror("cp_check_http(): send()");
//    	    close(sock);
//    	    continue;
//       	}
//
//	bzero(buf, 256);
//	if (recv(sock, buf, 256, 0) < 0)
//	{
////    	    _perror("cp_chk_http(): recv()");
//    	    close(sock);
//    	    continue;
//       	}
//
//	if (!str_prefix("HTTP/1.0 200", buf))
//	{
//	    close(sock);
//    	    return CP_IS_HTTP;
//       	}
//
//	close(sock);
//    }
//
//    return CP_IS_NONE;
//}
//
//int
//cp_check_whois(struct cp_list *list){
//#if !defined (ONEUSER)
//    unsigned int address;
//    unsigned char buf[MSL];
//    char whs[20];
//    char cntr[] = "country:", *out, *line, *p;
//    bool checked = FALSE;
//
//    address = ntohl(list->address);
//    sprintf(whs, "%u.%u.%u.%u\r\n",
//	    (address >> 24) & 0xFF, (address >> 16) & 0xFF,
//	    (address >>  8) & 0xFF, (address      ) & 0xFF);
//
//    out = whois(whs, ANICHOST);
//
//    if (IS_NULLSTR(out))
//	return CP_CHK_WHOIS;
//
//    p = out;
//    line = out;
//
//    while (1)
//    {
//	if (*p == '\0')
//	    break;
//
//	if (*p == '\n')
//	{
//	    line = one_argument(line, buf);
//
//	    if (!strcmp(buf, cntr))
//	    {
//		int i;
//
//		line[2] = '\0';
//		free_string(list->country);
//		list->country = str_dup(line);
//
//		for(i = 0; i < MAX_WHOIS_ENTRIES && whiteip[i] != NULL; i++)
//		    if (!str_prefix(whiteip[i], whs))
//		    {
//			checked = TRUE;
//			break;
//		    }
//
//		for(i = 0; i < MAX_WHOIS_ENTRIES && whitedomains[i] != NULL; i++)
//		    if (!str_cmp(line, whitedomains[i]))
//		    {
//			checked = TRUE;
//			break;
//		    }
//	    }
//	    if (checked)
//		break;
//
//	    line = p + 1;
//	}
//
//	p++;
//    }
//
//    if (!str_cmp(list->country, NONE_COUNTRY))
//	return CP_CHK_WHOIS;
//
//    if (checked)
//	return CP_CHK_SOCKS4;
//    else
//	return CP_IS_WHOIS;
//#else
//    return CP_CHK_SOCKS4;
//#endif
//}
//
//void
//cp_init() {
//    int port = /* htonl( */cfg.port /* ) */;
//    unsigned int address;
//    char caddr[] = "89.249.128.37\0"; /* XXX */
///*    int localport = 9000;	   */  /* XXX */
//
//    address = inet_addr(caddr);
//    address = htonl(address);
//
//    sprintf(socks4, "%c%c%c%c%c%c%c%c%c",
//		    4,
//		    1,
//		    (port >> 8) & 0xFF,
//		    port & 0xFF,
//		    (address  >> 24) & 0xFF,
//		    (address  >> 16) & 0xFF,
//		    (address  >>  8) & 0xFF,
//		    address  & 0xFF,
//		    0);
//
//    sprintf(socks5, "%c%c%c", 5, 1, 0);
//
//    sprintf(http, "CONNECT %s:%d HTTP/1.0\r\n\r\n", caddr, port);
//
//    cp_list_head = malloc(sizeof(struct cp_list));
//    cp_list_head->next = NULL;
//    cp_list_head->state = CP_START;
//
//    pthread_mutex_init(&cp_mutex, NULL);
//    pthread_create(&cp_thread, NULL, (void *)&cp_lookup, NULL);
//    pthread_detach(cp_thread);
//
////    at_exit(&cp_destroy);
//    return;
//}
//
//void cp_add(unsigned int address){
//    struct cp_list *list, *newlist;
//    int found = FALSE;
//
//    pthread_mutex_lock(&cp_mutex);
//    for (list = cp_list_head; list; list = list->next)
//    {
//	if (list->address == address)
//	{
//	    found = TRUE;
//	    break;
//	}
//
//	if (list->next == NULL)
//	    break;
//    }
//
//    if (!found)
//    {
//	struct in_addr addr;
//
//	newlist = malloc(sizeof(struct cp_list));
//	newlist->next = NULL;
//
//	if (cp_list_head->state == CP_START){
//	    cp_list_head = newlist;
//	} else {
//		list->next = newlist;
//	}
//
//	newlist->address = address;
//	newlist->state = CP_CHK_WHOIS;
//	newlist->country = str_dup(NONE_COUNTRY);
//	addr.s_addr = address;
//    }
//
//    pthread_mutex_unlock(&cp_mutex);
//}
//#endif

void check_proxy(unsigned int address){
//#ifndef NOTHREAD
//    static int running = 0;
//
//    if (running == 0)
//    {
//	running++;
//	cp_init();
//    }
//
//    cp_add(address);
//#endif
}

int cp_get_state(unsigned int address){
	return FALSE;
//#ifndef NOTHREAD
//    struct cp_list *list;
//    int found = FALSE;
//    int ret = FALSE;
//
//    pthread_mutex_lock(&cp_mutex);
//    for (list = cp_list_head; list; list = list->next)
//	if (list->address == address)
//	{
//	    found = TRUE;
//	    break;
//	}
//
//    if (found)
//    {
//	if (list->state == CP_IS_SOCKS4
//	    || list->state == CP_IS_SOCKS5
//	    || list->state == CP_IS_WHOIS
//	    || list->state == CP_IS_HTTP)
//	{
//	    ret = list->state;
//	}
//	else
//	    ret = FALSE;
//    }
//
//    pthread_mutex_unlock(&cp_mutex);
//    return ret;
//#else
//    return FALSE;
//#endif
}

char * cp_get_country(unsigned int address){
	return "LOCAL";
//#ifndef NOTHREAD
//    struct cp_list *list;
//    int found = FALSE;
//
//    pthread_mutex_lock(&cp_mutex);
//    for (list = cp_list_head; list; list = list->next)
//	if (list->address == address)
//	{
//	    found = TRUE;
//	    break;
//	}
//
//    pthread_mutex_unlock(&cp_mutex);
//
//    return found ? list->country : "";
//#else
//    return "LOCAL";
//#endif
}

//
//#if !defined(NOTHREAD) && !defined (ONEUSER)
//struct addrinfo * gethostinfo(char const *host){
//	struct addrinfo hints, *res;
//	int error;
//
//	memset(&hints, 0, sizeof(hints));
//	hints.ai_flags = 0;
//	hints.ai_family = AF_UNSPEC;
//	hints.ai_socktype = SOCK_STREAM;
//	error = getaddrinfo(host, "whois", &hints, &res);
//	if (error) {
//		bugf("Whois: %s: %s", host, gai_strerror(error));
//		return (NULL);
//	}
//	return (res);
//}
//
//
//char * whois(const char *query, const char *hostname){
//	FILE *sfi, *sfo;
//	struct addrinfo *hostres, *res;
//	char *buf, *host, *p;
//	int i, s = 0;
//	size_t c, len;
//	char nhost[MSL];
//	static char whois_ret[OUTBUF_SIZE];
//
//	whois_ret[0] = '\0';
//
//	pthread_mutex_lock(&cp_mutex);
//	hostres = gethostinfo(hostname);
//	pthread_mutex_unlock(&cp_mutex);
//
//	for (res = hostres; res; res = res->ai_next) {
//		s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
//		if (s < 0)
//			continue;
//		if (connect(s, res->ai_addr, res->ai_addrlen) == 0)
//			break;
//		close(s);
//	}
//
//	if (hostres)
//	    freeaddrinfo(hostres);
//
//	if (res == NULL)
//	{
//	bugf("Whois: connect()");
//		return "";
//	}
//
//	sfi = fdopen(s, "r");
//	sfo = fdopen(s, "w");
//	if (sfi == NULL || sfo == NULL)
//	{
//		bugf("Whois: fdopen()");
//		return "";
//	}
//	if (strcmp(hostname, GERMNICHOST) == 0) {
//		fprintf(sfo, "-T dn,ace -C US-ASCII %s\r\n", query);
//	} else {
//		fprintf(sfo, "%s\r\n", query);
//	}
//	fflush(sfo);
//	nhost[0] = '\0';
//	while ((buf = fgetln(sfi, &len)) != NULL) {
//		while (len > 0 && isspace((unsigned char)buf[len - 1]))
//			buf[--len] = '\0';
//
//
//		strcat(whois_ret, buf);
//		strcat(whois_ret, "\n");
//
//		if (IS_NULLSTR(nhost)) {
//			host = strnstr(buf, WHOIS_SERVER_ID, len);
//			if (host != NULL) {
//				host += sizeof(WHOIS_SERVER_ID) - 1;
//				for (p = host; p < buf + len; p++) {
//					if (!ishost(*p)) {
//						*p = '\0';
//						break;
//					}
//				}
//				snprintf(nhost, MSL, "%.*s", (int)(buf + len - host), host);
//			} else if ((host =
//			    strnstr(buf, WHOIS_ORG_SERVER_ID, len)) != NULL) {
//				host += sizeof(WHOIS_ORG_SERVER_ID) - 1;
//				for (p = host; p < buf + len; p++) {
//					if (!ishost(*p)) {
//						*p = '\0';
//						break;
//					}
//				}
//				snprintf(nhost, MSL, "%.*s", (int)(buf + len - host), host);
//			} else if (strcmp(hostname, ANICHOST) == 0) {
//				for (c = 0; c <= len; c++)
//					buf[c] = tolower((int)buf[c]);
//				for (i = 0; ip_whois[i] != NULL; i++) {
//					if (strnstr(buf, ip_whois[i], len) !=
//					    NULL) {
//						snprintf(nhost, MSL, "%s", ip_whois[i]);
//						break;
//					}
//				}
//			}
//		}
//	}
//
//	fclose(sfi);
//	fclose(sfo);
//
//	if (!IS_NULLSTR(nhost))
//	    whois(query, nhost);
//
//	return whois_ret;
//}
//#endif

/* charset=cp1251 */
