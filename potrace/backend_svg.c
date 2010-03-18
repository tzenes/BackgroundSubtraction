/* Copyright (C) 2001-2007 Peter Selinger.
   This file is part of Potrace. It is free software and it is covered
   by the GNU General Public License. See the file COPYING for details. */

/* $Id: backend_svg.c 147 2007-04-09 00:44:09Z selinger $ */

/* The SVG backend of Potrace. */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
extern "C"{
#include "potracelib.h"
#include "curve.h"
//#include "main.h"
#include "backend_svg.h"
#include "lists.h"
#include "auxiliary.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
}
/* ---------------------------------------------------------------------- */
/* path-drawing auxiliary functions */

/* coordinate quantization */
static inline point_t unit(dpoint_t p) {
  point_t q;

  q.x = (long)(floor(p.x*1+.5));
  q.y = (long)(floor(p.y*1+.5));
  return q;
}

static point_t cur;
static char lastop = 0;
static int column = 0;
static int newline = 1;

static void shiptoken(FILE *fout, char *token) {
  int c = strlen(token);
  if (!newline && column+c+1 > 75) {
    fprintf(fout, "\n");
    column = 0;
    newline = 1;
  } else if (!newline) {
    fprintf(fout, " ");
    column++;
  }
  fprintf(fout, "%s", token);
  column += c;
  newline = 0;
}

static void ship(FILE *fout, char *fmt, ...) {
  va_list args;
  static char buf[4096]; /* static string limit is okay here because
			    we only use constant format strings - for
			    the same reason, it is okay to use
			    vsprintf instead of vsnprintf below. */
  char *p, *q;

  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  buf[4095] = 0;
  va_end(args);

  p = buf;
  while ((q = strchr(p, ' ')) != NULL) {
    *q = 0;
    shiptoken(fout, p);
    p = q+1;
  }
  shiptoken(fout, p);
}

static void svg_moveto(FILE *fout, dpoint_t p) {
  cur = unit(p);

  ship(fout, "M%ld %ld", cur.x, cur.y);
  lastop = 'M';
}

static void svg_rmoveto(FILE *fout, dpoint_t p) {
  point_t q;

  q = unit(p);
  ship(fout, "m%ld %ld", q.x-cur.x, q.y-cur.y);
  cur = q;
  lastop = 'm';
}

static void svg_lineto(FILE *fout, dpoint_t p) {
  point_t q;

  q = unit(p);

  if (lastop != 'l') {
    ship(fout, "l%ld %ld", q.x-cur.x, q.y-cur.y);
  } else {
    ship(fout, "%ld %ld", q.x-cur.x, q.y-cur.y);
  }
  cur = q;
  lastop = 'l';
}

static void svg_curveto(FILE *fout, dpoint_t p1, dpoint_t p2, dpoint_t p3) {
  point_t q1, q2, q3;

  q1 = unit(p1);
  q2 = unit(p2);
  q3 = unit(p3);

  if (lastop != 'c') {
    ship(fout, "c%ld %ld %ld %ld %ld %ld", q1.x-cur.x, q1.y-cur.y, q2.x-cur.x, q2.y-cur.y, q3.x-cur.x, q3.y-cur.y);
  } else {
    ship(fout, "%ld %ld %ld %ld %ld %ld", q1.x-cur.x, q1.y-cur.y, q2.x-cur.x, q2.y-cur.y, q3.x-cur.x, q3.y-cur.y);
  }
  cur = q3;
  lastop = 'c';
}

/* ---------------------------------------------------------------------- */
/* functions for converting a path to an SVG path element */

/* Explicit encoding. If abs is set, move to first coordinate
   absolutely. */
static int svg_path(FILE *fout, potrace_curve_t *curve, int abs) {
  int i;
  dpoint_t *c;
  int m = curve->n;

  c = curve->c[m-1];
  if (abs) {
    svg_moveto(fout, c[2]);
  } else {
    svg_rmoveto(fout, c[2]);
  }

  for (i=0; i<m; i++) {
    c = curve->c[i];
    switch (curve->tag[i]) {
    case POTRACE_CORNER:
      svg_lineto(fout, c[1]);
      svg_lineto(fout, c[2]);
      break;
    case POTRACE_CURVETO:
      svg_curveto(fout, c[0], c[1], c[2]);
      break;
    }
  }
  newline = 1;
  shiptoken(fout, "z");
  return 0;
}

