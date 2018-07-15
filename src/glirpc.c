/*
 * (C) Copyright 1992  Josef Heinen
 *
 *
 * FACILITY:
 *
 *      Graphics Language Interpreter (GLI)
 *
 * ABSTRACT:
 *
 *      This module contains an RPC based call interface.
 *
 * AUTHOR(S):
 *
 *      Ruediger Mannert,
 *      Josef Heinen
 *
 * VERSION:
 *
 *      V1.0
 *
 */


#include <stdio.h>

#ifdef RPC

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>

#ifdef hpux
#include <limits.h>
#include <sys/utsname.h>
#endif

#include "terminal.h"

#define PROGNUM ((u_long)0x202FBF00)	/* 540000000 */
#define PROCNUM ((u_long)1)

#define NAMELEN     100
#define MESSAGELEN  133
#define ARGC_MAX    32
#define TIMEOUT     30


/* default rule for building RPC version numbers */

#define SERVICE_NUMBER (((unsigned long) geteuid ()) + 1000)

#ifndef caddr_t
#define caddr_t char *
#endif

#ifndef u_int
#define u_int unsigned int
#endif

/* Data structure passed via RPC */

typedef struct
 {
   char   *message;
   int     argc;
   int    *sizes;
   float **data;
 } rpccmd_t;

static void (*user_proc)(char *, int, int *, float **) = 0;

static int auth_num_uids = 0;
static int *auth_uids = NULL;

/*
 * Data conversion routine from and to network format.
 */

static
bool_t xdr_rpccmd (XDR *xdrs, rpccmd_t *objp)
 {
   int i;

   if (!xdr_string (xdrs, &(objp->message), MESSAGELEN))
      return FALSE;

   if (!xdr_int (xdrs, &(objp->argc)))
      return FALSE;

   if (!xdr_vector (xdrs, (char *) objp->sizes, (u_int) objp->argc,
		    (u_int) sizeof (int), (xdrproc_t) xdr_int))
      return FALSE;

   for (i = 0; i < objp->argc; i ++)
      if (!xdr_array (xdrs, (caddr_t *) &((objp->data)[i]),
		      (u_int *) &((objp->sizes)[i]), (u_int) objp->sizes[i],
                      (u_int) sizeof (float), (xdrproc_t) xdr_float))
         return FALSE;

   return TRUE;
 }


/*
 * A portable gethostname routine
 */

static
char *get_host_name (char *name)
 {
#ifdef hpux
   struct utsname utsname;

   uname (&utsname);
   strcpy (name, utsname.nodename);
#else
   gethostname (name, NAMELEN);
#endif
   strtok (name, ".");

   return name;
 }


/*
 * Create a client.
 */

static
CLIENT *clnt_connect (struct hostent *hostent, unsigned long prognum,
   unsigned long versnum)
 {
   struct sockaddr_in server_addr;
   int                socket_fd;
   CLIENT            *client;

   memcpy (&server_addr.sin_addr, hostent->h_addr, hostent->h_length);
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = 0;
   socket_fd = RPC_ANYSOCK;

   client = (CLIENT *) clnttcp_create (&server_addr, prognum, versnum,
	&socket_fd, 0, 0);

   return client;
 }


/*
 * Check if an RPC service is available.
 */

static
int clnt_test (struct hostent *hostent, unsigned long prognum,
   unsigned long versnum)
 {
          CLIENT         *client;
   static struct timeval  timeout = { TIMEOUT, 0 };
          int             status = 0;

   client = clnt_connect (hostent, prognum, versnum);
   if (client)
    {
      if (clnt_call (client, NULLPROC, (xdrproc_t) xdr_void, NULL,
                     (xdrproc_t) xdr_void, NULL, timeout) == RPC_SUCCESS)
         status = 1;
      clnt_destroy (client);
    }
   return status;
 }


/*
 * RPC authorization routine
 */

int gli_authrpc (int uid)
 {
   static int    this_uid = -1;
   register int  arg;

   if (this_uid == -1)
      this_uid = geteuid ();

   if (uid != this_uid)
    {
      for (arg = 0; arg < auth_num_uids; arg++)
       {
	 if (uid == auth_uids[arg])
	    return 0;
       }
      return 1;
    }
   else
      return 0;

 } /* gli_authrpc () */


/*
 * The following routine is invoked for each incoming RPC
 */

static
void dispatch (struct svc_req *rqstp, SVCXPRT *transp)
 {
   static char            message[MESSAGELEN];
   static int             sizes[ARGC_MAX];
   static float         *(data[ARGC_MAX]);
   static rpccmd_t        rpccmd = { message, 0, sizes, data };
   struct authunix_parms *auth;

   if (rqstp->rq_proc == NULLPROC)
    {
      /* Just send a reply */
      svc_sendreply (transp, (xdrproc_t) xdr_void, NULL);
    }
   else
    {
      /* Get the arguments */
      memset (data, 0, ARGC_MAX * sizeof (float *));
      svc_getargs (transp, (xdrproc_t) xdr_rpccmd, (caddr_t) &rpccmd);

      /* Get authentication */
      auth = (struct authunix_parms *) rqstp->rq_clntcred;
      if (!gli_authrpc (auth->aup_uid))
       {
         /* Call the user-supplied routine */
         (*user_proc) (rpccmd.message, rpccmd.argc, rpccmd.sizes, rpccmd.data);

         /* Send back the results */
	 svc_sendreply (transp, (xdrproc_t) xdr_rpccmd, (caddr_t) &rpccmd);
         svc_freeargs (transp, (xdrproc_t) xdr_rpccmd, (caddr_t) &rpccmd);
       }
      else
       {
         sprintf (message, "?%s:%d: unauthorized RPC", auth->aup_machname,
                  auth->aup_uid);
         /* Send back the error message */
	 svc_sendreply (transp, (xdrproc_t) xdr_rpccmd, (caddr_t) &rpccmd);
       }
    }
 }


