/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains some VMS system service routines
 *	for UNIX System V environments.
 *
 * AUTHOR(S):
 *
 *	Josef Heinen
 *	Jochen Werner
 *
 * VERSION:
 *
 *	V1.0
 *
 */

#if defined(RPC) || (defined(_WIN32) && !defined(__GNUC__))
#define HAVE_SOCKETS
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#ifdef linux
#define _XOPEN_SOURCE
#endif
#include <unistd.h>
#endif

#ifdef HAVE_SOCKETS

#if defined (cray) || defined (__SVR4) || defined (MSDOS) || defined(_WIN32)
#include <fcntl.h>
#else
#include <sys/types.h>
#include <sys/file.h>
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pwd.h>
#else
#include <windows.h>
#endif

#endif

#ifdef VMS
#include <descrip.h>
#endif

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)

#ifdef NO_TERMIO
#include <sys/ioctl.h>
#include <sgtty.h>
#else
#include <termios.h>
#endif

#if defined (cray)
#include <fcntl.h>
#else
#include <sys/types.h>
#include <sys/file.h>
#endif

#endif /* VMS, MSDOS, _WIN32 */

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef HAVE_SOCKETS
#define MAXCONN 5
#define TYPEAHEAD_BUFSIZ 132
#define CTRLC '\003'
#endif

#ifndef VMS

#define Bool	unsigned int
#define True	1
#define False	0

#define ff  '\f'
#define esc '\033'
#define ss3 '\217'		/* <SS3>, same as <ESC>O */
#define csi '\233'		/* <CSI>, same as <ESC>[ */

#define esc_inval	-1
#define esc_start	0
#define esc_inter	1
#define esc_spec	2
#define esc_param	3
#define esc_done	100


static int esc_char_table[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 5,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 6,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 7, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };

static int esc_state_table[4][8] = {
    {esc_inval, esc_inter, esc_done, esc_done,
     esc_inval, esc_inter, esc_spec, esc_param},
    {esc_inval, esc_inter, esc_done, esc_done,
     esc_inval, esc_inval, esc_done, esc_done},
    {esc_inval, esc_spec, esc_inval, esc_done,
     esc_inval, esc_inval, esc_inval, esc_done},
    {esc_inval, esc_spec, esc_param, esc_done,
     esc_inval, esc_param, esc_param, esc_done}
    };

static Bool escape = False;
static Bool cntrl = True;

#ifdef HAVE_SOCKETS

static int s = 0;
static int sd[MAXCONN] = {
    -1, -1, -1, -1, -1
    };
static int auth_nusers = 0;
static char **auth_users = NULL;

static unsigned short port = 0;

static char typeahead_buffer[TYPEAHEAD_BUFSIZ];
static int read_index = 0, write_index = 0;
static int socket_descriptor = 0;

#endif

#else

#define odd(status) ((status) & 01)

#endif /* VMS */

#include "system.h"


typedef struct {
    char *name;
    unsigned number;
    } fac_struct;

typedef struct {
    char *name;
    unsigned code;
    char *text;
    } msg_struct;


int (*lib_a_error_handler)(char *) = NULL;

static fac_struct facility[] = {

{"SYSTEM",	0x00000000},
{"RMS",		0x00000001},
{"MTH",		0x00000022},
{"C",		0x00000053},

{"CLI",		0x00000803},
{"FDV",		0x00000829},
{"FUNCTION",	0x00000851},
{"STRING",	0x00000824},
{"SYMBOL",	0x00000852},
{"TERMINAL",	0x00000853},
{"VARIABLE",	0x00000850},
{"GUS",		0x0000085B},
{"SIGHT",	0x0000085C},
{"IMAGE",	0x0000085D}
};

static int n_facilities = sizeof(facility)/sizeof(facility[0]);

static char *severity_code[] = {
    "W", "S", "E", "I", "F", "?", "?", "?"
    };

