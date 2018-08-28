/* 
    visual.h for Bt848 frame grabber driver

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

#ifndef QT_VISUAL_H
#define QT_VISUAL_H

#ifdef __cplusplus
extern "C" {
#endif

void List_visuals(Display *dpy);
const char *Vis_name(int vis);
int find_visual(Display *dpy, char *visualName,Visual **v,int *);

#ifdef __cplusplus
}
#endif

#endif /* QT_VISUAL_H */
