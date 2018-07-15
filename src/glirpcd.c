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
 *      This sample program demonstrates GLI's interapplication
 *      communication features using Sun RPCs. GLI can call remote
 *      procedures and exchange data with them. GLI provides an
 *      easy-to-use call interface to register a user-definable
 *      procedure for use in an RPC environment.
 *
 * AUTHOR:
 *
 *      Josef Heinen
 *
 * VERSION:
 *
 *      V1.0
 *
 * COMPILATION:
 *
 *      % cc -c glirpcd.c
 *      % cc -o glirpcd glirpcd.o $(GLI_HOME)/libgli.a
 *      % glirpcd &
 *
 *      Note: This program requires that your machine supports Sun RPCs
 *      and that a portmapper is running on your machine.
 *
 * USAGE:
 *
 *      GLI> x := 1..100
 *      GLI> rpc "read path" x
 *
 *      This example reads 100 values from file 'path' into the variable x.
 *      Note that the variable has to be defined previous to invoking the
 *      RPC command.
 *
 * EXAMPLE:
 *
 *      For more sophisticated use of RPC daemons from GLI, you may specify
 *      the service you intend to use. A 'service' identification consists
 *      of a hostname and a service number in the form '[host][:[number]]'.
 *
 *      sender% glirpcd :1
 *
 *      receiver% glirpcd :1
 *
 *      myhost% gli
 *      GLI> y := 1..1000
 *      GLI> def log GLI_SERVICE "sender:1"
 *      GLI> rpc "read tmpfile" y
 *      GLI> def log GLI_SERVICE "receiver:1"
 *      GLI> rpc "write tmpfile" y
 *
 *      This examples shows how to copy a file containing binary float
 *      numbers from host 'sender' to host 'receiver'.
 *
 * PROCEDURE ARGUMENTS:
 *
 *      message     A character string representing the first parameter
 *                  to the GLI RPC command. This string should tell the
 *                  daemon which action to perform. The string should
 *                  not be changed.
 *      argc        The number of variables passed (readonly).
 *      sizes       An array of argc elements containing the number of
 *                  values for each variable. GLI may truncate any
 *                  variable if its size has changed.
 *      data        An array pointing to the variables data space (as
 *                  specified in the GLI command line).
 *
 */


#include <stdio.h>
#include <string.h>

#define PATHLEN 100

void gli_registerrpc (
    int argc, char **argv, void (*proc)(char *, int, int *, float **));


static
void my_proc (char *message, int argc, int *sizes, float **data)
{
    char path[PATHLEN], *str;
    FILE *file;
    int i, n;

    if (!strncmp(message, "read", 4))
    {
        /* Read data from a binary file */
        if (str = strchr (message, ' '))
        {                        
            strcpy (path, ++str);
            if (file = fopen (path, "rb"))
            {
                n = 0;
                for (i = 0; i < argc; i ++)
                {
                    sizes[i] = fread (data[i], sizeof (float), sizes[i], file);
                    n += sizes[i];
                }
                fclose (file);
                sprintf (message, "%d value(s) read from file %s", n, path);
            }
            else
                sprintf (message, "?Can't open '%s' for reading", path);
        }
        else
            sprintf (message, "?Missing file specification");
    }

    else if (!strncmp(message, "write", 5))
    {
        /* Write data to a binary file */
        if (str = strchr (message, ' '))
        {                        
            strcpy (path, ++str);
            if (file = fopen (path, "wb"))
            {
                n = 0;
                for (i = 0; i < argc; i ++)
                    n += fwrite (data[i], sizeof (float), sizes[i], file);
                fclose (file);
                sprintf (message, "%d value(s) written to file %s", n, path);
            }
            else
                sprintf (message, "?Can't open '%s' for writing", path);
        }
        else
            sprintf (message, "?Missing file specification");
    }
    else
        sprintf (message, "?Unknown command");
}


int main (int argc, char **argv)
{
    gli_registerrpc (argc, argv, my_proc);
    return 0;
}

