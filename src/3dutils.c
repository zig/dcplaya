/* GhettoPlay: an S3M browser and playback util
   (c)2000 Dan Potter
*/

#include <stdarg.h>
#include <stdio.h>
#include "gp.h"

#include "sintab.h"

/* Some misc 3D utils */

/* Rotate a 3-pair of coordinates by the given angle 0..255 */
void rotate(int zang, int xang, int yang, float *x, float *y, float *z) {
	float tx, ty, tz;
	
	tx = (mcos(zang)* *x - msin(zang)* *y);
	ty = (mcos(zang)* *y + msin(zang)* *x);
	*x = tx; *y = ty;
		
	tz = (mcos(xang)* *z - msin(xang)* *y);
	ty = (mcos(xang)* *y + msin(xang)* *z);
	*y = ty; *z = tz;

	tx = (mcos(yang)* *x - msin(yang)* *z);
	tz = (mcos(yang)* *z + msin(yang)* *x);
	*x = tx; *z = tz;
}

/* Draw a polygon for a shaded box; wow, a nasty looking func =) */
void draw_poly_box(float x1, float y1, float x2, float y2, float z,
		float a1, float r1, float g1, float b1,
		float a2, float r2, float g2, float b2) {
	poly_hdr_t poly;
	vertex_oc_t vert;
	
	ta_poly_hdr_col(&poly, TA_TRANSLUCENT);
	ta_commit_poly_hdr(&poly);
	
	vert.flags = TA_VERTEX_NORMAL;
	vert.x = x1; vert.y = y2; vert.z = z;
	vert.a = (a1+a2)/2; vert.r = (r1+r2/2); vert.g = (g1+g2)/2; vert.b = (b1+b2)/2;
	ta_commit_vertex(&vert, sizeof(vert));
	
	vert.y = y1;
	vert.a = a1; vert.r = r1; vert.g = g1; vert.b = b1;
	ta_commit_vertex(&vert, sizeof(vert));
	
	vert.x = x2; vert.y = y2;
	vert.a = a2; vert.r = r2; vert.g = g2; vert.b = b2;
	ta_commit_vertex(&vert, sizeof(vert));
	
	vert.flags = TA_VERTEX_EOL;
	vert.y = y1;
	vert.a = (a1+a2)/2; vert.r = (r1+r2/2); vert.g = (g1+g2)/2; vert.b = (b1+b2)/2;
	ta_commit_vertex(&vert, sizeof(vert));
}









