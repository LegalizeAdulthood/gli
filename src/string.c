/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains various string manipulation and
 *	string conversion routines.
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
#include <math.h>
#include <string.h>
#include <ctype.h>

#if !defined(VMS) && !defined(MSDOS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "strlib.h"
#include "system.h"


#define FALSE 0
#define	TRUE 1
#define	BOOL int

#define odd(status) ((status) & 01)
#define present(arg) (arg != 0)

#define TOUPPER(c) ((c) >= 'a' && (c) <= 'z' ? ((c)-'a'+'A') : (c))

#define STR_MAX 31

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#ifndef FLT_DIG
#define FLT_DIG 6
#endif
#ifndef FLT_MIN
#define FLT_MIN 2.94E-37
#endif
#ifndef FLT_MAX
#define FLT_MAX 1.7E38
#endif

static char *bin_digits = "01";
static char *oct_digits = "01234567";
static char *dec_digits = "0123456789";
static char *hex_digits = "0123456789ABCDEFabcdef";



char *str_remove (char *str, char ch, int all)

/*
 *  str_remove - remove trailing characters
 */

{
    int i;

    i = strlen(str)-1;
    while (i >= 0 && str[i] == ch)
	str[i--] = '\0';

    if (all)
	{
	i = 0;
	while (*(str + i) && str[i] == ch)
	    i++;

	if (i > 0)
	    strcpy (str, &str[i]);
	}

    return str;
}



char *str_translate (char *str, char match, char translation)

/*
 *  str_translate - translate matched characters
 */

{
    char *cp = str;

    while (*cp)
	{
	if (*cp == match)
	    *cp = translation;
	++cp;
	}
	
    return str;
}



int str_locate (char *str, char ch)

/*
 *  str_locate - locate character within a string
 */

{
    int i;

    i = 0;
    while (*(str + i) && str[i] != ch)
	i++;

    return i;
}



int str_index (char *object, char *pattern)

/*
 *  str_index - locate given pattern string within an object string
 */

{
    int i, j, k;

    for (i=0; object[i] != '\0'; i++)
	{
	for (j=i, k=0; pattern[k] != '\0' && object[j] == pattern[k]; j++, k++)
	    ;
	if (pattern[k] == '\0')
	    return i;
	}

    return (-1);
}



void str_reverse (char *str)

/*
 * str_reverse - reverse given string
 */

{
    int c, i, j, len;

    len = strlen(str);
    for (i=0, j=len-1; i < j; i++, j--)
	{
	c = str[i];
	str[i] = str[j];
	str[j] = c;
	}
}



char *str_pad (char *str, char fill, int size)

/*
 *  str_pad - pad string with fill character
 */

{
    int i, len;

    len = strlen(str);
    for (i = len; i < size; i++)
	str[i] = fill;

    if (size < 0) size = 0;
    str[size] = '\0';

    return str;
}



char *str_cap (char *str)

/*
 *  str_cap - convert string to uppercase
 */

{
    int i, len;

    len = strlen(str);
    for (i=0; i < len; i++)
	if (islower(str[i])) str[i] = toupper(str[i]);

    return str;
}



char *str_element (char *result, int element, char delimiter, char *str)

/*
 *  str_element - extract an element from a string in which the elements
 *  are separated by a specified delimiter
 */

{
    int i, j, len;

    strcpy (result, "");
    i = 0;
    j = -1;
    len = strlen(str);

    while (element > 0 && j < len)
	{
	if (str[++j] == delimiter)
	    {
	    element--;
	    if (element != 0)
		i = j+1;
	    }
	}

    if (element == 1 && j == len)
	j++;
    else
	if (element != 0)
	    i = len;

    if (i <= len)
	strncat (result, &str[i], j-i);

    return result;
}



int str_integer (char *str, int *status)

/*
 *  str_integer - convert string to integer
 */