static msg_struct message[] = {

{"NORMAL",	1,	    "normal successful completion"},
{"TIMEOUT",	556,	    "device timeout"},
{"HANGUP",	716,	    "data set hang-up"},
{"CONTROLC",	1617,	    "operation completed under CTRL/C"},
{"CANCEL",	2096,	    "operation canceled"},
{"RESIGNAL",	2328,	    "resignal condition to next handler"},

{"NORMAL",	0x00010001, "normal successful completion"},
{"FNF",		0x00018292, "file not found"},
{"ACC",		0x0001C002, "file access failed"},
{"FND",		0x0001C02A, "file or directory lookup failed"},

{"NORMAL",	0x08038001, "normal successful completion"},
{"EMPTY",	0x08038009, "command line is empty"},
{"RETURN",	0x08038011, "return to caller"},
{"EOS",		0x08038019, "end of segment detected"},
{"SYMBOL",	0x08038021, "symbol assignment"},
{"VARIABLE",	0x08038029, "variable assignment"},
{"FUNCTION",	0x08038031, "function assignment"},
{"BREAK",	0x08038039, "break"},
{"TCL",		0x08038041, "Tcl command"},
{"NOCOMD",	0x08038802, "no command on line - reenter with alphabetic first\
 character"},
{"IVVERB",	0x0803880A, "unrecognized command verb - check validity and\
 spelling"},
{"IVKEYW",	0x08038812, "unrecognized command keyword - check validity and\
 spelling"},
{"IVQUAL",	0x0803881A, "unrecognized command qualifier - check validity\
 and spelling"},
{"ABVERB",	0x08038822, "ambiguous command verb - supply more characters"},
{"ABKEYW",	0x0803882A, "ambiguous command keyword - supply more characters\
 "},
{"NOKEYW",	0x08038832, "qualifier name is missing - append the name to the\
 slash"},
{"EXPSYN",	0x0803883A, "invalid expression syntax - check operators and\
 operands"},
{"SYNILLEXPR",	0x08038842, "Syntax: ill-formed expression"},
{"INSFPRM",	0x0803884A, "missing command parameters - supply all required\
 parameters"},
{"NOLBLS",	0x08038852, "label ignored - use only within command procedures\
 "},
{"USGOTO",	0x0803885A, "target of GOTO not found - check spelling and\
 presence of label"},
{"USGOSUB",	0x08038862, "target of GOSUB not found - check spelling and\
 presence of label"},
{"NOTHEN",	0x0803886A, "IF or ON statement syntax error - check placement\
 of THEN keyword"},
{"DEFOVF",	0x08038872, "too many command procedure parameters - limit to\
 eight"},
{"NOSYMBOL",	0x0803887A, "missing symbol"},
{"SYNAPOS",	0x08038882, "Syntax: \"'\" expected"},
{"ORDEXPRREQ",	0x0803888A, "ordinal expression required"},
{"MAXPARM",	0x08039000, "too many parameters - reenter command with fewer\
 parameters"},
{"NMD",		0x08039008, "no more data"},
{"INVGOTO",	0x08039010, "GOTO command not allowed on current command level\
 "},
{"INVGOSUB",	0x08039018, "GOSUB command not allowed on current command level\
 "},
{"INVRETURN",	0x08039020, "RETURN command not allowed on current command\
 level"},
{"INVGET",	0x08039028, "GET command not allowed on current command level"},
{"SKPTXT",	0x08039030, "textual information (non-numeric records) ignored\
 "},
{"EOF",		0x08039038, "file is at end-of-file"},
{"CONTROLC",	0x08039040, "input has been canceled by keyboard action"},
{"ERRDURGET",	0x08039048, "error during GET"},
{"INVARGPAS",	0x08039050, "invalid argument passed to CLI routine"},
{"INVCOMD",	0x08039058, "command not allowed on current command level"},
{"ALREADYCON",	0x08039804, "connection already done"},
{"CONFAI",	0x0803980C, "connection failure"},
{"NOCONNECT",	0x08039814, "no connection done"},
{"DISCONFAI",	0x0803981C, "disconnect failed"},
{"NOSPACE",	0x08039824, "not enough space for requested operation"},
{"COMDNYI",	0x0803982C, "command not yet implemented"},
{"RANGE",	0x08039834, "parameter out of range"},
{"NOTCL",	0x0803983C, "no Tcl interpreter"},
{"RPCERROR",	0x08039844, "RPC command error"},

{"NORMAL",	0x08298801, "normal successful completion"},
{"DO",		0x08298809, "signal event"},
{"RETURN",	0x08298811, "return from current menu"},
{"REWIND",	0x08298819, "rewind tree"},
{"QUIT",	0x08298821, "quit menu"},
{"EXIT",	0x08298829, "exit menu"},
{"SAVE",	0x08298831, "save menu"},
{"UNSAVE",	0x08298839, "unsave menu"},
{"RETAIN",	0x08298841, "retain message"},
{"HANGUP",	0x08298849, "hangup menu"},
{"ERROR",	0x08299002, "error status"},
{"INITFAI",	0x08298003, "initialization failed"},
{"ILLCALL",	0x0829A004, "recursive menu operation"},
{"FILNOTFND",	0x0829A00C, "menu description file not found"},
{"INVMENU",	0x0829A014, "invalid menu field descriptor"},
{"STRTOOLON",	0x0829A01C, "string is too long"},
{"ILLMENLEN",	0x0829A024, "menu exceeds 20 lines"},
{"OPENFAI",	0x0829A02C, "save file open failure"},
{"INVSYNENU",	0x0829A034, "invalid syntax for an enumeration type"},
{"ILLFORMAT",	0x0829A03C, "save file has illegal format"},
{"NOMENU",	0x0829A044, "no menu loaded"},
{"INVKEY",	0x0829A04C, "invalid key"},
{"UNDEFKEY",	0x0829A054, "undefined key"},
{"NODISP",	0x0829A05C, "no menu displayed"},
{"NOCOMREG",	0x0829A064, "no communication region"},
{"PARSE",	0x0829C000, "error parsing line %d, column %d"},
{"RANGE",	0x0829C008, "internal inconsistency, value out of range"},
{"INVADDR",	0x0829C010, "invalid communication region address"},
{"NONAME",	0x0829C018, "no such menu field name"},

{"NORMAL",	0x08518801, "normal successful completion"},
{"NMF",		0x08518809, "no more function"},
{"EOS",		0x08518811, "end of segment detected"},
{"CONSTANT",	0x08518819, "constant expression"},
{"UNDEFID",	0x08519002, "undefined identifier"},
{"INVCONST",	0x0851900A, "invalid constant"},
{"CONSTEXP",	0x08519012, "Syntax: constant expected"},
{"LPAREXP",	0x0851901A, "Syntax: \"(\" expected"},
{"RPAREXP",	0x08519022, "Syntax: \")\" expected"},
{"LBRACEXP",	0x0851902A, "Syntax: \"[\" expected"},
{"RBRACEXP",	0x08519032, "Syntax: \"]\" expected"},
{"EXPRESEXP",	0x0851903A, "Syntax: expression expected"},
{"EXTRACHAR",	0x08519042, "extra characters following a valid expression"},
{"IDEXP",	0x0851904A, "Syntax: identifier expected"},
{"EXISTFUN",	0x0851A004, "existing function"},
{"NONALPHA",	0x0851A00C, "Syntax: identifier must begin with alphabetic\
 character"},
{"INVFID",	0x0851A014, "Syntax: invalid function identifier"},
{"ILLFIDLEN",	0x0851A01C, "function identifier exceeds 31 characters"},
{"DUPLDCL",	0x0851A024, "duplicate declaration - name already declared as\
 variable"},
{"RESERVED",	0x0851A02C, "reserved name"},
{"UNDEFUN",	0x0851A034, "undefined function"},
{"EMPTY",	0x0851A03C, "empty string"},
{"ILLSTRLEN",	0x0851A044, "string exceeds 255 characters"},
{"NOFUNCTION",	0x0851A04C, "no functions found"},
{"FLTDIV",	0x0851A054, "floating zero divide"},
{"UNDEXP",	0x0851A05C, "undefined exponentiation"},
{"OVFEXP",	0x0851A064, "floating overflow in math library EXP"},
{"LOGZERNEG",	0x0851A06C, "logarithm of zero or negative number"},
{"SQUROONEG",	0x0851A074, "square root of negative number"},
{"INVARGMAT",	0X0851a07C, "invalid argument to math library"},
{"ILFUNCALL",	0x0851A084, "illegal function call"},
{"UNDSYMREF",	0x0851A08C, "undefined symbol reference"},

{"SUCCESS",	0x08248801, "normal successful completion"},
{"EMPTY",	0x08249803, "empty string"},
{"INTUDF",	0x0824A004, "integer underflow"},
{"INTOVF",	0x0824A00C, "integer overflow"},
{"INVSYNINT",	0x0824A014, "invalid syntax for an integer"},
{"FLTUDF",	0x0824A01C, "floating underflow"},
{"FLTOVF",	0x0824A024, "floating overflow"},
{"INVSYNREA",	0x0824A02C, "invalid syntax for a real number"},
{"INVSYNUIC",	0x0824A034, "invalid syntax for a user identification code"},
{"INVSYNPRO",	0x0824A03C, "invalid syntax for a protection specification"},

{"NORMAL",	0X08528801, "normal successful completion"},
{"NMS",		0x08528809, "no more symbol"},
{"SUPERSED",	0x08528811, "previous value has been superseded"},
{"NONALPHA",	0x0852A004, "Syntax: identifier must begin with alphabetic\
 character"},
{"INVSYMB",	0x0852A00C, "Syntax: invalid symbol"},
{"ILLSYMLEN",	0x0852A014, "symbol exceeds 31 characters"},
{"UNDSYMB",	0x0852A01C, "undefined symbol"},
{"EXISTSYM",	0x0852A024, "existing symbol"},
{"STRINGOVR",	0x0852A02C, "resulting string overflow"},
{"ILLSTRLEN",	0x0852A034, "string exceeds 255 characters"},
{"EMPTY",	0x0852A03C, "empty string"},
{"NOSYMBOL",	0x0852A044, "no symbols found"},

{"EOF",		0x08538000, "end of file detected"},
{"CANCEL",	0x08538008, "I/O operation canceled"},
{"TIMEOUT",	0x08538010, "timeout period expired"},
{"HANGUP",	0x08538018, "data set hang-up"},
{"BADPARAM",	0x08538020, "bad parameter value"},
{"NORMAL",	0x08538801, "normal successful completion"},
{"HELP",	0x08538809, "help key pressed"},
{"CONTROLC",	0x08538811, "input has been canceled by keyboard action"},
{"CONFAI",	0x0853A004, "connection failure"},
{"INVCONID",	0x0853A00C, "invalid connection identifier"},
{"DISCONFAI",	0x0853A014, "disconnect failed"},
{"CANFAI",	0x0853A01C, "cancel failed"},

{"INDNOTDEF",	0x08508000, "index not explicitly defined"},
{"CONSTANT",	0x08508008, "constant expression"},
{"NORMAL",	0x08508801, "normal successful completion"},
{"NMV",		0x08508809, "no more variable"},
{"EOS",		0x08508811, "end of segment detected"},
{"STKOVFLO",	0x0850A004, "stack overflow"},
{"STKUNDFLO",	0x0850A00C, "stack underflow"},
{"NONALPHA",	0x0850A014, "Syntax: identifier must begin with alphabetic\
 character"},
{"INVID",	0x0850A01C, "Syntax: invalid identifier"},
{"ILLIDLEN",	0x0850A024, "identifier exceeds 31 characters"},
{"DUPLDCL",	0x0850A02C, "duplicate declaration - name already declared as\
 function"},
{"RESERVED",	0x0850A034, "reserved name"},
{"INVIND",	0x0850A03C, "invalid index"},
{"UNDEFIND",	0x0850A044, "undefined index"},
{"UNDEFID",	0x0850A04C, "undefined identifier"},
{"EXISTID",	0x0850A054, "existing identifier"},
{"NOVARIABLE",	0x0850A05C, "no variables found"},
{"ILLVARDSC",	0x0850A064, "illegal variable descriptor"},
{"NME",		0x0850A06C, "no more entries"},

{"NORMAL",	0x085B8001, "normal successful completion"},
{"GKCL",	0x085B8802, "GKS not in proper state. GKS must be in one of the\
 states GKOP,WSOP,WSAC,SGOP in routine %s"},
{"NOTACT",	0x085B880A, "GKS not in proper state. GKS must be either in the\
 state WSAC or SGOP in routine %s"},
{"NOTOPE",	0x085B8812, "GKS not in proper state. GKS must be in one of the\
 states WSOP,WSAC,SGOP in routine %s"},
{"INVINTLEN",	0x085B881A, "invalid interval length for major tick-marks in\
 routine %s"},
{"INVNUMTIC",	0x085B8822, "invalid number for minor tick intervals\
 in routine %s"},
{"INVTICSIZ",	0x085B882A, "invalid tick-size in routine %s"},
{"ORGOUTWIN",	0x085B8832, "origin outside current window in routine %s"},
{"INVDIV",	0x085B883A, "invalid division for grid lines in routine %s"},
{"INVNUMPNT",	0x085B8842, "invalid number of points in routine %s"},
{"INVWINLIM",	0x085B884A, "cannot apply logarithmic transformation to current\
 window in routine %s"},
{"INVPNT",	0x085B8852, "point co-ordinates must be greater than 0 in\
 routine %s"},
{"INVZAXIS",	0x085B885A, "invalid z-axis specification in routine %s"},
{"INVANGLE",	0x085B8862, "invalid angle in routine %s"},
{"INVNUMDOM",	0x085B886A, "invalid number of domain values in routine %s"},
{"NOTSORASC",	0x085B8872, "points ordinates not sorted in ascending order in\
 routine %s"},
{"INVSMOLEV",	0x085B887A, "invalid smoothing level in routine %s"},
{"ILLFMT",	0x085B8882, "file has illegal format in routine %s"},
{"NOTPOWTWO",	0x085B888A, "number of points must be a power of two"},
{"SYNBACKSL",	0x085B8892, "Syntax \"\\\" expected"},
{"SYNLPAREN",	0x085B889A, "Syntax: \"(\" expected"},	
{"SYNRPAREN",	0x085B88A2, "Syntax: \")\" expected"},
{"SYNEXPR",	0x085B88AA, "Syntax: expression expected"},
{"SYNLBRACE",	0x085B88B2, "Syntax: \"{\" expected"},
{"SYNRBRACE",	0x085B88BA, "Syntax: \"}\" expected"},	
{"SYNCOMMA",	0x085B88C2, "Syntax: \",\" expected"},	
{"SYNLOWER",	0x085B88CA, "Syntax: \"<\" expected"},	
{"SYNGREATER",	0x085B88D2, "Syntax: \">\" expected"},	
{"SYNINVKEY",	0x085B88DA, "Syntax: invalid keyword"},
{"INARGMAT",	0x085B88E2, "invalid argument to math library"},
{"INVSCALE",	0x085B88EA, "invalid scale options in routine %s"},

{"NORMAL",	0x085C8001, "normal successful completion"},
{"ERROR",	0x085C8008, "error status"},
{"SEGOPE",	0x085C8010, "segment not closed"},
{"CLOSED",	0x085C8018, "SIGHT not open"},
{"NOSELECT",	0x085C8020, "no selection done"},
{"NOOBJECT",	0x085C8028, "no objects found"},
{"SEGCLO",	0x085C8030, "segment not open"},
{"OPENFAI",	0x085C8038, "file open failure"},
{"CLOSEFAI",	0x085C8040, "file close error"},
{"READFAI",	0x085C8048, "unrecoverable read error"},
{"INVSIFID",	0x085C8050, "invalid SIF identification"},
{"INVTYPE",	0x085C8058, "invalid SIF type"},
{"INVHEAD",	0x085C8060, "invalid header"},
{"INVGC",	0x085C8068, "invalid GC"},
{"INVOBJ",	0x085C8070, "invalid object"},
{"INVARG",	0x085C8078, "invalid argument"},

{"NORMAL",	0x085D8001, "normal successful completion"},
{"NMI",		0x085D8323, "no more images"},
{"EXISTIMG",	0x085D832B, "existing image"},
{"TOPPM",	0x085D8333, "promoting to ppm"},
{"TOPGM",	0x085D833B, "promoting to pgm"},
{"SUPERSED",	0x085D8343, "image has been superseded"},
{"NONALPHA",	0x085D8962, "Syntax: identifier must begin with alphabetic\
 character"},
{"INVIMG",	0x085D896A, "Syntax: invalid image"},
{"ILLIMGLEN",	0x085D8972, "image name exceeds 31 characters"},
{"UNDIMG",	0x085D897A, "undefined image"},
{"NOIMAGE",	0x085D8982, "no images found"},
{"NOMEM",	0x085D898A, "no more memory"},
{"CORRUPTED",	0x085D8992, "corrupted image file"},
{"OPEFAIINP",	0x085D899A, "can't open image file for reading"}, 
{"OPEFAIOUT",	0x085D89A2, "can't open image file for writing"},
{"INVGRAYLEV",	0x085D89AA, "invalid gray level"},
{"INVMAXGRAY",	0x085D89B2, "invalid maximum gray value"},
{"INVCOLLEV",	0x085D89BA, "invalid color level"},
{"INVNUMCOL",	0x085D89C2, "invalid number of colors"},
{"NOTIMGFIL",	0x085D89CA, "not an image file"},
{"INVIMGTYP",	0x085D89D2, "invalid image type"},
{"INVMODE",	0x085D89DA, "invalid display mode"},
{"INVANGLE",	0x085D89E2, "invalid angle"},
{"INVGAMMA",	0x085D89EA, "invalid gamma value"},
{"INVDIM",	0x085D89F2, "invalid dimension"},
{"INVCOLOR",	0x085D89FA, "invalid color table name"},
{"WRITERR",	0x085D8A02, "image write error"},
{"INVLEVEL",	0x085D8A0A, "invalid level for converting PBM images"},
{"INVDIRECT",	0x085D8A12, "invalid direction"},
{"INVENHANCE",	0x085D8A1A, "invalid enhance level"},
{"INVCONTR",	0x085D8A22, "invalid contrast level"},
{"OPENFAI",	0x085D8A2A, "file open failure"},
{"CLOSEFAI",	0x085D8A32, "file close failure"},
{"CLOSED",	0x085D8A3A, "IMAGE not open"}
};

