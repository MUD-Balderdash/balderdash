#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include <xds.h>

#include "queue.h"
#include "checker.h"


#define MAX_SER_SZ	256
    
static int
encode_host(xds_t *xds, void *engine_context, void *buffer, size_t buffer_size,
	    size_t *used_buffer_size, va_list *args)
{
    struct check_host *host;

    host = (struct check_host *)va_arg(*args, struct check_host *);
    return xds_encode(xds, "uint32 string string int32",
		      host->ip, host->hostname, host->country, host->state);
}

static int
decode_host(xds_t *xds, void *engine_context, void *buffer, size_t buffer_size,
	      size_t *used_buffer_size, va_list *args)
{
    struct check_host *host;

    host = (struct check_host *)va_arg(*args, struct check_host *);
    return xds_decode(xds, "uint32 string string int32",
		      &host->ip, &host->hostname, &host->country, &host->state);
}

xds_t *
cp_ser_init(xds_mode_t mode)
{
    xds_t *xds;
    int rc;

    if ((rc = xds_init(&xds, mode)) != XDS_OK)
	return NULL;

    rc = TRUE;
    
    switch (mode)
    {
    case XDS_ENCODE:
	if (xds_register(xds, "uint32", &xdr_encode_uint32, NULL) != XDS_OK
	    || xds_register(xds, "string", &xdr_encode_string, NULL) != XDS_OK
	    || xds_register(xds, "int32", &xdr_encode_int32, NULL) != XDS_OK
	    || xds_register(xds, "host", &encode_host, NULL) != XDS_OK)
	{
	    rc = FALSE;
	}
	break;

    case XDS_DECODE:
	if (xds_register(xds, "uint32", &xdr_decode_uint32, NULL) != XDS_OK
	    || xds_register(xds, "string", &xdr_decode_string, NULL) != XDS_OK
	    || xds_register(xds, "int32", &xdr_decode_int32, NULL) != XDS_OK
	    || xds_register(xds, "host", &decode_host, NULL) != XDS_OK)
	{
	    rc = FALSE;
	}
    }

    if (rc != TRUE)
    {
	xds_destroy(xds);
	return NULL;
    }
    
    return xds;
}

void
cp_ser_destroy(xds_t *xds)
{
    xds_destroy(xds);
}

int
cp_ser_host(xds_t *xds, const struct check_host *host, char **buffer, size_t *bufsiz)
{
    if (xds_encode(xds, "host", host) != XDS_OK)
	return FALSE;

    if (xds_getbuffer(xds, XDS_GIFT, (void **)buffer, bufsiz) != XDS_OK)
    {
	free(*buffer);
	return FALSE;
    }

    return TRUE;
}

int
cp_deser_host(xds_t *xds, char *buffer, size_t bufsiz, struct check_host *host)
{
    if (xds_setbuffer(xds, XDS_LOAN, buffer, bufsiz) != XDS_OK)
	return FALSE;

    if (xds_decode(xds, "host", host) != XDS_OK)
	return FALSE;

    return TRUE;
}

int
cp_deser_hosts(xds_t *xds, char *buffer, size_t bufsiz, struct check_host_head *host_tailq)
{
    struct check_host host;
    struct check_host *phost;
    
    if (xds_setbuffer(xds, XDS_GIFT, buffer, bufsiz) != XDS_OK)
	return FALSE;

    while (xds_decode(xds, "host", &host) == XDS_OK)
    {
	phost = malloc(sizeof(struct check_host));
	memcpy(phost, &host, sizeof(struct check_host));
	STAILQ_INSERT_TAIL(host_tailq, phost, link);
    }

    if (!STAILQ_EMPTY(host_tailq))
	return TRUE;
    else
	return FALSE;
}

