/* Copyright (C) 2001-2007 Peter Selinger.
   This file is part of Potrace. It is free software and it is covered
   by the GNU General Public License. See the file COPYING for details. */

/* $Id: backend_svg.h 147 2007-04-09 00:44:09Z selinger $ */

#ifndef BACKEND_SVG_H
#define BACKEND_SVG_H

#include "potracelib.h"
//#include "main.h"
/* structure to hold a dimensioned value */
struct dim_s {
  double x; /* value */
  double d; /* dimension (in pt), or 0 if not given */
};
typedef struct dim_s dim_t;

#define DIM_IN (72)
#define DIM_CM (72 / 2.54)
#define DIM_MM (72 / 25.4)
#define DIM_PT (1)

/* set some regional defaults */

#ifdef USE_METRIC
#define DEFAULT_DIM DIM_CM
#define DEFAULT_DIM_NAME "centimeters"
#else
#define DEFAULT_DIM DIM_IN
#define DEFAULT_DIM_NAME "inches"
#endif

#ifdef USE_A4
#define DEFAULT_PAPERWIDTH 595
#define DEFAULT_PAPERHEIGHT 842
#define DEFAULT_PAPERFORMAT "a4"
#else
#define DEFAULT_PAPERWIDTH 612
#define DEFAULT_PAPERHEIGHT 792
#define DEFAULT_PAPERFORMAT "letter"
#endif



/* structure to hold command line options */
struct info_s {
  struct backend_s *backend;  /* type of backend (eps,ps,pgm etc) */
  potrace_param_t *param;  /* tracing parameters, see potracelib.h */
  int debug;         /* type of output (0-2) (for BACKEND_PS/EPS only) */
  dim_t width_d;     /* desired width of image */
  dim_t height_d;    /* desired height of image */
  double rx;         /* desired x resolution (in dpi) */
  double ry;         /* desired y resolution (in dpi) */
  double sx;         /* desired x scaling factor */
  double sy;         /* desired y scaling factor */
  double stretch;    /* ry/rx, if not otherwise determined */
  dim_t lmar_d, rmar_d, tmar_d, bmar_d;   /* margins */
  double angle;      /* rotate by this many degrees */
  int paperwidth, paperheight;  /* paper size for ps backend (in pt) */
  double unit;       /* granularity of output grid */
  int compress;      /* apply compression? */
  int pslevel;       /* postscript level to use: affects only compression */
  int color;         /* rgb color code 0xrrggbb: line color */
  int fillcolor;     /* rgb color code 0xrrggbb: fill color */
  double gamma;      /* gamma value for pgm backend */
  int longcoding;    /* do not optimize for file size? */
  char *outfile;     /* output filename, if given */
  char **infiles;    /* array of input filenames */
  int infilecount;   /* number of input filenames */
  double blacklevel; /* 0 to 1: black/white cutoff in input file */
  int invert;        /* invert bitmap? */
  int opaque;        /* paint white shapes opaquely? */
  int group;         /* group paths together? */
  int progress;      /* should we display a progress bar? */
};
typedef struct info_s info_t;

//info_t info;
int page_svg_open(FILE *fout, int width, int height);
int page_svg(FILE *fout, potrace_path_t *plist, int width, int height, int color);
int page_svg_insert(FILE *fout, potrace_path_t *plist, int color, float importance = 0);
int page_svg_close(FILE *fout);
#endif /* BACKEND_SVG_H */

