/*
 * (C) Copyright 1996-1998  Josef Heinen
 *
 *
 * FACILITY:
 *
 *	GLI
 *
 * ABSTRACT:
 *
 *	This module contains a serial line communication routine.
 *
 * USAGE:
 *
 *	com(string)
 *
 * PARAMETERS:
 *
 *	string
 *		[-d[isplaycontrols]] [-n[ewline]] [-v[erbose]] [-<speed>]
 *		[-e[cho]] [device] [command] [response]
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0-00
 *
 */

#include <stdio.h>

#if defined(__alpha) || defined(ultrix) || defined(aix) || defined(linux)

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef ultrix
#define NO_TERMIO
#endif

#ifdef NO_TERMIO
#include <sys/ioctl.h>
#include <sgtty.h>
#else
#include <termios.h>
#endif

#include <sys/file.h> 
#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 5*BUFSIZ

#define usage "\
?usage: com [-d[isplaycontrols]] [-n[ewline]] [-v[erbose]] [-<speed>]\n\
	    [-e[cho]] [device] [command] [response]"

#ifdef sun
extern char *sys_errlist[];
extern int sys_nerr;
#define strerror(errno) (errno < sys_nerr) ? sys_errlist[errno] : NULL
#endif

#ifdef NO_TERMIO
static struct sgttyb Iterm, Iterm2, term, term2;
static int speed = B9600;
#else
static struct termios Iterm, term;
static speed_t speed = B9600;
#endif /* NO_TERMIO */

static int filedes, ret, ret1;	  
static fd_set readfd, writefd, exception;
static int streamlf = 0, displaycontrols = 0, verbose = 0, echo = 0;
static jmp_buf done;
static int value = 0;
static char result[BUFSIZ];

extern int errno;

static char *cntrl[] = {
    "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
    "BS",  "HT",  "LF",  "VT",	"FF",  "CR",  "SO",  "SI",
    "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", 
    "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US"
};

static
void fatal(char *string)
{
    char *message;

    message = strerror(errno);
    if (message == NULL)
	message = "Unknown error";
    sprintf(result, "?%s: %s", string, message);
    longjmp(done, value);
}

static
void setline(void)
{
#ifdef NO_TERMIO
    struct sgttyb term, term2;
#else
    struct termios term;
#endif /* NO_TERMIO */

#ifdef NO_TERMIO
    ioctl(filedes, TIOCGETP, &term);
    ioctl(filedes, TIOCLGET, &term2);

    term.sg_flags &= ~(CRMOD | ECHO);
    term.sg_flags |= (CBREAK | RAW);
    term.sg_ispeed = speed;
    term.sg_ospeed = speed;
    ioctl(filedes, TIOCSETN, &term);
#else
    if (tcgetattr(filedes, &term) < 0)
	fatal("tcgetattr");

    term.c_cflag |= (CREAD | CS8 | CLOCAL);
    term.c_lflag &= ~(ECHO | ICANON | ISIG);
    term.c_iflag &= ~(IGNCR | INLCR | ICRNL);
    cfsetispeed(&term, speed);
    cfsetospeed(&term, speed);
    if (tcsetattr(filedes, TCSADRAIN, &term) < 0)
	fatal("tcsetattr");

    if (cfgetispeed(&term) != speed || cfgetispeed(&term) != speed) {
	sprintf(result, "?Invalid speed");
	longjmp(done, value);
    }
#endif
}

static
void readline(char *buf)
{
    for (;;) {
	if (read(filedes, buf, 1) == 1) {
	    if (*buf == '\r' || *buf == ':') {
		*buf = '\0';
		break;
	    }
	    else if (*buf != '\n')
		buf++;
	} else
	    fatal("read");
    }
}

static
void dispcntrls(char *buf, int ret, char *buffer, int *nbytes)
{
    register int i;
    register char c;

    *nbytes = 0;
    for (i = 0; i < ret; i++) {
	c = buf[i];
	if (iscntrl(c)) {
	    sprintf(buffer + *nbytes, "<%s>", c < 040 ? cntrl[(int)c] : "DEL");
	    if (c == '\n')
		strcat(buffer, "\n\r");
	    *nbytes = strlen(buffer);
	}
	else
	    buffer[(*nbytes)++] = c;
    }
}

static
void resetline(void)
{
    close(filedes);
}

