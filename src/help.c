/*
 *
 * FACILITY:
 *
 *	HELP utility.
 *
 * ABSTRACT:
 *
 *	This module contains the routines to implement the HELP
 *	command.
 *
 * AUTHOR:
 *
 *	Josef Heinen	21-NOV-1990
 *
 *	Based on a module originally written by Stephen Ducharme
 *	for Digital's VAXELN toolkit.
 *
 * VERSION:
 *
 *	V1.0
 *
 */


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "strlib.h"
#include "terminal.h"

#define odd(status) ((status) & 01)

#define Boolean unsigned int
#define True 1
#define False 0

#define max_levels 8		/* maximum number of help levels */
#define topic_array_size 40	/* character length of each entry in topic
				   array */
#define record_max BUFSIZ	/* maximum length of a record to allow */

#define convert(j) (char)((j)+'0')
#ifndef max
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif


static tt_cid *tt = NULL;
static FILE *f;
static char help_array[max_levels][20];
static char topic_array[max_levels][topic_array_size];
static char output_buf[record_max];
static char current_buf[record_max], text_buf[record_max];
static char ch, end_level, level;
static Boolean unique, more;



static
void uppercase (char *result, char *str)
{
    do
	*result++ = islower(*str) ? toupper(*str) : *str;
    while (*str++);
}


static
int parse_help_topic (char *lookup_string, int j)
{
    int i;
    char topic[topic_array_size];

    i = 1;
    while (strlen(str_element (topic, i, ' ', lookup_string)))
	{
	uppercase (help_array[j], topic);
	if (*help_array[j] != '*' && *help_array[j] != '?')
	    j++;
	i++;
	}

    return (j);
}


static
int get_help_topic (int j)
{
    int i, stat;
    char prompt[record_max], lookup_string[record_max];

    if (j == 0)
	tt_printf ("\n");

    do {
	if (j == 0)
	    strcpy (prompt, "Topic? ");
	else
	    {
	    *prompt = '\0';
	    for (i=0; i<j; i++)
		{
		strcat (prompt, topic_array[i]+2*i);
		strcat (prompt, " ");
		}
	    strcat (prompt, "Subtopic? ");
	    }

	tt_get_line (tt, prompt, lookup_string, 0, True, False, NULL, &stat);

	if (odd(stat))
	    {
	    if (j == 0)
		tt_clear_disp (tt, &stat);

	    strtok (lookup_string, "\n");

	    if (*lookup_string)
		j = parse_help_topic (lookup_string, j);
	    else
		j--;
	    }
	else
	    j = -1;
	}
    while (!*lookup_string && (j >= 0));

    if (j == 1)
	tt_printf ("\n");

    return (j);
}


static
void readln ()
{
    fgets (text_buf, record_max, f);
    str_translate (text_buf, '\n', '\0');
}


static
Boolean find_help (int j, Boolean cross_levels)
{
    int extract_size, i;
    Boolean done, more;

    level = convert(j);
    end_level = convert(j + 1);

    done = more = False;
    while (!done && !feof(f))
	{
	uppercase (current_buf, text_buf);
	if (*current_buf)
	    ch = *current_buf;
	else
	    ch = ' ';

	if (isdigit(ch))
	    {
	    if (strlen(current_buf) >= 3) {
		if (ch == level)
		    {
		    if ((strncmp (current_buf+2, help_array[j-1],
			strlen(help_array[j-1])) == 0) ||
			(*help_array[j-1] == '*') || (*help_array[j-1] == '?'))
		    {
			more = True;
		    }
		    else {
			if (j == 1 && !cross_levels)
			    done = True;
			}
		    }
		else {
		    if (ch < level)
			done = True;
		    }
		}
	    }
	if (more)
	    {
	    strcpy (output_buf, "");
	    for (i=1; i<j; i++)
		strcat (output_buf, "  ");

	    extract_size = topic_array_size - strlen(output_buf);
	    if (strlen(text_buf) < extract_size)
		extract_size = strlen(text_buf);

	    strcpy (topic_array[j-1], output_buf);
	    strncat (topic_array[j-1], text_buf+2, extract_size-2);
	    done = True;
	    }

	if (!done)
	    readln ();
	}

    return (more);
}


static
void more_help (int j)
{
    int i;

    strcpy (output_buf, "");
    level = convert(j);
    end_level = convert(j - 1);
    more = True;

    while (more && !feof(f))
	{
	if ((strlen(text_buf) >= 3) && (*text_buf == level))
	    {
	    if (strlen(output_buf) + max(strlen(text_buf) - 2, 11) >= 78)
		{
		tt_more ("  %s\n", output_buf);
		strcpy (output_buf, "");
		}

	    if (!*output_buf)
		for (i=0; i<j-1; i++)
		    strcat (output_buf, "  ");

	    str_translate (text_buf+2, ' ', '\0');
	    strcat (output_buf, text_buf+2);

	    str_pad (output_buf, ' ', 11*((int)(strlen(output_buf)/11)+1));
	    }

	else if (*text_buf == end_level)

	    more = False;

	else if (isdigit(*text_buf) && (*text_buf < level))

	    more = False;

	if (more)
	    readln ();
	}

    if (*output_buf)
	tt_more ("  %s\n", output_buf);
}