/* produce a jaggy path - for debugging. If abs is set, move to first
   coordinate absolutely. If abs is not set, move to first coordinate
   relatively, and traverse path in the opposite direction. */
static int svg_jaggy_path(FILE *fout, point_t *pt, int n, int abs) {
  int i;
  point_t cur, prev;
  
  if (abs) {
    cur = prev = pt[n-1];
    svg_moveto(fout, dpoint(cur));
    for (i=0; i<n; i++) {
      if (pt[i].x != cur.x && pt[i].y != cur.y) {
	cur = prev;
	svg_lineto(fout, dpoint(cur));
      }
      prev = pt[i];
    }
    svg_lineto(fout, dpoint(pt[n-1]));
  } else {
    cur = prev = pt[0];
    svg_rmoveto(fout, dpoint(cur));
    for (i=n-1; i>=0; i--) {
      if (pt[i].x != cur.x && pt[i].y != cur.y) {
	cur = prev;
	svg_lineto(fout, dpoint(cur));
      }
      prev = pt[i];
    }
    svg_lineto(fout, dpoint(pt[0]));
  }
  newline = 1;
  shiptoken(fout, "z");
  return 0;
}

static void write_paths_opaque(FILE *fout, potrace_path_t *tree, int color, float importance = 0) {
  potrace_path_t *p, *q;
  int c;
  
  for (p=tree; p; p=p->sibling) {
    if (0) {
      fprintf(fout, "<g>\n");
      fprintf(fout, "<g>\n");
    }
    
    c = fprintf(fout, "<path fill=\"#%06x\" stroke-width=\"2\" stroke=\"#%06x\" importance=\"%0.4f\" d=\"", color, color, importance);
    column = c;
    newline = 1;
    lastop = 0;
    if (0) {
      svg_jaggy_path(fout, p->priv->pt, p->priv->len, 1);
    } else {
      svg_path(fout, &p->curve, 1);
    }
    fprintf(fout, "\"/>\n");
    for (q=p->childlist; q; q=q->sibling) {
        
      c = fprintf(fout, "<path fill=\"#%06x\" stroke-width=\"2\" stroke=\"#%06x\" importance=\"%0.4f\" d=\"", color, color, importance);
      column = c;
      newline = 1;
      lastop = 0;
      if (0) {
         svg_jaggy_path(fout, q->priv->pt, q->priv->len, 1);
      } else {
	     svg_path(fout, &q->curve, 1);
      }
      fprintf(fout, "\"/>\n");
    }
    if (0) {
      fprintf(fout, "</g>\n");
    }
    for (q=p->childlist; q; q=q->sibling) {
      write_paths_opaque(fout, q->childlist, color);
    }
    if (0) {
      fprintf(fout, "</g>\n");
    }
  }
}

static void write_paths_transparent(FILE *fout, potrace_path_t *tree) {
  potrace_path_t *p, *q;
  int c;

  for (p=tree; p; p=p->sibling) {
    if (1) {
      fprintf(fout, "<g>\n");
    }
    c = fprintf(fout, "<path d=\"");
    column = c;
    newline = 1;
    lastop = 0;
    if (0) {
      svg_jaggy_path(fout, p->priv->pt, p->priv->len, 1);
    } else {
      svg_path(fout, &p->curve, 1);
    }
    for (q=p->childlist; q; q=q->sibling) {
      if (0) {
	svg_jaggy_path(fout, q->priv->pt, q->priv->len, 0);
      } else {
	svg_path(fout, &q->curve, 0);
      }
    }
    fprintf(fout, "\"/>\n");
    for (q=p->childlist; q; q=q->sibling) {
      write_paths_transparent(fout, q->childlist);
    }
    if (1) {
      fprintf(fout, "</g>\n");
    }
  }
}

/* ---------------------------------------------------------------------- */
/* Backend. */

