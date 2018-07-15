#if defined(_WIN32) && !defined(__GNUC__)
extern void __stdcall FSUB(int *, float *, char *, int);
#endif
#include <stdio.h>

void __stdcall CSUB(int *i, float *r, char *s, int slen)
{
 printf("%d %g %s\n", *i, *r, s);
 *i = 4711;
 *r = 1.234;
 strcpy(s, "Done");
}

main()
{
 int i = 4711;
 float r = 1.234;
 char *s = "Hello Fortran";
 FSUB(&i, &r, s, strlen(s));
 printf("%d %g %s\n", i, r, s);
}