static
int additional_information (int j)
{
    int i, k;
    char ch = '\0';
    Boolean done, more;

    done = False;
    more = False;
    k = j;
    if (unique)
	{
	if (k != 0)
	    {
	    for (i=0; i<k; i++)
		tt_printf ("\n%s\n", topic_array[i]);
	    }
	}
    else
	k = 0;

    tt_printf ("\n");
    for (i=0; i<=k; i++) 
	tt_printf ("  ");
    tt_printf ("%s", "Sorry, no information on ");
    for (i=0; i<=j; i++)
	tt_printf ("%s ", help_array[i]);
    tt_printf ("\n\n");

    rewind (f);
    if (k == 0)
	{
	i = 0;

	if (!feof(f))
	    {
	    tt_more ("\n  Information available:\n\n");
	    readln ();
	    more_help (1);
	    }
	}
    else
	{
	level = convert(k);
	end_level = convert(k + 1);

	i = 0;
	while (!done && (i != k))
	    {
	    readln ();
	    if (!find_help (i + 1, True))
		done = True;
	    else
		i++;
	    }

	if (!done)
	    {
	    while (more && !feof(f))
		{
		readln ();
		if (*text_buf)
		    ch = *text_buf;
		else
		    ch = ' ';
		if (isdigit(ch))
		    more = False;
		}
	    if ((ch == end_level) && (strlen(text_buf) >= 3))
		{
		tt_more ("\n");
		for (i=0; i<k; i++)
		    tt_more ("  ");
		tt_more ("%s\n\n", "Additional information available:");
		more_help (k + 1);
		tt_printf ("\n");
		}	    
	    }
	}

    return (i);
}


static
void output_help (int j)
{
    int i;
    char ch;

    more = True;

    while (more && !feof(f))
    	{
        if (*text_buf)
    	    ch = *text_buf;
        else
    	    ch = ' ';
        if (isdigit(ch))
    	    more = False;
        else
    	    {
	    if ((ch != '!') && more)
	    	{
	    	for (i=0; i<j; i++)
	    	    tt_more ("  ");
	    	tt_more ("%s\n", text_buf);
	    	}
    	    readln ();
	    }
	}

    if ((ch == end_level) && (strlen(text_buf) >=3))
	{
	tt_more ("\n");
    	for (i=0; i<j; i++)
    	    tt_more ("  ");
    	tt_more ("Additional help available:\n\n");
	more_help (j + 1);
	}
}


void lib_help (tt_cid *cid, char *topic, char *helpdb)

/*
 * help - main routine for the HELP Utility. It parses the program
 * arguments, opens the help file and drives the lookup.
 */

{
    int ignore, i, j, k;
    char *word;
    Boolean cross_levels;
    int first_failure;
    Boolean done, saved_flag;

    saved_flag = tt_b_disable_help_key;
    tt_b_disable_help_key = True;

    f = fopen (helpdb, "r");
    if (!f)
	{
	tt_printf ("help: cannot open help database\n");
	return;
	}

    tt = cid;

    j = 0;
    while (*topic && (j < max_levels))
	{
	while (*topic == ' ' || *topic == '\t')
	    topic++;

	word = topic;
	while (*topic != '\0' && *topic != ' ' && *topic != '\t')
	    topic++;
	*topic = '\0';

	if (*word)
	    {
	    uppercase (help_array[j], word);
	    j++;
	    }
	};

    while (j >= 0)
	{
	tt_clear_disp (tt, &ignore);

	rewind (f);

	first_failure = -1;
	unique = True;

	if (j == 0)
	    {
	    if (!feof(f))
		{
		tt_more ("\n  Information available:\n\n");
		readln ();
		more_help (1);
		}
	    }
	else {
	    done = False;
	    if (!feof(f))
		{
		k = 0;
		readln ();

		while (!done && (k != j))
		    {
		    if (first_failure == -1)
			cross_levels = True;
		    else
			cross_levels = False;

		    if (!find_help (k + 1, cross_levels))
			{
			if ((k != 0) && !feof(f))
			    {
			    if (first_failure == -1)
				first_failure = k;
			    else
				unique = False;
			    k = 0;
			    }
			else {
			    if (first_failure == -1)
				first_failure = 0;

			    j = additional_information (first_failure);
			    done = True;
			    }    			
			}
		    else {
			k++;
			if (!done)
			    readln ();
			}
		    }

		if (!done)
		    {
		    if (j != 1)
			{
			for (k=1; k<j; k++)
			    tt_more ("\n%s\n", topic_array[k-1]);
			tt_more ("\n");
			}
		    }

		k = j;
		while (!done)
		    {
		    tt_more ("%s\n\n", topic_array[j-1]);
		    output_help (j);
		    tt_more ("\n");

		    if (feof(f))
			done = True;

		    i = j;
		    k = j;
		    while (!done && !feof(f) && (k != j+1))
			{
			if (k == 1)
			    cross_levels = False;
			else
			    cross_levels = True;

			if (!find_help (k, cross_levels))
			    {
			    if (feof(f) || (k == 1))
				done = !done;
			    else
				if (k != 1)
				    {
				    k--;
				    if (i != 1)
					i--;
				    }
			    }
			else {
			    readln ();
			    k++;
			    }
			}

		    k--;
		    if (!done)
			{
			tt_more ("\n");
			for (k=i; k<j; k++)
			    tt_more ("%s\n\n", topic_array[k-1]);
			}
		    }
		}
	    }

	j = get_help_topic (j);
	}

    fclose (f);

    tt_b_disable_help_key = saved_flag;
}
