/* $Id: checker.h,v 1.1.2.1 2006/03/20 21:05:59 kemm Exp $ */
#ifndef CHECKER_H
#define CHECKER_H

#define CP_START	0
#define CP_CHK_SOCKS4	1
#define CP_CHK_SOCKS5	2
#define CP_CHK_HTTP	3
#define CP_CHK_WHOIS	4
#define CP_IS_SOCKS4	5
#define CP_IS_SOCKS5	6
#define CP_IS_HTTP	7
#define CP_IS_NONE	8

#define CP_SER_FLDS	4

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

struct check_host
{
    uint32_t	ip;			/* Network byte order!	*/
    char	*hostname;
    char	*country;
    int32_t	state;
    STAILQ_ENTRY(check_host) link;
};

STAILQ_HEAD(check_host_head, check_host);

/* checker_util.c */
xds_t	*cp_ser_init(xds_mode_t mode);
void	cp_ser_destroy(xds_t *xds);
int	cp_ser_host(xds_t *xds, const struct check_host *host, char **buffer, size_t *bufsiz);
int	cp_deser_host(xds_t *xds, char *buffer, size_t bufsiz, struct check_host *host);
int	cp_deser_hosts(xds_t *xds, char *buffer, size_t bufsiz, struct check_host_head *host_tailq);

#endif /* CHECKER_H */