/* public interface for SVG */
int page_svg(FILE *fout, potrace_path_t *plist, int width, int height, int color) {

  //info = infoin;
  /*
  int bboxx = (int)ceil(info->trans.bb[0]+info->lmar+info->rmar);
  int bboxy = (int)ceil(info->trans.bb[1]+info->tmar+info->bmar);
  double origx = info->trans.orig[0] + info->lmar;
  double origy = bboxy - info->trans.orig[1] - info->bmar;
  double scalex = info->width / info->pixwidth / info.unit;
  double scaley = -info->height / info->pixheight / info.unit;
  */
  int bboxx = width;
  int bboxy = height;
  
  /* header */
  fprintf(fout, "<?xml version=\"1.0\" standalone=\"no\"?>\n");
  fprintf(fout, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\"\n");
  fprintf(fout, " \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n");

  /* set bounding box and namespace */
  fprintf(fout, "<svg version=\"1.0\" xmlns=\"http://www.w3.org/2000/svg\"\n");
  fprintf(fout, " width=\"%dpt\" height=\"%dpt\" viewBox=\"0 0 %d %d\"\n", 
	  bboxx, bboxy, bboxx, bboxy);
  fprintf(fout, " preserveAspectRatio=\"xMidYMid meet\">\n");

  /* metadata: creator */
  fprintf(fout, "<metadata>\n");
  fprintf(fout, "Created in conjunction with \"POTRACE\" \"VERSION\"\n");
  fprintf(fout, "</metadata>\n");

  /* use a "group" tag to establish coordinate system and style */
  fprintf(fout, "<g transform=\"");/*
  if (origx != 0 || origy != 0) {
    fprintf(fout, "translate(%.0f,%.0f) ", origx, origy);
  }
  if (info.angle != 0) {
    fprintf(fout, "rotate(%.2f) ", info.angle);
  }*/
  fprintf(fout, "scale(%f,%f)", 1.f, 1.f);
  fprintf(fout, "\"\n");
  fprintf(fout, "fill=\"#%06x\" stroke=\"none\">\n", color);


  write_paths_opaque(fout, plist, color);


  /* write footer */
  fprintf(fout, "</g>\n");
  fprintf(fout, "</svg>\n");
  fflush(fout);

  return 0;
}

int page_svg_open(FILE *fout, int width, int height){
      int bboxx = width;
      int bboxy = height;
  
      /* header */
      fprintf(fout, "<?xml version=\"1.0\" standalone=\"no\"?>\n");
      fprintf(fout, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20010904//EN\"\n");
      fprintf(fout, " \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n");

      /* set bounding box and namespace */
      fprintf(fout, "<svg version=\"1.0\" xmlns=\"http://www.w3.org/2000/svg\"\n");
      fprintf(fout, " width=\"%dpt\" height=\"%dpt\" viewBox=\"0 0 %d %d\"\n", 
	                bboxx, bboxy, bboxx, bboxy);
      fprintf(fout, " preserveAspectRatio=\"xMidYMid meet\">\n");

      /* metadata: creator */
      fprintf(fout, "<metadata>\n");
      fprintf(fout, "Created in conjunction with \"POTRACE\" \"VERSION\"\n");
      fprintf(fout, "</metadata>\n");
      
      
      
      /* use a "group" tag to establish coordinate system and style */
      fprintf(fout, "<g transform=\"");/*
      if (origx != 0 || origy != 0) {
         fprintf(fout, "translate(%.0f,%.0f) ", origx, origy);
      }
      if (info.angle != 0) {
         fprintf(fout, "rotate(%.2f) ", info.angle);
      }*/
      fprintf(fout, "scale(%f,%f)", 1.f, 1.f);
      fprintf(fout, "\"\n");
      fprintf(fout, "stroke=\"none\">\n");

      /* background color is specified here */
      fprintf(fout, "<rect width=\"%d\" height=\"%d\" fill=\"0x888888\" />\n", width, height);

      fflush(fout);
      return 0;
}

int page_svg_insert(FILE *fout, potrace_path_t *plist, int color, float importance){
    write_paths_opaque(fout, plist, color, importance);
}

int page_svg_close(FILE *fout){
      /* write footer */
      fprintf(fout, "</g>\n");
      fprintf(fout, "</svg>\n");
      fflush(fout);

      return 0; 
}