static int n_messages = sizeof(message)/sizeof(message[0]);

int tt_fprintf(FILE *, char *, ...);


#ifndef VMS


#ifdef HAVE_SOCKETS

static
int open_socket(void)
{
#if defined(_WIN32) && !defined(__GNUC__)
    WORD wVersionRequested = MAKEWORD(2, 0);
    WSADATA wsaData;
#endif
    struct sockaddr_in sin;
    int s, n, opt;

#if defined(_WIN32) && !defined(__GNUC__)
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
	fprintf(stderr, "Can't find a usable WinSock DLL\n");
	return -1;
    }
#endif

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
	perror("socket");
	return -1;
    }

    opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

    if (port == 0)
	port = 0x4500;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
	perror("bind");
	close(s);
	s = -1;
    }
    else if (listen(s, 1) == -1) {
	perror("listen");
	close(s);
	s = -1;
    }

    for (n = 0; n < MAXCONN; n++)
	sd[n] = -1;

    return s;
}


#ifdef _WIN32
typedef DWORD (*dll_proc)(LPCTSTR lpszPassword, BOOL FAR *pfMatch);
#endif


static
int readline(int sd, char *string, int echo)
{
    fd_set readfds;
#ifndef _WIN32
    struct timeval timeout;
#else
    TIMEVAL timeout;
#endif
    int n = 0;

    for (;;)
    {
	FD_ZERO(&readfds);
	FD_SET(sd, &readfds);

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	if (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout) == -1)
	{
	    if (errno != EINTR)
	    {
		perror("select");
		s = -1;
		return -1;
	    }
	}
	if (FD_ISSET(sd, &readfds))
	{
	    if (recv(sd, string + n, 1, 0) == -1)
	    {
		if (errno != EINTR)
		    return -1;
	    }
	    else
	    {
		char *cp = string + n;
		if (*cp > ' ' && *cp < 127)
		{
		    if (send(sd, echo ? cp : "*", 1, 0) == -1)
			break;
		    ++n;
		}
		else if (*cp == '\b' || *cp == 127)
		{
		    if (n > 0)
		    {
			string[n--] = '\0';
			if (send(sd, "\b \b", 3, 0) == -1)
			    break;
		    }
		}
		else
		    break;
	    }
	}
	else
	    return -1;
    }
    string[n] = '\0';

    return 0;
}


