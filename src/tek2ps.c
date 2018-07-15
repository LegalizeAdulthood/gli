
#include <stdio.h>
#include <string.h>

#include "gus.h"
#include "gks.h"

#define POSTSCRIPT 62

int main(int argc, char **argv)
{
  Gasfs asfs = {
    GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL,
    GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL,
    GINDIVIDUAL, GINDIVIDUAL, GINDIVIDUAL
  };
  Gint output = 1, wstype = 62;
  char buffer[BUFSIZ];
  int len, clear_flag = 0;

  gopengks(stdout, 0);
  gsetasf(&asfs);

  gopenws(output, (char *)NULL, &wstype);
  gactivatews(output);

  gus_plot10_initt();
  memset((void *)buffer, 0, BUFSIZ);

  while (fgets(buffer, BUFSIZ, stdin))
    {
      len = strlen(buffer);
      gus_plot10_adeout(&len, buffer, &clear_flag);
      memset((void *)buffer, 0, BUFSIZ);
    }

  gus_plot10_finitt();
  gemergencyclosegks();

  return 0;
}