{
    int i, sign, value, digit, base, stat;
    char *digits;

    i = 0;
    while (str[i] == ' ' || str[i] == '\t')
	i++;

    sign = 1;
    if (str[i] == '+' || str[i] == '-')
	sign = (str[i++] == '+') ? 1 : -1;

    base = 10;
    digits = dec_digits;

    if (str[i] == '0')
	{
	switch (str[++i]) {
	    case 'B' :
	    case 'b' : base = 2;  digits = bin_digits; i++; break;
	    case 'O' :
	    case 'o' : base = 8;  digits = oct_digits; i++; break;
	    case 'X' :
	    case 'x' : base = 16; digits = hex_digits; i++; break;
	    default  : break;
	    }
	}

    stat = str__success;
    value = 0;

    while (odd(stat) && str[i] && strchr (digits, str[i]))
	{
	if (isdigit(str[i]))
	   digit = str[i++]-'0';
	else if (isupper(str[i]))
	   digit = 10+str[i++]-'A';
	else if (islower(str[i]))
	   digit = 10+str[i++]-'a';

	if (value*base > INT_MAX-digit)
	    stat = (sign == 1) ? str__intovf : str__intudf;
	else
	    value = value*base + digit;
	}

    if (odd(stat))
	{
	if (str[i] == '\0')
	    {
	    if (i != 0)
		value = sign*value;
	    else
		stat = str__empty; /* empty string */
	    }
	else
	    stat = str__invsynint; /* invalid syntax for an integer */
	}

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return value;
}



float str_real (char *str, int *status)

/*
 *  str_real - convert string to real
 */

{
    int i, sign, exp_sign, exp, stat;
    float factor, value;
    BOOL mantissa, out_of_range;

    i = 0;
    while (str[i] == ' ' || str[i] == '\t')
	i++;

    sign = 1;
    value = 0;
    if (str[i] == '+' || str[i] == '-')
	sign = (str[i++] == '+') ? 1 : -1;

    mantissa = FALSE;
    while (isdigit(str[i]))
	{
	value = value*10 + str[i++]-'0';
	mantissa = TRUE;
	}

    if (str[i] == '.')
	{
	i++;
	factor = 1;
	while (isdigit(str[i]))
	    {
	    factor = factor/10;
	    value = value + (str[i++]-'0') * factor;
	    mantissa = TRUE;
	    }
	}

    exp_sign = 1;
    exp = 0;
    if ((str[i] == 'e' || str[i] == 'E' || str[i] == 'd' || str[i] == 'D') &&
	mantissa)
	{
	i++;
	if (str[i] == '+' || str[i] == '-')
	    exp_sign = (str[i++] == '+') ? 1 : -1;

	if (isdigit(str[i]))
	    {
	    exp = str[i++]-'0';
	    if (isdigit(str[i]))
		{
		    exp = exp*10 + str[i++]-'0';
		    if (isdigit(str[i]))
			exp = exp*10 + str[i++]-'0';
		}
	    }
	}

    if (value > 0 && present(status))
	out_of_range =
	    (log10 (FLT_MIN) > exp + log10 (value)) ||
	    (exp + log10 (value) > log10 (FLT_MAX));
    else
	out_of_range = FALSE;

    if (!out_of_range)
	{
	if (str[i] == '\0')
	    {
	    if (i != 0)
		{
		value = sign*value * pow(10.0, (double)(exp_sign*exp));
		stat = str__success;
		}
	    else
		stat = str__empty; /* empty string */
	    }
	else
	    stat = str__invsynrea; /* invalid syntax for a real number */
	}
    else
	stat = (exp_sign == 1) ? str__fltovf : str__fltudf;

    if (present(status))
	*status = stat;
    else
	if (!odd(stat))
	    raise_exception (stat, 0, NULL, NULL);

    return value;
}



char *str_dec (char *str, int value)

/*
 *  str_dec - convert integer value to decimal equivalent
 */

{
    int i, sign;

    if ((sign = value) < 0)
	value = -value;

    i = 0;
    do {
	str[i++] = value%10 + '0';
    } while ((value /= 10) > 0);

    if (sign < 0)
	str[i++] = '-';

    str[i] = '\0';
    str_reverse (str);

    return str;
}



char *str_flt (char *result, float value)

/*
 *  str_flt - convert real value to floating equivalent
 */