static
int invalid(char *user)
{
    int i;

    for (i = 0; i < auth_nusers; i++)
    {
	if (strcmp(user, auth_users[i]) == 0)
	    return False;
    }
    return True;
}


static
int authorize(int sd)
{
    char s[BUFSIZ], password[BUFSIZ];
    char user[100], host[100];
#ifndef _WIN32
    char *env;
    struct passwd *pw;
#else
    char lpBuffer[100];
    DWORD nSize = 100;
    HINSTANCE hLib;
    dll_proc lpfnDLLProc;
    BOOL pfMatch;
#endif

#ifndef _WIN32
    if ((env = (char *)getenv("USER")) != NULL)
	strcpy(user, env);
    else
	*user = '\0';
#else
    if (GetUserName(lpBuffer, &nSize) != 0)
    {
	strcpy(user, lpBuffer);
	user[nSize] = '\0';
    }
    else
	*user = '\0';
#endif
    if (*user == '\0' || gethostname(host, 100))
	return -1;
    
    sprintf(s, "%s@%s's password: ", user, host);
    if (send(sd, s, strlen(s), 0) == -1)
	return -1;

    if (readline(sd, password, False) == -1)
	return -1;

    while (*password == '\0')
    {
	for (;;)
	{
	    sprintf(s, "\nlogin: ");
	    if (send(sd, s, strlen(s), 0) == -1)
		return -1;

	    if (readline(sd, user, True) == -1)
		return -1;
	    else if (*user)
		break;
	}
	sprintf(s, "\n%s's Password: ", user);
	if (send(sd, s, strlen(s), 0) == -1)
	    return -1;
	if (readline(sd, password, False) == -1)
	    return -1;

	if (invalid(user))
	    return -1;
    }

#ifndef _WIN32
    pw = getpwnam(user);
    if (pw != NULL)
    {
	if (strcmp((char *)crypt(password, pw->pw_passwd), pw->pw_passwd) == 0)
	    return 0;
    }
#else
    if ((hLib = LoadLibrary("C:\\WINDOWS\\SYSTEM\\MPR.DLL")) != NULL)
    {
	if ((lpfnDLLProc =
	    (dll_proc)GetProcAddress(hLib, "WNetVerifyPasswordA")) != NULL)
	{
	    if ((*lpfnDLLProc)(password, &pfMatch) == 0)
	    {
		if (pfMatch != 0)
		    return 0;
	    }
	}
	FreeLibrary(hLib);
    }
#endif
    sprintf(s, "\nPermission denied.\n");
    send(sd, s, strlen(s), 0);

    return -1;
}