static
void resettty(int sig)
{
    /*
     * Restore the terminal characteristics to their state before the
     * current session was entered.
     */
#ifdef NO_TERMIO
    ioctl(0, TIOCSETN, &Iterm);
#else
    if (tcsetattr(0, TCSADRAIN, &Iterm) < 0)
	fatal("tcsetattr");
#endif

    resetline();

    printf("\nConnection closed\n\n");
    longjmp(done, value);
}

static
void localcommands(void)
{
    resettty(0);
}

static
void termmain(void)
{
    char buf[BUFFER_SIZE], buffer[BUFFER_SIZE], *bufptr;
    int nbytes;

#ifdef NO_TERMIO
    ioctl(0, TIOCGETP, &term);
    ioctl(0, TIOCLGET, &term2);

    term.sg_flags &= ~(CRMOD | ECHO);
    term.sg_flags |= (CBREAK | RAW);

    ioctl(0, TIOCSETN, &term);
#else
    if (tcgetattr(0, &Iterm) < 0)
	fatal("tcgetattr");

    term = Iterm;
    term.c_iflag &= ~(IGNCR | INLCR);
    if (!streamlf)
	term.c_iflag &= ~ICRNL;
    term.c_lflag &= ~(ICANON | ISIG);
    if (!echo)
	term.c_lflag &= ~ECHO;
    if (tcsetattr(0, TCSADRAIN, &term) < 0) 
	fatal("tcsetattr");
#endif

    signal(SIGHUP, resettty);
    signal(SIGINT, resettty);
    signal(SIGQUIT, resettty);
    signal(SIGBUS, resettty);
    signal(SIGSEGV, resettty);

    printf("\r\nEscape character is '^\\' (Ctrl-\\, ASCII 28, FS).\r\n\n");
    for (;;) {
	FD_ZERO(&readfd);
        FD_SET(0, &readfd);
        FD_SET(filedes, &readfd);
	FD_ZERO(&exception);
        FD_SET(0, &exception);
        FD_SET(filedes, &exception);
	errno = 0;
	if ((select(FD_SETSIZE, &readfd, NULL, &exception, NULL)) > 0) {
	    if (FD_ISSET(filedes, &readfd)) {
		if ((ret = read(filedes, buf, BUFSIZ)) <= 0)
		    resettty(0);

		if (displaycontrols) {
		    dispcntrls(buf, ret, buffer, &nbytes);
		    strncpy(buf, buffer, nbytes);
		    ret = nbytes;
		}
		ret1 = write(0, buf, ret);     
		ret -= ret1;
		bufptr = buf + ret1;

		while (ret) {
		    FD_ZERO(&writefd);
		    FD_SET(0, &writefd);
		    select(FD_SETSIZE, NULL, &writefd, NULL, NULL);
		    if (FD_ISSET(0, &writefd)) {
			ret1 = write(0, bufptr, ret);	  
			ret -= ret1;
			bufptr = bufptr + ret1;
		    }
		}
	    }
	    if (FD_ISSET(0, &readfd)) {
		ret = read(0, buf, BUFSIZ);
		if (ret > 0) {
		    char *s;
		    buf[ret] = '\0';
		    if (streamlf) {
			while ((s = strchr(buf, '\r')) != NULL)
			    *s = '\n';
		    }
		    if (strchr(buf, 28) != NULL) {
			localcommands();
			continue;
		    }
		    write(filedes, buf, ret);
		}
	    }
	    if (FD_ISSET(filedes, &exception))
		resettty(0);
	}
	else
	    fatal("select");
    }
}

static
void writeline(char *format, char *buf)
{
    char buffer[BUFFER_SIZE];
    int nbytes;

    dispcntrls(buf, strlen(buf), buffer, &nbytes);
    buffer[nbytes] = '\0';

    printf(format, buffer);
}

static
void noresponse(int sig)
{
    sprintf(result, "?No response\n");

    resetline();
    longjmp(done, value);
}

static
void expect(char *command, char *response)
{
    char buf[BUFSIZ];

    if (response != NULL) {
	signal(SIGALRM, noresponse);
	alarm(3);
    }

    sprintf(buf, "%s%c", command, streamlf ? '\n' : '\r');
    write(filedes, buf, strlen(buf));
    if (verbose)
	writeline("-->%s\n", buf);

    if (response != NULL) {
	for (;;) {
	    readline(buf);
	    if (verbose)
		writeline("<--%s\n", buf);
	    if (strstr(buf, response) || *response == '\0')
		break;
	}
	strcpy(result, buf);

	alarm(0);
	signal(SIGALRM, SIG_IGN);
    }

    resetline();
}