int gli_registerrpc (
   int argc, char **argv, void (*proc)(char *, int, int *, float **))
 {
   SVCXPRT        *transp;
   char            hostname[NAMELEN+1];
   struct hostent *hostent;
   unsigned long   service_number;
   int             arg;

   /* Get local host's IP address */
   get_host_name (hostname);
   hostent = gethostbyname (hostname);

   /* Search arguments for valid service number */
   if ((argc < 2) ||
       (sscanf (argv[1], "%*[:]%lu", &service_number) < 1))
    {
      /* Otherwise get the default service-number. */
      service_number = SERVICE_NUMBER;
    }

   /* Ensure not to duplicate a service by attempting to call it. */
   if (clnt_test (hostent, PROGNUM, service_number))
    {
      tt_fprintf (stderr, "%s: Service '%s:%lu' already exists\n",
        argv[0], hostname, service_number);
      exit (1);
    }

   /* Parse remaining arguments for authorized UIDs */
   if (argc > 2)
    {
      auth_num_uids = argc - 2;
      auth_uids = (int *) malloc (auth_num_uids * sizeof(int));

      for (arg = 0; arg < auth_num_uids; arg++)
         auth_uids[arg] = atoi(argv[arg + 2]);
    }

   /* Unregister service. Registration may fail if omitted. */
   pmap_unset (PROGNUM, service_number);

   /* Create a TCP socket and register it to the portmapper */
   if (((transp = (SVCXPRT *) svctcp_create (RPC_ANYSOCK, 0, 0)) == NULL) ||
       (!svc_register (transp, PROGNUM, service_number, dispatch,
                       IPPROTO_TCP)))
    {
      tt_fprintf (stderr, "Service '%s' could not be registered\n", argv[0]);
      exit (1);
    }

   user_proc = proc;
   svc_run ();

   tt_fprintf (stderr, "%s terminated abnormally\n", argv[0]);

   return 1;

 } /* gli_registerrpc () */


/*
 * Split a service into hostname and service ID
 */

static
void parse_service (char *service, char *hostname,
   unsigned long *service_number)
 {
   char           *c;

   /* Read hostname */
   if ((service == NULL) || (*service == '\0') || (*service == ':'))
    {
      get_host_name (hostname);
    }
   else
    {
      sscanf (service, "%[^:]", hostname);
    }

   /* Read service number */
   if ((service == NULL) ||
       ((c = strrchr (service, ':')) == NULL) ||
       (sscanf (c + 1, "%lu", service_number) < 1))
    {
      *service_number = SERVICE_NUMBER;
    }
 }


#endif /* RPC */


/*
 * GLI entry point
 */

void gli_callrpc (char *message, int argc, int *sizes, float **data)
 {
#ifdef RPC
          char           *service;
   static char            hostname[NAMELEN+1];
          struct hostent *hostent;
          unsigned long   service_number;
          CLIENT         *client;
          rpccmd_t        rpccmd;
   static struct timeval  timeout = { TIMEOUT, 0 };

   service = getenv ("GLI_SERVICE");

   /* Split into hostname and service number */
   parse_service (service, hostname, &service_number);

   /* Check if host is known */
   hostent = gethostbyname (hostname);
   if (hostent == NULL)
    {
      sprintf (message, "?%s: unknown host", hostname);
    }
   else
    {
      /* Check if server is running */
      client = clnt_connect (hostent, PROGNUM, service_number);
      if (client == NULL)
       {
         sprintf(message, "?%s:%lu: unknown service", hostname, service_number);
       }
      else
       {
         /* Arrange parameters */
         rpccmd.message = message;
         rpccmd.argc = argc;
         rpccmd.sizes = sizes;
         rpccmd.data = data;

         /* Get authentication */
         client->cl_auth = authunix_create_default ();

         /* Perform the RPC */
         if (clnt_call (client, PROCNUM, (xdrproc_t) xdr_rpccmd,
			(caddr_t) &rpccmd, (xdrproc_t) xdr_rpccmd,
			(caddr_t) &rpccmd, timeout) != RPC_SUCCESS)
          {
            sprintf (message, "?Service %s:%lu does not respond", hostname,
                     service_number);
          }

         /* Delete authentication and close connection */
         auth_destroy (client->cl_auth);
         clnt_destroy (client);

       } /* if client */
    }    /* if host */

#else /* RPC */
   sprintf (message, "?RPC not supported for this type of machine");
#endif /* RPC */

 } /* gli_callrpc () */