static
int accept_socket(int s)
{
    int n;

    for (n = 0; n < MAXCONN; n++)
    {
	if (sd[n] == -1)
	{
	    sd[n] = accept(s, NULL, NULL);
	    if (sd[n] == -1) {
		perror("accept");
	    }
	    else if (authorize(sd[n])) {
#ifdef _WIN32
		closesocket(sd[n]);
#else
		close(sd[n]);
#endif
		sd[n] = -1;
		return -1;
	    }
	    return 0;
	}
    }
    return -1;
}



static
int read_socket(fd_set readfds, void *buffer, size_t nbytes)
{
    int cc = -1, n;

    for (n = 0; n < MAXCONN; n++)
    {
	if (sd[n] == -1)
	    continue;
	if (FD_ISSET(sd[n], &readfds))
	{
	    cc = recv(sd[n], buffer, nbytes, 0);
	    if (cc == -1)
		perror("receive");
	    else if (cc == 0)
	    {
#ifdef _WIN32
		closesocket(sd[n]);
#else
		close(sd[n]);
#endif
		sd[n] = -1;
	    }
	    else
	    {
#ifndef _WIN32
		char *cp = (char *)buffer;

		cp[cc] = '\0';
		if (strchr(cp, CTRLC) != NULL)
		{
		    kill(getpid(), SIGINT);
		    break;
		}
#endif
		socket_descriptor = sd[n];
		return cc;
	    }
	}
    }

    return cc;
}



void sys_authorize(int nusers, char **users)
{
    auth_nusers = nusers;
    auth_users = users;
}



void sys_listen(unsigned short portnum)
{
    fd_set readfds;
    char c;
    int cc = -1, n;
    struct timeval timeout;

    if (port == 0)
	port = portnum;

    if (s == 0)
	s = open_socket();

    if (s != -1)
    {
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	for (n = 0; n < MAXCONN; n++)
	    if (sd[n] != -1)
		FD_SET(sd[n], &readfds);

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	if (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout) == -1)
	{
	    if (errno != EINTR)
	    {
		perror("select");
		s = -1;
	    }
	}
	else if (FD_ISSET(s, &readfds))
	{
	    if (accept_socket(s) == 0)
	    {
		c = ff;
		cc = 1;
	    }
	}
	else
	{
	    cc = read_socket(readfds, &c, 1);
	}
	if (cc == 1)
	{
	    typeahead_buffer[write_index++] = c;
	    write_index %= TYPEAHEAD_BUFSIZ;
	}
    }
}



void sys_receive(void)
{
    if (s > 0)
	sys_listen(0);
}



int sys_socket(void)
{
    return socket_descriptor;
}



void sys_shutdown(void)
{
    int n;

    if (s > 0)
    {
	for (n = 0; n < MAXCONN; n++)
	{
	    if (sd[n] != -1)
	    {
#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif
		shutdown(sd[n], SHUT_RDWR);
#ifdef _WIN32
		closesocket(sd[n]);
#else
		close(sd[n]);
#endif
		sd[n] = -1;
	    }
	}
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
	s = -1;
    }
}

#endif


int sys_system(char *string)
{
    int result;
#ifdef HAVE_SOCKETS
    int sd, filedes, saved_fd[3];

    if ((sd = sys_socket()) > 0)
    {
	for (filedes = 0; filedes <= 2; filedes++)
	{
	    saved_fd[filedes] = dup(filedes);
	    dup2(sd, filedes);
	}
    }
#endif
    if ((result = system(string)) != 0)
	tt_fprintf (stdout, "\n");

#ifdef HAVE_SOCKETS
    if (sd > 0)
    {
	for (filedes = 0; filedes <= 2; filedes++)
	    dup2(saved_fd[filedes], filedes);
    }
#endif
    return result;
}


#ifdef HAVE_SOCKETS

static
void write_socket(void *buffer, size_t nbytes)
{
    int n;

    for (n = 0; n < MAXCONN; n++)
    {
	if (sd[n] != -1)
	{
	    if (send(sd[n], buffer, nbytes, 0) == -1)
	    {
		perror("send");
		sd[n] = -1;
	    }
	}
    }
}

#endif


#if !defined(MSDOS) && !defined(_WIN32)

#ifdef NO_TERMIO
static struct sgttyb saved_term;
static int saved_term2;
#else
static struct termios saved_term;
#endif


int SYS_ASSIGN (char *devnam, unsigned int *chan, int acmode, int mbxname)
{
    *chan = dup (0);
#ifdef HAVE_SOCKETS
    port = (unsigned short) mbxname;
#endif

#ifdef NO_TERMIO
    ioctl (*chan, TIOCGETP, &saved_term);
    ioctl (*chan, TIOCLGET, &saved_term2);
#else
    if (tcgetattr (*chan, &saved_term) < 0)
	perror ("tcgetattr");	
#endif
    cntrl = True;
    escape = False;

    return SS__NORMAL;
}



int SYS_DASSGN (int chan)
{
#ifdef NO_TERMIO
    ioctl (chan, TIOCSETN, &saved_term);
    ioctl (chan, TIOCLSET, &saved_term2);
#else
    if (tcsetattr (chan, TCSADRAIN, &saved_term) < 0)
	perror ("tcsetattr");
#endif
#ifdef _WIN32
    if (s > 0)
	WSACleanup();
#endif
    return SS__NORMAL;
}



static
int sys_read(int filedes, void *buffer, size_t nbytes)
{
#ifdef HAVE_SOCKETS
    fd_set readfds;
    int cc, n;
    char *cp = (char *)buffer;

    socket_descriptor = 0;

    if (s > 0)
    {
	cc = 0;
	while (cc < nbytes && read_index != write_index)
	{
	    cp[cc++] = typeahead_buffer[read_index++];
	    read_index %= TYPEAHEAD_BUFSIZ;
	}		
	if (cc != nbytes)
	{
	    buffer = cp + cc;
	    nbytes -= cc;
	}
	else
	    return nbytes;

	while (s > 0)
	{
	    FD_ZERO(&readfds);
	    FD_SET(filedes, &readfds);
	    FD_SET(s, &readfds);
	    for (n = 0; n < MAXCONN; n++)
		if (sd[n] != -1)
		    FD_SET(sd[n], &readfds);

	    while (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) == -1)
	    {
		if (errno != EINTR)
		{
		    perror("select");
		    s = -1;
		    return -1;
		}
	    }
	    if (FD_ISSET(filedes, &readfds))
	    {
		return read(filedes, buffer, nbytes);	 
	    }
	    else if (FD_ISSET(s, &readfds))
	    {
		char *bp = (char *)buffer;

		if (accept_socket(s) == 0)
		{
		    *bp = ff;
		    nbytes = 1;
		    return nbytes;
		}
	    }
	    else
	    {
		return read_socket(readfds, buffer, nbytes);
	    }
	}
	return -1;
    }
    else