{
    static char *digit = "0123456789";

    float abs_val;
    char str[STR_MAX];
    int count, exponent, factor, mantissa;
    BOOL scientific_notation;

    if (value != 0)
	{
	abs_val = fabs(value);

	exponent = (int)(log10(abs_val) + pow(10.0, (double)(-FLT_DIG)));
	if (exponent < 0)
	    exponent--;

	factor = (FLT_DIG-1)-exponent;
	mantissa = (int)(abs_val * pow(10.0, (double)factor) + 0.5);

	strcpy (result, "");

	count = 0;
	do {
	    count++;

	    strcpy (str, result);
	    result[0] = digit[mantissa%10];
	    result[1] = '\0';
	    strcat (result, str);

	    if (count == factor)
		{
		strcpy (str, result);
		strcpy (result, ".");
		strcat (result, str);
		}

	    mantissa = mantissa/10;
	    }
	while (count != FLT_DIG);

	scientific_notation = (exponent <= 1-FLT_DIG) || (exponent >= FLT_DIG);

	if (scientific_notation || exponent < 0)
	    {
	    if (!scientific_notation)
		{
		strcpy (str, "");
		str_pad (str, '0', -exponent-1);
		strcat (str, result);
		strcpy (result, str);
		}

	    strcpy (str, "0.");
	    strcat (str, result);
	    strcpy (result, str);
	    }

	if (value < 0)
	    {
	    strcpy (str, "-");
	    strcat (str, result);
	    strcpy (result, str);
	    }

	if (strchr (result, '.') != 0)
	    {
	    str_remove (result, '0', FALSE);
	    str_remove (result, '.', FALSE);
	    }

	if (scientific_notation)
	    {
	    strcat (result, "E");
	    strcat (result, str_dec(str, exponent+1));
	    }
	}
    else
	strcpy (result, "0");

    return result;
}



char *str_ftoa (char *result, float value, float reference)

/*
 *  str_flt - convert real value to floating equivalent
 */

{
    static char *digit = "0123456789";
    char format[STR_MAX];

    float abs_val;
    char str[STR_MAX], *fcp, *cp;
    int count, exponent, factor, mantissa;
    int fdigits, digits;
    BOOL scientific_notation;

    if (value != 0)
	{
	abs_val = fabs(value);

	exponent = (int)(log10(abs_val) + pow(10.0, (double)(-FLT_DIG)));
	if (exponent < 0)
	    exponent--;

	factor = (FLT_DIG-1)-exponent;
	mantissa = (int)(abs_val * pow(10.0, (double)factor) + 0.5);

	strcpy (result, "");

	count = 0;
	fdigits = digits = 0;

	do {
	    count++;

	    strcpy (str, result);
	    result[0] = digit[mantissa%10];
	    result[1] = '\0';
	    strcat (result, str);

	    if (count == factor)
		{
		strcpy (str, result);
		strcpy (result, ".");
		strcat (result, str);
		}

	    mantissa = mantissa/10;
	    }
	while (count != FLT_DIG);

	scientific_notation = (exponent <= 1-FLT_DIG) || (exponent >= FLT_DIG);

	if (scientific_notation || exponent < 0)
	    {
	    if (!scientific_notation)
		{
		strcpy (str, "");
		str_pad (str, '0', -exponent-1);
		strcat (str, result);
		strcpy (result, str);
		}

	    strcpy (str, "0.");
	    strcat (str, result);
	    strcpy (result, str);
	    }

	if (value < 0)
	    {
	    strcpy (str, "-");
	    strcat (str, result);
	    strcpy (result, str);
	    }

	if (strchr (result, '.') != 0)
	    {
	    str_remove (result, '0', FALSE);
	    str_remove (result, '.', FALSE);
	    }

	if (scientific_notation)
	    {
	    strcat (result, "E");
	    strcat (result, str_dec(str, exponent+1));
	    }
	else
	    {
	    str_flt (format, reference);

	    if (strchr (format, 'E') == 0)

		if ((fcp = strchr (format, '.')) != 0)
		    {
		    fdigits = strlen (format) - (int)(fcp - format) - 1;

		    if ((cp = strchr (result, '.')) != 0)
			{
			digits = strlen (result) - (int)(cp - result) - 1;

			if (fdigits > digits)
			    strncat (result, "00000", fdigits - digits);
			}
		    else
			{
			strcat (result, ".");
			strncat (result, "00000", fdigits);
			}
		    }
	    }
	}
    else
	strcpy (result, "0");

    return result;
}



BOOL str_match (char *candidate, char *pattern, BOOL abbreviation)

/*
 *  str_match - compare two strings
 */

