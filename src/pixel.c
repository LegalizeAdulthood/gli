#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_WIN32) && !defined(__GNUC__)
#define STDCALL __stdcall
#else
#ifdef STDCALL
#undef STDCALL
#endif
#define STDCALL
#endif

#include "gksdefs.h"
#include "frtl.h"

#define NINT(a) ((int) ((a) + 0.5))

static
void glint(int dinp, int *inp, int doutp, int *outp)
{
  int i, j, k, n;
  float ratio, delta;

  n = (doutp + 1) / dinp;
  ratio = 1.0 / n;

  j = (n + 1) / 2;
  for (k = 0; k < j; k++)
    outp[k] = inp[0];

  for (i = 0; i < dinp - 1; i++)
    {
      delta = ratio * (inp[i + 1] - inp[i]);
      for (k = 1; k <= n; k++)
	outp[j++] = inp[i] + NINT(k * delta);
    }

  for (k = j; k < doutp; k++)
    outp[k] = inp[dinp - 1];
}

void STDCALL GPIXEL(
  float xmin, float xmax, float ymin, float ymax,
  int dx, int dy, int *colia, int w, int h, int *pixmap,
  int dwk, int *wk1, int *wk2)
{
  int i, j, ix, nx;
  int sx = 1, sy = 1;

  if ((w + 1) % dx != 0 || (h + 1) % dy != 0)
    {
      printf("***   IMPROPER INPUT PARAMETER VALUE(S).\n");
      printf("     H = %d  W = %d\n", h, w);
      printf("    DX = %d, DY= %d\n", dx, dy);
      printf(" ERROR DETECTED IN ROUTINE   GPIXEL\n\n");
      return;
    }

  ix = 0;
  nx = (w + 1) / dx;

  for (i = 0; i < dx; i++)
    {
      for (j = 0; j < dy; j++)
	wk1[j] = colia[i + j * dx];

      glint(dy, wk1, h, wk2);
      for (j = 0; j < h; j++)
	pixmap[ix + j * w] = wk2[j];

      ix += nx;
    }

  for (j = 0; j < h; j++)
    {
      ix = 0;
      for (i = 0; i < dx; i++)
	{
	  wk1[i] = pixmap[ix + j * w];
	  ix += nx;
	}

      glint(dx, wk1, w, wk2);
      for (i = 0; i < w; i++)
	pixmap[i + j * w] = wk2[i];
    }

  GCA(&xmin, &ymin, &xmax, &ymax, &w, &h, &sx, &sy, &w, &h, pixmap);
}