#endif
	return read(filedes, buffer, nbytes);	 
}



static
void sys_write(int filedes, void *buffer, size_t nbytes)
{
#ifdef HAVE_SOCKETS
    write_socket(buffer, nbytes);
#endif
    write(filedes, buffer, nbytes);    
}



int SYS_QIOW (int efn, int chan, int func, short *iosb, int (*astadr)(),
    int astprm, unsigned char *p1, int p2, int p3, int p4, char *p5, int p6)
{
#ifdef NO_TERMIO
    struct sgttyb term;
    int term2;
#else
    struct termios term;
#endif
    char str[BUFSIZ];
    static int esc_state;
    int cc;

    if ((func & IO__READVBLK) == IO__READVBLK)
	{
	if (cntrl && !escape)
	    {
#ifdef NO_TERMIO
	    ioctl (chan, TIOCGETP, &term);
	    ioctl (chan, TIOCLGET, &term2);
	    saved_term = term;
	    saved_term2 = term2;

	    /* Turn off CR mapping, turn on full raw mode */

	    term.sg_flags &= ~(CRMOD);
	    term.sg_flags |= (CBREAK | RAW);

	    /* Possibly turn off echo */
	    if (func & IO_M_NOECHO)
		term.sg_flags &= ~ECHO;

	    ioctl (chan, TIOCSETN, &term);
#else
	    if (tcgetattr (chan, &term) < 0)
		perror ("tcgetattr");	
	    saved_term = term;

	    term.c_iflag &= ~(IGNCR | INLCR | ICRNL);
	    term.c_lflag &= ~(ICANON | ISIG);

	    if (func & IO_M_NOECHO)
		term.c_lflag &= ~ECHO; 

	    if (p3 == 0) {
		term.c_cc[VMIN] = p2;
		term.c_cc[VTIME] = 0;
		}
	    else {
		term.c_cc[VMIN] = 0;
		term.c_cc[VTIME] = 10*p3;
		}

	    if (tcsetattr (chan, TCSADRAIN, &term) < 0)
		perror ("tcsetattr");
#endif /* NO_TERMIO */
	    }

	if (p6 != 0)
	    sys_write (chan, p5, p6);

	do
	{
	    cc = sys_read (chan, p1, p2);
	    if (cc == 0)
	    {
		*p1 = 0xff;
		escape = False;
		cntrl = True;
		break;
	    }
	}
	while (cc != p2);

	cntrl = False;

	for (cc = 0; cc < p2; cc++)

	    switch ((char)p1[cc]) {

	    case esc: 
		esc_state = esc_start;
		escape = True;
		break;
	    case csi: 
		esc_state = esc_param; 
		escape = True;
		break;
	    case ss3: 
		esc_state = esc_param;
		escape = True;
		break;

	    default:
		if (escape) {
		    esc_state = esc_state_table[esc_state][
			esc_char_table[p1[cc]]];
		    escape = (esc_state != esc_done) && 
			(esc_state != esc_inval);
		    if (esc_state == esc_inval)
			cntrl = True;
		    }
		else if (!isprint(p1[cc] & 0177)) 
		    cntrl = True;
		else if (p1[cc] == '?')
		    cntrl = True;
		break;
	    }

	if (iosb != 0) {
	    if (*p1 == 0xff)
		iosb[0] = SS__TIMEOUT;
	    else if (*p1 == 035)	/* CTRL/] */
		iosb[0] = SS__HANGUP;
	    else
		iosb[0] = SS__NORMAL;
	    }		

	if (cntrl && !escape)
#ifdef NO_TERMIO
	    {
	    ioctl (chan, TIOCSETN, &saved_term);
	    ioctl (chan, TIOCLSET, &saved_term2);
	    }
#else
	    if (tcsetattr (chan, TCSADRAIN, &saved_term) < 0)
		perror ("tcsetattr");
#endif
	}

    else if ((func & IO__WRITEVBLK) == IO__WRITEVBLK)
	{
	switch (p4)
	    {
	    case 0x00 :
		strcpy (str, "");
		strncat (str, (char *)p1, p2);
		break;

	    case 0x24 :
		strcpy (str, PREFIX);
		strncat (str, (char *)p1, p2);
		break;

	    default :
		strcpy (str, PREFIX);
		strncat (str, (char *)p1, p2);
		strcat (str, POSTFIX);
	    }

	sys_write (chan, str, strlen(str));
	}

    return SS__NORMAL;
}



int SYS_CANCEL (int chan)
{
    return SS__NORMAL;
}



#else

#define def_attributes FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

static DWORD saved_mode, mode;
static HANDLE std_input, std_output;
static COORD saved_cursor = { 0, 0 }, cursor = { 0, 0 };
static WORD attributes = def_attributes;


int SYS_ASSIGN (char *devnam, unsigned int *chan, int acmode, int mbxname)
{
    int n;

    *chan = 0;
#ifdef HAVE_SOCKETS
    port = (unsigned short) mbxname;
#endif

    std_input = GetStdHandle (STD_INPUT_HANDLE);
    std_output = GetStdHandle (STD_OUTPUT_HANDLE);
    GetConsoleMode (std_input, &saved_mode);
    FlushConsoleInputBuffer (std_input);

    cntrl = True;
    escape = False;

    return SS__NORMAL;
}



int SYS_DASSGN (int chan)
{
    SetConsoleMode (std_input, saved_mode);

    return SS__NORMAL;
}



static unsigned char mapkey (unsigned char ch)
{
    unsigned char key;

    switch (ch)
	{
	case VK_NUMPAD0	  :
	case VK_NUMPAD1	  :
	case VK_NUMPAD2	  :
	case VK_NUMPAD3	  :
	case VK_NUMPAD4	  :
	case VK_NUMPAD5	  :
	case VK_NUMPAD6	  :
	case VK_NUMPAD7	  :
	case VK_NUMPAD8	  :
	case VK_NUMPAD9	  : key = '\200' + ch - VK_NUMPAD0; break;
	case VK_NUMLOCK	  : key = '\212'; break;
	case VK_DIVIDE	  : key = '\213'; break;
	case VK_MULTIPLY  : key = '\214'; break;
	case VK_SUBTRACT  : key = '\215'; break;
	case VK_ADD	  : key = '\220'; break;
	case VK_SEPARATOR : key = '\222'; break;
	case VK_F1	  : key = '\253'; break;
	case VK_F2	  : key = '\254'; break;
	case VK_F3	  : key = '\255'; break;
	case VK_F4	  : key = '\256'; break;
	case VK_F5	  : key = '\257'; break;
	case VK_F6	  : key = '\261'; break;
	case VK_F7	  : key = '\262'; break;
	case VK_F8	  : key = '\263'; break;
	case VK_F9	  : key = '\264'; break;
	case VK_F10	  : key = '\265'; break;
	case VK_F11	  : key = '\267'; break;
	case VK_F12	  : key = '\270'; break;
	case VK_F13	  : key = '\271'; break;
	case VK_F14	  : key = '\272'; break;
	case VK_F15	  : key = '\274'; break;
	case VK_F16	  : key = '\275'; break;
	case VK_F17	  : key = '\277'; break;
	case VK_F18	  : key = '\300'; break;
	case VK_F19	  : key = '\301'; break;
	case VK_F20	  : key = '\302'; break;
	case VK_INSERT	  : key = '\231'; break;
	case VK_DELETE	  : key = '\234'; break;
	case VK_HOME	  : key = '\232'; break;
	case VK_END	  : key = '\235'; break;
	case VK_PRIOR	  : key = '\233'; break;
	case VK_NEXT	  : key = '\236'; break;
	case VK_UP	  : key = '\244'; break;
	case VK_LEFT	  : key = '\247'; break;
	case VK_DOWN	  : key = '\245'; break;
	case VK_RIGHT	  : key = '\246'; break;
	default		  : key = 0;
	}

    return key;
}