{
    register char *c, *p;
    register BOOL match, wildcard;

    c = candidate - 1;
    p = pattern - 1;

    match = TRUE;
    if (strchr (pattern, '|') != 0)
	abbreviation = FALSE;

    do
	{
	c++;
        p++;

	if (*c == '|')
	    c++;

	if (isdigit (*p))
	    abbreviation = FALSE;

	if (*p == '|')
	    {
	    abbreviation = TRUE;
	    p++;
	    }

	if (wildcard = (*p == '*'))
	    {
	    p++;
	    while (TOUPPER(*c) != TOUPPER(*p) && *c)
		c++;
	    }

        if (*c == '*')
            {
            c++;
            while (TOUPPER(*p) != TOUPPER(*c) && *p)
                p++;
            }

	if ((*p != '%' && *c) || wildcard)
	    match = (TOUPPER(*p) == TOUPPER(*c));
	}

    while (match && *c);

    if (match && !abbreviation)
	match = (!*p && !*c);

    return match;
}



static char *parse_field (char *character_set, char *wild_cards,
    char *prefix, char *postfix, char *read_ch, char *field_string)
{
    BOOL match;
    char *ch;
    char *saved_ch;
    char chars[256];


    strcpy (field_string, "");
    saved_ch = read_ch;

    ch = prefix;
    match = TRUE;
    while (match && *ch != '\0')
	{
	match = (*ch == *read_ch);
	ch++; read_ch++;
	}

    if (match)
	{
	strcpy (field_string, prefix);
	
	strcpy (chars, character_set);
	strcat (chars, wild_cards);
	while (str_locate(chars, *read_ch) != strlen(chars) && *read_ch != '\0')
	    {
	    strncat (field_string, read_ch, 1);
	    read_ch++;
	    }
  
	ch = postfix;
	match = TRUE;
	while (match && *ch != '\0')
	    {
	    match = (*ch == *read_ch);
	    ch++; read_ch++;
	    }

	if (match)
	    strcat (field_string, postfix);
	else
	    {
	    read_ch = saved_ch;
	    strcpy (field_string, "");
	    }
	}
    else
	{
	read_ch = saved_ch;
	strcpy (field_string, "");
	}
    
    return (read_ch);

}



static char *str_parse_vms (char *file_spec, char *default_spec, int field_set,
    char *result_spec)

/*
 *  str_parse_vms - parse a VMS file specification
 */