static
int opensocket(char *path)
{
    char *host, *port;
    struct sockaddr_in saddr;
    struct hostent *host_ptr;
    int filedes;

    host = strtok(path, ":");
    port = strtok(NULL, ":");
    if ((host_ptr = gethostbyname(host)) == NULL) {
	perror("gethostbyname");
	return -1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(port));
    memcpy(&saddr.sin_addr, host_ptr->h_addr_list[0], host_ptr->h_length);

    if ((filedes = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("socket");
	return -1;
    }
    if (connect(filedes, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
	perror("connect");
	close(filedes);
	return -1;
    }
    streamlf = 1;

    return filedes;
}

static
char *getparm(char *s)
{
    static char *last = NULL;
    char *tok;

    if (s == NULL)
	s = last;
    if (s == NULL)
	return NULL;

    while (*s == ' ' || *s == '\t')
	s++;
    if (*s == '\0')
	return NULL;

    if (*s == '"') {
	tok = ++s;
	while (*s && *s != '"')
	    s++;
	if (*s == '"')
	    *s++ = '\0';
    } else {
	tok = s;
	while (*s && *s != ' ' && *s != '\t')
	    s++;
    }
    if (*s)
	*s++ = '\0';
    last = s;

    return tok;
}

char *com(char *string)
{
    char *parm, *path = NULL, *command = NULL, *response = NULL;
    char buf[MAXPATHLEN];

    *result = '\0';

    /* Parse command line */
    parm = getparm(string);
    while (parm) {
	if (!strcmp(parm, "-d") || !strcmp(parm, "-displaycontrols"))
	    displaycontrols = 1;
	else if (!strcmp(parm, "-n") || !strcmp(parm, "-newline"))
	    streamlf = 1;
	else if (!strcmp(parm, "-v") || !strcmp(parm, "-verbose"))
	    verbose = 1;
	else if (!strcmp(parm, "-300"))
	    speed = B300;
	else if (!strcmp(parm, "-600"))
	    speed = B600;
	else if (!strcmp(parm, "-1200"))
	    speed = B1200;
	else if (!strcmp(parm, "-2400"))
	    speed = B2400;
	else if (!strcmp(parm, "-4800"))
	    speed = B4800;
	else if (!strcmp(parm, "-9600"))
	    speed = B9600;
#ifdef B19200
	else if (!strcmp(parm, "-19200"))
	    speed = B19200;
#endif
#ifdef B38400
	else if (!strcmp(parm, "-38400"))
	    speed = B38400;
#endif
#ifdef B57600
	else if (!strcmp(parm, "-57600"))
	    speed = B57600;
#endif
#ifdef B115200
	else if (!strcmp(parm, "-115200"))
	    speed = B115200;
#endif
	else if (!strcmp(parm, "-e") || !strcmp(parm, "-echo"))
	    echo = 1;
	else if (*parm == '-') {
	    strcpy(result, usage);
	    return result;
	}
	else if (path == NULL)
	    path = parm;
	else if (command == NULL)
	    command = parm;
	else if (response == NULL)
	    response = parm;
	else {
	    strcpy(result, usage);
	    return result;
	}
	parm = getparm(NULL);
    }
    if (path == NULL)
#ifdef linux
	path = "/dev/cua0";
#else
	path = "/dev/tty00";
#endif
    else if (isdigit(*path)) {
#ifdef linux
	sprintf(buf, "/dev/cua%1d", atoi(path));
#else
	sprintf(buf, "/dev/tty%02d", atoi(path));
#endif
	path = buf;
    }
    else if (*path != '/') {
	if (strchr(path, ':') == NULL) {
	    sprintf(buf, "/dev/%s", path);
	    path = buf;
	}
    }

    if (setjmp(done) == 0) {
	if (strchr(path, ':') == NULL) {
	    if ((filedes = open(path, O_RDWR)) >= 0)
		setline();
	}
	else
	    filedes = opensocket(path);
	if (filedes >= 0) {
	    if (command)
		expect(command, response);
	    else
		termmain();
	    return result;
	}
	else
	    fatal("open");
    }
    return result;
}

#else

char *com(char *string)
{
    return "?Not yet implemented";
}

#endif