static
int read_console(void *buffer, size_t nbytes)
{
    int cc;
    INPUT_RECORD input_event;
    DWORD num_read;
    char *s = buffer;

    cc = 0;
    while (cc < nbytes)
    {
	ReadConsoleInput(std_input, &input_event, 1, &num_read);
	if (input_event.EventType == KEY_EVENT)
	{
	    if (input_event.Event.KeyEvent.bKeyDown)
	    {
		unsigned char ch, key;

		ch = input_event.Event.KeyEvent.uChar.AsciiChar;
		key = input_event.Event.KeyEvent.wVirtualKeyCode;

		if (ch == 0 || ch >= 0xe0 || key >= 0x60)
		{
		    key = mapkey(key);
		    if (key != 0)
			ch = key;
		    else if (ch == 0)
			continue;
		}

		s[cc++] = ch;
	    }
	}
    }

    return cc;
}



static
int sys_read(void *buffer, size_t nbytes)
{
#ifdef HAVE_SOCKETS
    fd_set readfds;
    int cc, n;
    char *cp = buffer;
    INPUT_RECORD input_event;
    DWORD num_read;
    TIMEVAL timeout = { 0, 0 };

    if (s > 0)
    {
	cc = 0;
	while (cc < nbytes && read_index != write_index)
	{
	    cp[cc++] = typeahead_buffer[read_index++];
	    read_index %= TYPEAHEAD_BUFSIZ;
	}		
	if (cc != nbytes)
	{
	    buffer = cp + cc;
	    nbytes -= cc;
	}
	else
	    return nbytes;

	while (s > 0)
	{
	    PeekConsoleInput(std_input, &input_event, 1, &num_read);
	    if (num_read > 0)
	    {
		if (input_event.EventType != KEY_EVENT)
		{
		    ReadConsoleInput(std_input, &input_event, 1, &num_read);
		    continue;
		}
		else
		    return read_console(buffer, nbytes);
	    }

	    FD_ZERO(&readfds);
	    FD_SET(s, &readfds);
	    for (n = 0; n < MAXCONN; n++)
		if (sd[n] != -1)
		    FD_SET(sd[n], &readfds);

	    timeout.tv_sec = 0;
	    timeout.tv_usec = 10000; /* 10ms */

	    while (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout) == -1)
	    {
		if (errno != EINTR)
		{
		    perror("select");
		    s = -1;
		    return -1;
		}
	    }
	    if (FD_ISSET(s, &readfds))
	    {
		char *bp = buffer;

		if (accept_socket(s) == 0)
		{
		    *bp = ff;
		    nbytes = 1;
		    return nbytes;
		}
	    }
	    else
	    {
		return read_socket(readfds, buffer, nbytes);
	    }
	}
	return -1;
    }
    else
#endif
	return read_console(buffer, nbytes);	 
}



static
void sys_write(void *buffer, size_t nbytes)
{
    DWORD num_write;
    CONSOLE_SCREEN_BUFFER_INFO info;
    char *cp = buffer, s[BUFSIZ];
    int len = 0, *parm, parms[20], num_parms;
    int bold, inverse;

#ifdef HAVE_SOCKETS
    write_socket(buffer, nbytes);
#endif

    while (*cp)
    {
	if (len)
	{
	    WriteConsole(std_output, s, len, &num_write, NULL);
	    len = 0;
	}

	if (*cp == '\033')
	{
	    cp++;
	    if (*cp == '[')
	    {
		cp++;
		parm = parms;
		num_parms = 0;
		while (isdigit(*cp) || *cp == ';')
		{
		    num_parms++;
		    *parm = 0;
		    while (isdigit(*cp))
			*parm = *parm * 10 + *cp++ - '0';
		    if (*cp == ';')
		    {
			cp++;
			parm = parms + num_parms;
		    }
		    else
			break;
		}

		switch (*cp)
		{
		    case 'H':
			cp++;
			cursor.Y = num_parms > 0 ? parms[0] - 1 : 0;
			cursor.X = num_parms > 1 ? parms[1] - 1 : 0;
			SetConsoleCursorPosition(std_output, cursor);
			break;

		    case 'J':
			cp++;
			GetConsoleScreenBufferInfo(std_output, &info);
			memcpy(&cursor, &info.dwCursorPosition, sizeof(cursor));

			FillConsoleOutputAttribute(std_output, def_attributes,
			    80 - cursor.X + (24 - cursor.Y) * 80,
			    cursor, &num_write);
			FillConsoleOutputCharacter(std_output, ' ',
			    80 - cursor.X + (24 - cursor.Y) * 80,
			    cursor, &num_write);

			SetConsoleCursorPosition(std_output, cursor);
			break;

		    case 'K':
			cp++;
			GetConsoleScreenBufferInfo(std_output, &info);
			memcpy(&cursor, &info.dwCursorPosition, sizeof(cursor));

			FillConsoleOutputAttribute(std_output, def_attributes,
			    80 - cursor.X, cursor, &num_write);
			FillConsoleOutputCharacter(std_output, ' ',
			    80 - cursor.X, cursor, &num_write);

			SetConsoleCursorPosition(std_output, cursor);
			break;

		    case 'P': cp++; break;
		    case 'h': cp++; break;
		    case 'l': cp++; break;

		    case 'm':
			cp++;
			bold = inverse = 0;
			while (num_parms--)
			{
			    if (parms[num_parms] == 1)
				bold = 1;
			    else if (parms[num_parms] == 7)
				inverse = 1;
			}
			if (inverse)
			{
			    attributes = BACKGROUND_RED | BACKGROUND_GREEN |
				BACKGROUND_BLUE;
			    if (bold)
				attributes |= BACKGROUND_INTENSITY;
			}
			else
			{
			    attributes = def_attributes;
			    if (bold)
				attributes |= FOREGROUND_INTENSITY;
			}
			SetConsoleTextAttribute(std_output, attributes);
			break;

		    case 'r':
			cp++;
			cursor.Y = num_parms > 0 ? parms[0] - 1 : 0;
			cursor.X = 0;
			SetConsoleCursorPosition(std_output, cursor);
			break;
		}
	    }
	    else
	    {
		switch (*cp)
		{
		    case '=': cp++; break;
		    case '<': cp++; break;
		    case '>': cp++; break;

		    case '7':
			cp++;
			GetConsoleScreenBufferInfo(std_output, &info);
			memcpy(&saved_cursor, &info.dwCursorPosition,
			    sizeof(saved_cursor));
			break;

		    case '8':
			cp++;
			memcpy(&cursor, &saved_cursor, sizeof(cursor));
			SetConsoleCursorPosition(std_output, cursor);
			break;
		}
	    }
	}
	else
	    s[len++] = *cp++;
    }

    if (len)
	WriteConsole(std_output, s, len, &num_write, NULL);
}