{
    char *ch;
    char field_string[256];
    char character_set[256];
    char chars[256];

    static char *alphanumeric = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_$";
    static char *numeric = "0123456789";


    strcpy (chars, "");
    
    ch = file_spec;

    strcpy (character_set, alphanumeric);
    strcat (character_set, "\" ");
    ch = parse_field (character_set, "", "", "::", ch, field_string);
    if (FNode & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_vms (default_spec, "", FNode, field_string);

	strcat (chars, field_string);
	}
    

    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "", "", ":", ch, field_string);
    if (FDevice & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_vms (default_spec, "", FDevice, field_string);

	strcat (chars, field_string);
	}

    
    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "*.%-", "[", "]", ch, field_string);
    if (FDirectory & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_vms (default_spec, "", FDirectory, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "*%-", "", "", ch, field_string);
    if (FName & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_vms (default_spec, "", FName, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "*%", ".", "", ch, field_string);
    if (FType & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_vms (default_spec, "", FType, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, numeric);
    ch = parse_field (character_set, "*%", ";", "", ch, field_string);
    if (FVersion & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_vms (default_spec, "", FVersion, field_string);

	strcat (chars, field_string);
	}


    strcpy (result_spec, chars);
    return (result_spec);
}



static char *str_parse_unix (char *file_spec, char *default_spec, int field_set,
    char *result_spec)

/*
 *  str_parse_unix - parse a UNIX file specification
 */

{
    char *ch;
    char field_string[256];
    char character_set[256];
    char chars[256];
    BOOL match;

    static char *alphanumeric = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_$";


    strcpy (chars, "");

    ch = file_spec;

    strcpy (character_set, alphanumeric);
    strcat (character_set, "\" ");
    ch = parse_field (character_set, "", "", "!/", ch, field_string);
    if (FNode & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_unix (default_spec, "", FNode, field_string);

	strcat (chars, field_string);
	}
    

    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "*%", "/", "/", ch, field_string);
    if (FDevice & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_unix (default_spec, "", FDevice, field_string);

	strcat (chars, field_string);
	}


    match = FALSE;
    do
	{
	strcpy (character_set, alphanumeric);
	strcat (character_set, "-.");
	ch = parse_field (character_set, "*%", "", "/", ch, field_string);

	if (strlen(field_string) > 0)
	    match = TRUE;

	if (FDirectory & field_set)
	    strcat (chars, field_string);
	}
    while (strlen(field_string) > 0);

    if (FDirectory & field_set)
	{
	if (!match && strlen(default_spec) > 0)
	    str_parse_unix (default_spec, "", FDirectory, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, alphanumeric);
    strcat (character_set, "-");
    ch = parse_field (character_set, "*%", "", "", ch, field_string);
    if (FName & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_unix (default_spec, "", FName, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "*%", ".", "", ch, field_string);
    if (FType & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_unix (default_spec, "", FType, field_string);

	strcat (chars, field_string);
	}


    strcpy (result_spec, chars);
    return (result_spec);
}



static char *str_parse_win (char *file_spec, char *default_spec, int field_set,
    char *result_spec)

/*
 *  str_parse_win - parse a Windows file specification
 */

{
    char *ch;
    char field_string[256];
    char character_set[256];
    char chars[256];
    BOOL match;

    static char *alphanumeric = 
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_$";


    strcpy (chars, "");

    ch = file_spec;

    strcpy (character_set, alphanumeric);
    strcat (character_set, "\" ");
    ch = parse_field (character_set, "", "", "!\\", ch, field_string);
    if (FNode & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_win (default_spec, "", FNode, field_string);

	strcat (chars, field_string);
	}
    

    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "", "", ":", ch, field_string);
    if (FDevice & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_win (default_spec, "", FDevice, field_string);

	strcat (chars, field_string);
	}


    match = FALSE;
    do
	{
	strcpy (character_set, alphanumeric);
	strcat (character_set, "-.");
	ch = parse_field (character_set, "*%", "", "\\", ch, field_string);

	if (strlen(field_string) > 0)
	    match = TRUE;

	if (FDirectory & field_set)
	    strcat (chars, field_string);
	}
    while (strlen(field_string) > 0);

    if (FDirectory & field_set)
	{
	if (!match && strlen(default_spec) > 0)
	    str_parse_win (default_spec, "", FDirectory, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, alphanumeric);
    strcat (character_set, "-");
    ch = parse_field (character_set, "*%", "", "", ch, field_string);
    if (FName & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_win (default_spec, "", FName, field_string);

	strcat (chars, field_string);
	}


    strcpy (character_set, alphanumeric);
    ch = parse_field (character_set, "*%", ".", "", ch, field_string);
    if (FType & field_set)
	{
	if (strlen(field_string) == 0 && strlen(default_spec) > 0)
	    str_parse_win (default_spec, "", FType, field_string);

	strcat (chars, field_string);
	}


    strcpy (result_spec, chars);
    return (result_spec);
}



char *str_parse (char *file_spec, char *default_spec, int field_set,
    char *result_spec)

/*
 *  str_parse - parse a file specification
 */

{
    char chars[256];

    if (field_set == FAll)
    {
	if (access (file_spec, 0) == 0)
	{
	    strcpy (result_spec, file_spec);
	    return (result_spec);
	}
    }

    strcpy (chars, file_spec);
#if defined(_WIN32) && defined(__GNUC__)
    str_translate (chars, '/', '\\');
#endif

    if (strchr (chars, '/') || strchr (default_spec, '/'))
	str_parse_unix (chars, default_spec, field_set, result_spec);
    else if (strchr (chars, '\\') || strchr (default_spec, '\\'))
	str_parse_win (chars, default_spec, field_set, result_spec);
    else
	str_parse_vms (chars, default_spec, field_set, result_spec);

    return (result_spec);
}



float str_atof (
    char *string,		/* A decimal ASCII floating-point number,
				 * optionally preceded by white space.
				 * Must have form "-I.FE-X", where I is the
				 * integer part of the mantissa, F is the
				 * fractional part of the mantissa, and X
				 * is the exponent.  Either of the signs
				 * may be "+", "-", or omitted.  Either I
				 * or F may be omitted, or both.  The decimal
				 * point isn't necessary unless F is present.
				 * The "E" may actually be an "e".  E and X
				 * may both be omitted (but not just one).
				 */
    char **endPtr		/* If non-NULL, store terminating character's
				 * address here. */
    )
{
#ifdef vax
#define maxExponent 63
    static double powersOf10[] = {
	10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32
	};
#else
#define maxExponent 511
    static double powersOf10[] = {
	10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256
	};
#endif

    int sign, expSign = FALSE;
    double fraction, dblExp, *d;
    register char *p;
    register int c;
    int exp = 0;		/* Exponent read from "EX" field. */
    int fracExp = 0;		/* Exponent that derives from the fractional
				 * part.  Under normal circumstatnces, it is
				 * the negative of the number of digits in F.
				 * However, if I is very long, the last digits
				 * of I get dropped (otherwise a long I with a
				 * large negative exponent could cause an
				 * unnecessary overflow on I alone).  In this
				 * case, fracExp is incremented one for each
				 * dropped digit. */
    int mantSize;		/* Number of digits in mantissa. */
    int decPt;			/* Number of mantissa digits BEFORE decimal
				 * point. */
    char *pExp;			/* Temporarily holds location of exponent
				 * in string. */

    /*
     * Strip off leading blanks and check for a sign.
     */

    p = string;
    while (isspace(*p)) {
	p += 1;
    }
    if (*p == '-') {
	sign = TRUE;
	p += 1;
    } else {
	if (*p == '+') {
	    p += 1;
	}
	sign = FALSE;
    }

    /*
     * Count the number of digits in the mantissa (including the decimal
     * point), and also locate the decimal point.
     */

    decPt = -1;
    for (mantSize = 0; ; mantSize += 1)
    {
	c = *p;
	if (!isdigit(c)) {
	    if ((c != '.') || (decPt >= 0)) {
		break;
	    }
	    decPt = mantSize;
	}
	p += 1;
    }

    /*
     * Now suck up the digits in the mantissa.  Use two integers to
     * collect 9 digits each (this is faster than using floating-point).
     * If the mantissa has more than 18 digits, ignore the extras, since
     * they can't affect the value anyway.
     */
    
    pExp  = p;
    p -= mantSize;
    if (decPt < 0) {
	decPt = mantSize;
    } else {
	mantSize -= 1;			/* One of the digits was the point. */
    }
    if (mantSize > 18) {
	fracExp = decPt - 18;
	mantSize = 18;
    } else {
	fracExp = decPt - mantSize;
    }
    if (mantSize == 0) {
	fraction = 0.0;
	p = string;
	goto done;
    } else {
	int frac1, frac2;
	frac1 = 0;
	for ( ; mantSize > 9; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac1 = 10*frac1 + (c - '0');
	}
	frac2 = 0;
	for (; mantSize > 0; mantSize -= 1)
	{
	    c = *p;
	    p += 1;
	    if (c == '.') {
		c = *p;
		p += 1;
	    }
	    frac2 = 10*frac2 + (c - '0');
	}
	fraction = (1.0e9 * frac1) + frac2;
    }

    /*
     * Skim off the exponent.
     */

    p = pExp;
    if ((*p == 'E') || (*p == 'e') || (*p == 'D') || (*p == 'd')) {
	p += 1;
	if (*p == '-') {
	    expSign = TRUE;
	    p += 1;
	} else {
	    if (*p == '+') {
		p += 1;
	    }
	    expSign = FALSE;
	}
	while (isdigit(*p)) {
	    exp = exp * 10 + (*p - '0');
	    p += 1;
	}
    }
    if (expSign) {
	exp = fracExp - exp;
    } else {
	exp = fracExp + exp;
    }

    /*
     * Generate a floating-point number that represents the exponent.
     * Do this by processing the exponent one bit at a time to combine
     * many powers of 2 of 10. Then combine the exponent with the
     * fraction.
     */
    
    if (exp < 0) {
	expSign = TRUE;
	exp = -exp;
    } else {
	expSign = FALSE;
    }
    if (exp > maxExponent) {
	exp = maxExponent;
    }
    dblExp = 1.0;
    for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
	if (exp & 01) {
	    dblExp *= *d;
	}
    }
    if (expSign) {
	fraction /= dblExp;
    } else {
	fraction *= dblExp;
    }

done:
    if (endPtr != NULL) {
	*endPtr = (char *) p;
    }

    if (sign) {
	return -fraction;
    }
    return fraction;
}
