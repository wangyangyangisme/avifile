/*
 visual.c for Bt848 frame grabber driver

 Copyright (C) 1996,97 Marcus Metzler (mocm@thp.uni-koeln.de)

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <avm_default.h>
#include <stdio.h>

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/ObjectP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/ShellP.h>

#include "qt_visual.h"


#define MAX_VISUAL_CLASS	6
#define MAX_VISUAL_NAME 	6

char visualName[MAX_VISUAL_NAME];

/*
 * Visual names.
 */
static const struct visual_class_name {
    int		visual_class;
    const char	*visual_name;
} visual_class_names[MAX_VISUAL_CLASS] = {
    { StaticGray,	"StaticGray"  },
    { GrayScale,	"GrayScale"   },
    { StaticColor,	"StaticColor" },
    { PseudoColor,	"PseudoColor" },
    { TrueColor,	"TrueColor"   },
    { DirectColor,	"DirectColor" }
};

/*
 * Convert a visual class to its name.
 */
const char *Vis_name(int vis)
{
    int i;

    for (i = 0; i < MAX_VISUAL_CLASS; i++) {
	if (visual_class_names[i].visual_class == vis) {
	    return visual_class_names[i].visual_name;
	}
    }
    return "UnknownVisual";
}

/*
 * List the available visuals for the default screen.
 */
void List_visuals(Display *dpy)
{
    int i,num,c;
    XVisualInfo *vinfo_ptr,my_vinfo;
    long mask;
    //Display *dpy=XtDisplay(wid);

    num = 0;
    mask = 0;
    my_vinfo.screen = DefaultScreen(dpy);
    mask |= VisualScreenMask;
    vinfo_ptr = XGetVisualInfo(dpy, mask, &my_vinfo, &num);
    printf("Listing all visuals:\n");
    for (i = 0; i < num; i++) {
	c=vinfo_ptr[i].class;
	printf("Visual class    %12s  ( %d ) \n",Vis_name(c),c);
	printf("    id              0x%02x\n", (unsigned)vinfo_ptr[i].visualid);
	printf("    screen          %8d\n", vinfo_ptr[i].screen);
	printf("    depth           %8d\n", vinfo_ptr[i].depth);
	printf("    red_mask        0x%06x\n", (unsigned)vinfo_ptr[i].red_mask);
	printf("    green_mask      0x%06x\n", (unsigned)vinfo_ptr[i].green_mask);
	printf("    blue_mask       0x%06x\n", (unsigned)vinfo_ptr[i].blue_mask);
	printf("    colormap_size   %8d\n", vinfo_ptr[i].colormap_size);
	printf("    bits_per_rgb    %8d\n", vinfo_ptr[i].bits_per_rgb);
    }
    XFree((void *) vinfo_ptr);
}


/*
 * Use suitable visual
 */
int find_visual(Display *dpy, char *visualName,Visual **v,int *visual_class)
{
    int i,num,best_depth,c_depth,visual_id,dispDepth = 0;
    XVisualInfo *vinfo_ptr,my_vinfo,*best_vinfo;
    long mask;
    char cdummy;
    //Display *dpy;
    //  Colormap colormap ;
    Boolean dual=False;

    //dpy=XtDisplay(w);
    visual_id = -1;
    *visual_class = -1;

    if (visualName) {
	if (strncmp(visualName, "0x", 2) == 0) {
	    if (sscanf(visualName, "%x", &visual_id) < 1) {
		printf("fatal: qt_visual: Bad visual id \"%s\", using default\n", visualName);
		List_visuals(dpy);
		visual_id = -1;
	    }
	} else  if (strncmp(visualName, "c", 1) == 0) {
	    if (sscanf(visualName, "%s%d",&cdummy, visual_class) < 1) {
		printf("fatal: qt_visual: Bad visual id \"%s\", using default\n", visualName);
		List_visuals(dpy);
		visual_id = -1;
	    }
	} else {
	    for (i = 0; i < MAX_VISUAL_CLASS; i++) {
		if (strncasecmp(visualName, visual_class_names[i].visual_name,
				strlen(visual_class_names[i].visual_name))
		    == 0) {
		    *visual_class = visual_class_names[i].visual_class;
		    break;
		}
	    }
	    if (*visual_class == -1) {
		printf("fatal: qt_visual: Unknown visual class named \"%s\", using default\n",
		       visualName);
		List_visuals(dpy);
	    }
	}
    }

    mask = 0;
    my_vinfo.screen = DefaultScreen(dpy);
    mask |= VisualScreenMask;
    if (*visual_class >= 0) {
	my_vinfo.class = *visual_class;
	mask |= VisualClassMask;
    }
    if (visual_id >= 0) {
	my_vinfo.visualid = visual_id;
	mask |= VisualIDMask;
    }
    num = 0;

    if ((vinfo_ptr = XGetVisualInfo(dpy, mask, &my_vinfo, &num)) == NULL
	|| num <= 0) {
	printf("fatal: qt_visual: No visuals available with class name \"%s\", using default\n",
	       visualName);
	*visual_class = -1;
    }
    else {
	best_vinfo = vinfo_ptr;
	if (best_vinfo->depth < 16) dual=True;
	for (i = 1; i < num; i++) {
	    best_depth = best_vinfo->depth;
	    c_depth = vinfo_ptr[i].depth;
	    if (c_depth < 16) dual=True;
	    if (c_depth > best_depth) best_vinfo = &vinfo_ptr[i];
	}
	*v = best_vinfo->visual;
	*visual_class = best_vinfo->class;
	dispDepth = best_vinfo->depth;
	XFree((void *) vinfo_ptr);
    }
    if (dispDepth<16 && dual) dual=False;
    if(dual){
	/*
	 colormap= XCreateColormap(dpy, DefaultRootWindow(dpy),
	 *v, AllocNone);
	 */
#ifdef myDEBUG
	printf ("Debug: qt_visual: found Dual Visual Mode using %s\n",Vis_name(*visual_class));
#endif
    } else {
#ifdef myDEBUG
	printf ("Debug: qt_visual:using Visual %s\n",Vis_name(*visual_class));
#endif
	//w->core.colormap=XDefaultColormapOfScreen(XtScreen(w));
    }

    return dispDepth;
}
#else
void List_visuals(Display *dpy) {}
#endif