int SYS_QIOW (int efn, int chan, int func, short *iosb, int (*astadr)(),
    int astprm, unsigned char *p1, int p2, int p3, int p4, char *p5, int p6)
{
    char str[BUFSIZ];
    int cc;

    if ((func & IO__READVBLK) == IO__READVBLK)
	{
	if (p6 != 0)
	    sys_write (p5, p6);

	GetConsoleMode (std_input, &saved_mode);
	mode = saved_mode & ~ENABLE_PROCESSED_INPUT & ~ENABLE_LINE_INPUT;

	/* Possibly turn off echo */
	if (func & IO_M_NOECHO)
	    mode &= ~ENABLE_ECHO_INPUT;

	SetConsoleMode (std_input, mode);

	do
	{
	    cc = sys_read (p1, p2);
	    if (cc == 0)
	    {
		*p1 = 0xff;
		break;
	    }
	}
	while (cc != p2);

	if (iosb != 0) {
	    if (*p1 == 0xff)
		iosb[0] = SS__TIMEOUT;
	    else if (*p1 == 035)	/* CTRL/] */
		iosb[0] = SS__HANGUP;
	    else
		iosb[0] = SS__NORMAL;
	    }		

	SetConsoleMode (std_input, saved_mode);
	}

    else if ((func & IO__WRITEVBLK) == IO__WRITEVBLK)
	{
	switch (p4)
	    {
	    case 0x00 :
		strcpy (str, "");
		strncat (str, (char *)p1, p2);
		break;

	    case 0x24 :
		strcpy (str, PREFIX);
		strncat (str, (char *)p1, p2);
		break;

	    default :
		strcpy (str, PREFIX);
		strncat (str, (char *)p1, p2);
		strcat (str, POSTFIX);
	    }

	sys_write (str, strlen(str));
	}

    return SS__NORMAL;
}



int SYS_CANCEL (int chan)
{
    return SS__NORMAL;
}

#endif /* MSDOS, _WIN32 */

#else


int sys_system(char *string)
{
    int result;

    if ((result = system(string)) != 0)
	tt_fprintf (stdout, "\n");

    return result;
}


#endif /* VMS */


int get_status_text (int msgid, unsigned int flags, char *bufadr)
{
    static char fac[32], sev[2], id[32], text[256];
    int i;

    strcpy (fac, "NONAME");
    for (i = 0; i < n_facilities; i++)
	if (STATUS_FAC_NO (msgid) == facility[i].number) {
	    strcpy (fac, facility[i].name);
	    break;
	    }
 
    strcpy (sev, severity_code [STATUS_SEVERITY (msgid)]);
    strcpy (id, "NOMSG");
    strcpy (text, "");

    for (i = 0; i < n_messages; i++)
	if (STATUS_COND_ID (msgid) == STATUS_COND_ID (message[i].code)) {
	    strcpy (id, message[i].name);
	    strcpy (text, message[i].text);
	    break;
	    }

    if (strlen(text) == 0)
	sprintf (text, "Message number %x", msgid);

    if (flags == 0)
	flags = STS_M_MSG_FAC | STS_M_MSG_SEV | STS_M_MSG_ID | STS_M_MSG_TEXT;

    strcpy (bufadr, "");
    if (flags & STS_M_MSG_FAC) {
	strcpy (bufadr, "%%");
	strcat (bufadr, fac);
	}

    if (flags & STS_M_MSG_SEV) {
	if (strlen(bufadr) == 0)
	    strcpy (bufadr, "%%");
	else
	    strcat (bufadr, "-");
	strcat (bufadr, sev);
	}

    if (flags & STS_M_MSG_ID) {
	if (strlen(bufadr) == 0)
	    strcpy (bufadr, "%%");
	else
	    strcat (bufadr, "-");
	strcat (bufadr, id);
	}

    if (flags & STS_M_MSG_TEXT) {
	if (strlen(bufadr) != 0) strcat (bufadr, ", ");
	strcat (bufadr, text);
	}

    return SS__NORMAL;
}


#if defined(VMS) && !defined(__DECC)

putenv (env)
    char *env;
{
    char log_name[255];
    int pos, stat;
    char equ_name[255];
    struct dsc$descriptor_s logical_name, value_string;

    strcpy (log_name, env);
    pos = str_locate(env, '=');
    log_name[pos++] = '\0';

    logical_name.dsc$b_dtype = DSC$K_DTYPE_T;
    logical_name.dsc$b_class = DSC$K_CLASS_S;
    logical_name.dsc$w_length = strlen(log_name);
    logical_name.dsc$a_pointer = log_name;

    if (pos < strlen(env))
	strcpy (equ_name, env+pos);
    else
	strcpy (equ_name, " ");

    value_string.dsc$b_dtype = DSC$K_DTYPE_T;
    value_string.dsc$b_class = DSC$K_CLASS_S;
    value_string.dsc$w_length = strlen(equ_name);
    value_string.dsc$a_pointer = equ_name;

    stat = LIB$SET_LOGICAL (&logical_name, &value_string, NULL, NULL, NULL);
    if (!odd(stat))
	LIB$SIGNAL (stat);
}

#endif /* VMS */

#if defined(_WIN32) && !defined(__GNUC__)

#include <time.h>

void sleep (wait)
    unsigned int wait;
{
    clock_t goal;

    goal = wait * 1000 + clock();
    while (goal > clock())
	;
}

#endif /* _WIN32 */



void establish (int (*error_handler)())

/*
 *  establish - establish a user-defined error handler
 */

{
    lib_a_error_handler = (int (*)(char *)) error_handler;
}



void raise_exception (unsigned int status, int narg, ...)

/*
 *  raise_exception - signal exception condition
 */

{
    char format[256], line[256];
    va_list args;

    get_status_text (status, 0, format);

    va_start (args, narg);
    vsprintf (line, format, args);
    va_end (args);

    tt_fprintf (stderr, "%s\n", line);

    if (lib_a_error_handler != NULL)
	lib_a_error_handler (line);
}
