/* KallistiOS 1.1.5

   ta.c
   (c)2000-2001 Dan Potter
   
*/

/*

Improved by Vincent Penne.

Now the render_finish flag of the TA is correctly used.
We can exploit the remaining time between ta_finish_frame and
ta_begin_render correctly, using the three buffer flipping of the DC
(2 screen buffers + the TA list buffer).
So ta_finish_frame does not anymore wait for the vertical blanking, 
while ta_begin_render now wait that rendering has finished.

RENDER TO TEXTURE WILL NOT WORK ANYMORE FOR NOW, SORRY :((

*/

static char id[] = "KOS $Id: ta.c,v 1.2 2004-07-31 22:55:18 vincentp Exp $";

#include <stdio.h>
#include <arch/types.h>
#include <dc/video.h>
#include "dc/ta.h"
#include "dc/g2.h"
#include <dc/asic.h>
#include <arch/irq.h>
#include <kos/thread.h>
#include <kos/sem.h>

#define thd_enabled thd_mode

/*

This module handles access to the Tile Accelerator, also known as the
common access point of the 3D chip of the DC. Currently it supports
only one mode of access, which is the direct-to-TA mode using two
lists (one opaque, one translucent). This support will be improved
and expanded in the future.

This module makes use of the PVR interrupts to handle responsive page flipping
in the rendering process. Make sure that IRQ9 is enabled or a call to
ta_finish_frame() will freeze your program.

Note: Inviting the TA routines into your program will render the normal frame
buffer access defunct unless you know where to access it and set the
start address appropriately.

WARNING: Do not disable interrupts and then call ta_finish_frame(). This
will cause a deadlock condition and unless you're in a seperate thread,
the kernel will halt.

Many thanks to Maiwe for his input, which has clarified a lot of the
buffer setup stuff, and for ta-intro.txt that explains the poly registration
process. Also thanks to Marcus Comstedt for explaining the 32/64-bit texture
memory thing.

*/

/* Global background data structure; this is used during the rendering process. */
static ta_bkg_poly ta_bkg_internal;
ta_bkg_poly *ta_bkg = &ta_bkg_internal;


/* TA lists that you can activate */
#define TA_LIST_OPAQUE_POLYS	1
#define TA_LIST_TRANS_POLYS	2
#define TA_LIST_OPAQUE_MODS	4
#define TA_LIST_TRANS_MODS	8
#define TA_LIST_PUNCH_THRU	16

/* Polygon buffer sizes */
#define TA_POLYBUF_0		0
#define TA_POLYBUF_8		8
#define TA_POLYBUF_16		16
#define TA_POLYBUF_32		32

uint32 render_counter;
uint32 render_counter2;
void (* ta_render_done_cb) ();
int ta_block_render;

void (* ta_scan_cb) ();

/* TA state structure */
ta_state_t ta_state;

/* Current page */
int ta_curpage = 0;

/* 3d-specific parameters; these are all about rendering and nothing
   to do with setting up the video, although these do currently assume
   a 640x480x16bit screen. Some stuff in here is still unknown. */
static uint32 three_d_parameters[] = {
	0x80a8, 0x15d1c951,	/* M (Unknown magic value) */
	0x80a0, 0x00000020,	/* M */
	0x8008, 0x00000000,	/* TA out of reset */
	0x8048, 0x00000009,	/* alpha config */
	0x8068, 0x02800000,	/* pixel clipping x */
	0x806c, 0x01e00000,	/* pixel clipping y */
	0x8110, 0x00093f39,	/* M */
	0x8098, 0x00800408,	/* M */
	0x804c, 0x000000a0,	/* display align (640*2)/8 */
	0x8078, 0x3f800000,	/* polygon culling (1.0f) */
	0x8084, 0x00000000,	/* M */
	0x8030, 0x00000101,	/* M */
	0x80b0, 0x007f7f7f,	/* Fog table color */
	0x80b4, 0x007f7f7f,	/* Fog vertex color */
	0x80c0, 0x00000000,	/* color clamp min */
	0x80bc, 0xffffffff,	/* color clamp max */
	0x8080, 0x00000007,	/* M */
	0x8074, 0x00000001,	/* cheap shadow */
	0x807c, 0x0027df77,	/* M */
	0x8008, 0x00000001,	/* TA reset */
	0x8008, 0x00000000,	/* TA out of reset */
	0x80e4, 0x00000000,	/* stride width */
#if 0
	0x6884, 0x00000000,	/* Disable all interrupt events */
	0x6930, 0x00000000,
	0x6938, 0x00000000,
	0x6900, 0xffffffff,	/* Clear all pending int events */
	0x6908, 0xffffffff,
#endif
	0x80b8, 0x0000ff07,	/* fog density */
	0x80b4, 0x007f7f7f,	/* fog vertex color */
	0x80b0, 0x007f7f7f	/* fog table color */
};

/* We wait for vertical blank (to make it nicer looking) and then
   set these screen parameters. */
uint32 scrn_parameters[] = {
	0x80d4, 0x007e0345,	/* horizontal border */
	0x80e0, 0x07d6c63f,	/* sync control */
	0x80c8, 0x03450000,	/* set to same as border H in 80d4 */
	0x8068, 0x027f0000,	/* (X resolution - 1) << 16 */
	0x806c, 0x01df0000,	/* (Y resolution - 1) << 16 */
	0x804c, 0x000000a0,	/* display align */
	0x8118, 0x00008040,	/* M */
	0x80f4, 0x00000401,	/* anti-aliasing */
	0x8048, 0x00000009,	/* alpha config */
//	0x7814, 0x00000000,	/* More interrupt control stuff (so it seems) */
//	0x7834, 0x00000000,
//	0x7854, 0x00000000,
//	0x7874, 0x00000000,
//	0x78bc, 0x4659404f,
	0x8040, 0x00000000	/* border color */
};

/* Int handler stuff */
static vint32 visible_page;
static vint32 next_visible_page;
static vint32 rendered_page;
static vint32 next_rendered_page;
static vint32 next_page;
static vuint32 list_complete;
static vuint32 render_complete;
static kthread_t *waiting_thd;
static uint32 to_texture, to_txr_addr, to_txr_rp;
static void int_handler(uint32 type);

static semaphore_t * render_sema;

/* A macro for setting PVR registers so we can hook it for debug */
#if 0
void SETREG(uint32 n, uint32 v) {
	dbglog(DBG_KDEBUG, "%08lx <- %08lx\r\n", n, v);
	*((vuint32*)(0xa05f0000+(n))) = (v);
}

uint32 GETREG(n) {
	uint32 value = *((vuint32*)(0xa05f0000+(n)));
	dbglog(DBG_KDEBUG, "%08lx -> %08lx\r\n", n, value);
	return value;
}
#else
#define SETREG(n, v) *((vuint32*)(0xa05f0000+(n))) = (v)
#define GETREG(n) *((vuint32*)(0xa05f0000+(n)))
#endif

/* Allocate TA buffers given a set of parameters

This is a little bit confusing so I'll clarify here:
- The registration process takes place into the buffer which is currently
  being displayed to the user. This is ok since registration doesn't affect
  the output display.
- The rendering process takes places into the buffer which is not being
  displayed. This is also ok since the user can't see this taking place.

So the "frame" that goes with a given set of buffers is not actually the frame
where that data will be rendered, it's the view frame that goes along with
registration into that buffer.

The other confusing thing is that texture ram is a 64-bit multiplexed space
rather than a copy of the flat 32-bit VRAM. So in order to maximize the
available texture RAM, the PVR structures for the two frames are broken
up and placed at 0x000000 and 0x400000.

*/
#define TA_ALIGN 128
#define TA_ALIGN_MASK (TA_ALIGN-1)
static void ta_allocate_buffers(int w, int h, uint32 lists,
		uint32 vertex_list_size, uint32 poly_buf_size) {
	int		i;
	uint32		outaddr, polybuf, sconst;
	ta_buffers_t	*buf;

	/* Set screen sizes */
	ta_state.w = w; ta_state.h = h;
	ta_state.tw = w/32; ta_state.th = h/32;
	ta_state.tsize_const = ((ta_state.th-1)<<16) | ((ta_state.tw-1)<<0);
	ta_state.zclip = 0.0001f;
	ta_state.pclip_left = 0; ta_state.pclip_right = 640-1;
	ta_state.pclip_top = 0; ta_state.pclip_bottom = 480-1;
	ta_state.pclip_x = (ta_state.pclip_right << 16) | (ta_state.pclip_left);
	ta_state.pclip_y = (ta_state.pclip_bottom << 16) | (ta_state.pclip_top);
	
	/* Look at active lists and figure out how much to allocate
	   for each poly type */
	ta_state.lists = lists;
	ta_state.poly_buf_size = poly_buf_size * 4;		/* in bytes */
	ta_state.poly_buf_ind = ta_state.poly_buf_size * ta_state.tw * ta_state.th;
	polybuf = 0;
	ta_state.list_mask = 1<<20;
	switch(poly_buf_size) {
		case TA_POLYBUF_0: sconst = 0; break;
		case TA_POLYBUF_8: sconst = 1; break;
		case TA_POLYBUF_16: sconst = 2; break;
		case TA_POLYBUF_32: sconst = 3; break;
		default: sconst = 2; break;
	}
	if (lists & TA_LIST_OPAQUE_POLYS) {
		polybuf += ta_state.poly_buf_ind;
		ta_state.list_mask |= sconst << 0;
	}
	if (lists & TA_LIST_TRANS_POLYS) {
		polybuf += ta_state.poly_buf_ind;
		ta_state.list_mask |= sconst << 8;
	}
	if (lists & TA_LIST_OPAQUE_MODS) {
		polybuf += ta_state.poly_buf_ind;
		ta_state.list_mask |= sconst << 4;
	}
	if (lists & TA_LIST_TRANS_MODS) {
		polybuf += ta_state.poly_buf_ind;
		ta_state.list_mask |= sconst << 12;
	}
	if (lists & TA_LIST_PUNCH_THRU) {
		polybuf += ta_state.poly_buf_ind;
		ta_state.list_mask |= sconst << 16;
	}
	

	/* Initialize each buffer set */
	for (i=0; i<2; i++) {
		uint32 polybuf_alloc;
		
		if (i == 0)
			outaddr = 0;
		else
			outaddr = 0x400000;

		buf = ta_state.buffers + i;
	
		/* Vertex buffer */
		buf->vertex = outaddr;
		buf->vertex_size = vertex_list_size;
		outaddr += buf->vertex_size;
	
		/* N-byte align */
		outaddr = (outaddr + TA_ALIGN_MASK) & ~TA_ALIGN_MASK;

		/* Polygon buffers */
		buf->poly_buf_size = 0x50580 + polybuf;
		outaddr += buf->poly_buf_size;
		// buf->poly_buf_size = (outaddr - polybuf) - buf->vertex;
		polybuf_alloc = buf->poly_buf = outaddr - polybuf;
		
		if (lists & TA_LIST_OPAQUE_POLYS) {
			buf->poly_bufs[0] = polybuf_alloc;
			polybuf_alloc += ta_state.poly_buf_ind;
		}
		else
			buf->poly_bufs[0] = 0x80000000;

		if (lists & TA_LIST_OPAQUE_MODS) {
			buf->poly_bufs[1] = polybuf_alloc;
			polybuf_alloc += ta_state.poly_buf_ind;
		}
		else
			buf->poly_bufs[1] = 0x80000000;

		if (lists & TA_LIST_TRANS_POLYS) {
			buf->poly_bufs[2] = polybuf_alloc;
			polybuf_alloc += ta_state.poly_buf_ind;
		}
		else
			buf->poly_bufs[2] = 0x80000000;

		if (lists & TA_LIST_TRANS_MODS) {
			buf->poly_bufs[3] = polybuf_alloc;
			polybuf_alloc += ta_state.poly_buf_ind;
		}
		else
			buf->poly_bufs[3] = 0x80000000;

		if (lists & TA_LIST_PUNCH_THRU) {
			buf->poly_bufs[4] = polybuf_alloc;
			polybuf_alloc += ta_state.poly_buf_ind;
		}
		else
			buf->poly_bufs[4] = 0x80000000;
		outaddr += buf->poly_buf_size;
	
		/* N-byte align */
		outaddr = (outaddr + TA_ALIGN_MASK) & ~TA_ALIGN_MASK;

		/* TA Matrix */
		buf->tile_matrix = outaddr;
		buf->tile_matrix_size = (18+6*ta_state.tw*ta_state.th)*4;
		outaddr += buf->tile_matrix_size;
	
		/* N-byte align */
		outaddr = (outaddr + TA_ALIGN_MASK) & ~TA_ALIGN_MASK;
		
		/* Output buffer */
		buf->frame = outaddr;
		buf->frame_size = w*h*2;
		outaddr += buf->frame_size;
		
		/* N-byte align */
		outaddr = (outaddr + TA_ALIGN_MASK) & ~TA_ALIGN_MASK;
	}

	/* Texture ram is whatever is left */
	ta_state.texture_base = (outaddr - 0x400000)*2;
	dbglog(DBG_KDEBUG, "ta: texture RAM begins at %08x\r\n", outaddr);

	dbglog(DBG_KDEBUG, "Init TA buffers:\r\n");
	for (i=0; i<2; i++) {		
		buf = ta_state.buffers+i;
		dbglog(DBG_KDEBUG, "  vertex/vertex_size: %08lx/%08lx\r\n", buf->vertex, buf->vertex_size);
		dbglog(DBG_KDEBUG, "  poly_buf/poly_buf_size: %08lx/%08lx\r\n", buf->poly_buf, buf->poly_buf_size);
		dbglog(DBG_KDEBUG, "  poly_bufs: %08lx %08lx %08lx %08lx %08lx\r\n",
			buf->poly_bufs[0],buf->poly_bufs[1],buf->poly_bufs[2],buf->poly_bufs[3],buf->poly_bufs[4]);
		dbglog(DBG_KDEBUG, "  tile_matrix/tile_matrix_size: %08lx/%08lx\r\n", buf->tile_matrix, buf->tile_matrix_size);
		dbglog(DBG_KDEBUG, "  frame/frame_size: %08lx/%08lx\r\n", buf->frame, buf->frame_size);
	}
	
	dbglog(DBG_KDEBUG, "lists/list_mask %08lx/%08lx\r\n", ta_state.lists, ta_state.list_mask);
	dbglog(DBG_KDEBUG, "poly_buf_ind/poly_buf_size %08lx/%08lx\r\n", ta_state.poly_buf_ind, ta_state.poly_buf_size);
	dbglog(DBG_KDEBUG, "w/h = %d/%d, tw/th = %d/%d\r\n", ta_state.w, ta_state.h,
		ta_state.tw, ta_state.th);
	dbglog(DBG_KDEBUG, "zclip %08lx\r\n", *((uint32*)&ta_state.zclip));
	dbglog(DBG_KDEBUG, "pclip_left/right %08lx/%08lx\r\n", ta_state.pclip_left, ta_state.pclip_right);
	dbglog(DBG_KDEBUG, "pclip_top/bottom %08lx/%08lx\r\n", ta_state.pclip_top, ta_state.pclip_bottom);
}


/* Initialize fog tables; we don't use these right now but
   it's part of the proper setup. */
static void ta_fog_init() {
	uint32	idx;
	uint32	value;
	
	for (idx=0x8200, value=0xfffe; idx<0x8400; idx+=4) {
		SETREG(idx, value);
		value -= 0x101;
	}
}

/* Fill Tile Matrix buffers. This function takes a base address and sets up
   the TA rendering structures there. Each tile of the screen (32x32) receives
   a small buffer space. */
static void ta_create_buffers(int which) {
	int		x, y;
	uint32		*vr = (uint32*)0xa5000000;
	ta_buffers_t	*buf = ta_state.buffers + which;
	uint32		pbs = ta_state.poly_buf_size;
	uint32		strbase, bufbase;

	strbase = buf->tile_matrix;
	bufbase = buf->poly_buf;
	
	/* Header of zeros */
	vr += buf->tile_matrix/4;
	for (x=0; x<0x48; x+=4)
		*vr++ = 0;
		
	/* Initial init tile */
	vr[0] = 0x10000000;
	vr[1] = 0x80000000;
	vr[2] = 0x80000000;
	vr[3] = 0x80000000;
	vr[4] = 0x80000000;
	vr[5] = 0x80000000;
	vr += 6;
	
	/* Now the main tile matrix */
	dbglog(DBG_KDEBUG, "Using poly buffers %08lx/%08lx/%08lx/%08lx/%08lx\r\n",
		buf->poly_bufs[0],buf->poly_bufs[1],buf->poly_bufs[2],buf->poly_bufs[3],buf->poly_bufs[4]);
	for (x=0; x<ta_state.tw; x++) {
		for (y=0; y<ta_state.th; y++) {
			/*printf("%d,%d -> %08lx/%08lx\r\n", x, y,
				buf->poly_bufs[0]+pbs*ta_state.tw*y+pbs*x,
				buf->poly_bufs[2]+pbs*ta_state.tw*y+pbs*x);*/

			/* Control word */
			vr[0] = (y << 8) | (x << 2);

			/* Opaque poly buffer */
			vr[1] = buf->poly_bufs[0] + pbs*ta_state.tw*y + pbs*x;
			
			/* Opaque volume mod buffer */
			vr[2] = buf->poly_bufs[1] + pbs*ta_state.tw*y + pbs*x;
			
			/* Translucent poly buffer */
			vr[3] = buf->poly_bufs[2] + pbs*ta_state.tw*y + pbs*x;
			
			/* Translucent volume mod buffer */
			vr[4] = buf->poly_bufs[3] + pbs*ta_state.tw*y + pbs*x;
			
			/* Punch-thru poly buffer */
			vr[5] = buf->poly_bufs[4] + pbs*ta_state.tw*y + pbs*x;
			vr += 6;
		}
	}
	vr[-6] |= 1<<31;

	/* Must skip over zeroed header for actual usage */
	buf->tile_matrix += 0x48;
}

/* Take a series of register/value pairs and set the values */
static void set_regs(uint32 *values, uint32 cnt) {
	int i;
	uint32 r, v;

	for (i=0; i<cnt; i+=2) {
		r = values[i];
		v = values[i+1];
		SETREG(r, v);
	}
}

/* Select a structure buffer; after this completes, all registration
   and setup operations will reference the given buffer. */
static void ta_set_reg_target(int which) {
	ta_buffers_t	*buf = ta_state.buffers + which;

	SETREG(0x8008, 1);			/* Reset registration mode */
	SETREG(0x8008, 0);
	SETREG(0x8124, buf->poly_buf);		/* Set buffer pointers */
	SETREG(0x812c, buf->poly_buf - buf->poly_buf_size);
	SETREG(0x8128, buf->vertex);
	SETREG(0x8130, buf->vertex + buf->vertex_size);
	SETREG(0x8164, buf->poly_buf);
	SETREG(0x813c, ta_state.tsize_const);	/* Tile count: (H/32-1) << 16 | (W/32-1) */
	SETREG(0x8140, ta_state.list_mask);	/* List enables */
	SETREG(0x8144, 0x80000000);		/* Confirm settings */
	(void)GETREG(0x8144);
}

/* Select a target view buffer; after this completes, the user will be looking
   at the frame buffer of the requested frame. */
static void ta_set_view_target(int which) {
	vid_set_start(ta_state.buffers[which].frame);
}

/* Begin the rendering process from the given registration source,
   into the given destination frame buffer. */
static void ta_render_target(int which) {
	ta_buffers_t *buf = ta_state.buffers + which;
	uint32 *vrl = (uint32*)0xa5000000;

	/* Calculate background value for below */
	/* Small side note: during setup, the value is originally
	   0x01203000... I'm thinking that the upper word signifies
	   the length of the background plane list in dwords
	   shifted up by 4. */
	uint32 taend = 0x01000000 | ((GETREG(0x8138) - buf->vertex) << 1);
	
	/* Throw the background data on the end of the TA's list */
	int i;
	// uint32		*bkgdata = (uint32*)ta_bkg;
	vrl = (uint32*)(0xa5000000 | GETREG(0x8138));
	for (i=0; i<0x12; i++)
		vrl[i] = 0;
	//	vrl[(i+taend)/4] = bkgdata[i/4];

	/* Finish up rendering the current frame (into the other buffer) */
	SETREG(0x802c, buf->tile_matrix);
	SETREG(0x8020, buf->vertex);
	if (!to_texture)
		SETREG(0x8060, buf->frame);
	else {
		SETREG(0x8060, to_txr_addr | (1<<24));
		SETREG(0x8064, to_txr_addr | (1<<24));
	}
	SETREG(0x808c, taend);			/* Bkg plane location */
	SETREG(0x8088, *((uint32*)&ta_state.zclip));
	SETREG(0x8068, ta_state.pclip_x);
	SETREG(0x806c, ta_state.pclip_y);
	if (!to_texture)
		SETREG(0x804c, (ta_state.w*2)/8);
	else
		SETREG(0x804c, to_txr_rp);
	SETREG(0x8048, 0x00000009);		/* Alpha mode */
}

void ta_start_render()
{

 	SETREG(0x8014, 0xffffffff);		/* Start render */
}

void ta_stop_render()
{

 	SETREG(0x8014, 0);		/* Start render */
}


/* Set the active list mask and buffer sizes; note that this can ONLY be
   done before hdwr_init has been called, or AFTER hdwr_shutdown has
   been called. */
void ta_set_buffer_config(uint32 listmask, uint32 buffersize, uint32 vertsize) {
	ta_state.lists_to_alloc = listmask;
	ta_state.buffer_size = buffersize;
	ta_state.vertex_buf_size = vertsize;
}

/* Prepare the TA for page flipped 3D */
static void ta_hdwr_init() {
	/* Fully reset TA */
	SETREG(0x8008, 0xffffffff);
	SETREG(0x8008, 0);

	/* Allocate VRAM space for all PVR structures */
	ta_allocate_buffers(vid_mode->width, vid_mode->height,
		ta_state.lists_to_alloc,
		ta_state.vertex_buf_size,
		ta_state.buffer_size);
	//ta_allocate_buffers(640, 480, TA_LIST_OPAQUE_POLYS,
	//	512*1024, TA_POLYBUF_32);

	/* Blank screen and reset display enable */
	SETREG(0x80e8, GETREG(0x80e8) | 8);	/* Blank */
	SETREG(0x8044, GETREG(0x8044) & ~1);	/* Display disable */

	/* Start with a non-primed interrupt */
	rendered_page = next_rendered_page = -1;
	visible_page = next_visible_page = -1;
	next_page = 0;
	list_complete = 0;
	render_complete = 1;
	to_texture = 0;
	waiting_thd = NULL;

	/* Hook the PVR interrupt */
	/* irq_set_handler(EXC_IRQ9, int_handler); */
	asic_evt_set_handler(G2EVT_TA_SCANINT1, int_handler);	asic_evt_enable(G2EVT_TA_SCANINT1, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_OPAQUEDONE, int_handler);	asic_evt_enable(G2EVT_TA_OPAQUEDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_OPAQUEMODDONE, int_handler);	asic_evt_enable(G2EVT_TA_OPAQUEMODDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_TRANSDONE, int_handler);	asic_evt_enable(G2EVT_TA_TRANSDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_TRANSMODDONE, int_handler);	asic_evt_enable(G2EVT_TA_TRANSMODDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_PTDONE, int_handler);	asic_evt_enable(G2EVT_TA_PTDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_RENDERDONE, int_handler);	asic_evt_enable(G2EVT_TA_RENDERDONE, ASIC_IRQ_DEFAULT);

	/* Clear out video memory */
	vid_empty();

	/* Setup basic 3D parameters */
	set_regs(three_d_parameters, sizeof(three_d_parameters)/4);
	ta_fog_init();

	/* Set screen mode parameters */
	vid_waitvbl();
	set_regs(scrn_parameters, sizeof(scrn_parameters)/4);	

	/* If it's VGA, turn off pixel scaling */
	if (vid_mode->cable_type == CT_VGA) {
		dbglog(DBG_KDEBUG, "ta: setting Y output scale to 1.0\r\n");
		SETREG(0x80f4, 0x400);
	}

	/* Point at the second set of buffer structures, 
	   and build said structures. */
	ta_set_reg_target(1);
	ta_create_buffers(1);

	/* Now setup the first frame */
	ta_set_reg_target(0);
	ta_create_buffers(0);

	/* Point back at the second output buffer */
	ta_set_view_target(1);
	ta_set_reg_target(1);

	/* Set starting render output addresses */
	SETREG(0x8060, ta_state.buffers[1].frame);	/* render output address */
	SETREG(0x8064, ta_state.buffers[0].frame);	/* ? */
	
	/* Unblank screen and set display enable */
	SETREG(0x80e8, GETREG(0x80e8) & ~8);	/* Unblank */
	SETREG(0x8044, GETREG(0x8044) | 1);	/* Display enable */

/* This block shouldn't even be here! Leaving in here until I'm sure it's
   not gonna break anything to remove it. */
#if 0
	/* Enable interrupts based on active lists (normally 0x288) */
	SETREG(0x6930,
		0x04 |			/* Render done int */
		0x08 |			/* Scanline int */
		((ta_state.lists & TA_LIST_OPAQUE_POLYS) ? 0x80 : 0x00) |
		((ta_state.lists & TA_LIST_TRANS_POLYS) ? 0x200 : 0x00) |
		((ta_state.lists & TA_LIST_OPAQUE_MODS) ? 0x100 : 0x00) |
		((ta_state.lists & TA_LIST_TRANS_MODS) ? 0x400 : 0x00) |
		((ta_state.lists & TA_LIST_PUNCH_THRU) ? 0x200000 : 0x00));
#endif

	/* Set current page */
	ta_curpage = 0;
}

/* Turn off TA -- including turning off pesky interrupts */
static void ta_hdwr_shutdown() {
	/* Disable all PVR interrupts */
	asic_evt_set_handler(G2EVT_TA_SCANINT1, NULL);		asic_evt_disable(G2EVT_TA_SCANINT1, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_OPAQUEDONE, NULL);		asic_evt_disable(G2EVT_TA_OPAQUEDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_OPAQUEMODDONE, NULL);	asic_evt_disable(G2EVT_TA_OPAQUEMODDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_TRANSDONE, NULL);		asic_evt_disable(G2EVT_TA_TRANSDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_TRANSMODDONE, NULL);		asic_evt_disable(G2EVT_TA_TRANSMODDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_PTDONE, NULL);		asic_evt_disable(G2EVT_TA_PTDONE, ASIC_IRQ_DEFAULT);
	asic_evt_set_handler(G2EVT_TA_RENDERDONE, NULL);		asic_evt_disable(G2EVT_TA_RENDERDONE, ASIC_IRQ_DEFAULT);
	
	/* SETREG(0x6930, 0);
	SETREG(0x6938, 0);
	irq_set_handler(EXC_IRQ9, NULL); */
	
	/* Clear out the main display buffer and switch back */
	vid_clear(0,0,0);
	vid_set_start(0x00000000);
}

/* Copy data 4 bytes at a time */
static void copy4(uint32 *dest, uint32 *src, int bytes) {
	bytes = bytes / 4;
	while (bytes-- > 0) {
		*dest++ = *src++;
	}
}

/* Send a store queue full of data to the TA */
void ta_send_queue(void *sql, int size) {
	vuint32 *regs = (uint32*)0xff000038;

	sq_lock();

	/* Set store queue destination == tile accelerator */
	regs[0] = regs[1] = 0x10;

	/* Post the first queue */
	copy4((uint32*)0xe0000000, (uint32*)sql, size);
	asm("mov	#0xe0,r0");
	asm("shll16	r0");
	asm("shll8	r0");
	asm("pref	@r0");

	/* If there was a second queue... */
	if (size == 64) {
		asm("mov	#0xe0,r0");
		asm("shll16	r0");
		asm("shll8	r0");
		asm("or		#0x20,r0");
		asm("pref	@r0");
	}
	sq_unlock();
}

/* Begin the rendering process for one frame */
void ta_begin_render() {
  int	ticks;
  
  if (!to_texture) {
    /* Wait for render-done signal from interrupt, which mean the current
       TA list has been rendered, when this is done we
       can start to fill the next TA list. */
    waiting_thd = thd_current;
    //next_rendered_page = next_page; /* ask to render on next_page */
    {
      static first;
      if (!first) {
	first = 1;
	next_rendered_page = next_page; /* ask to render on next_page */
      }
    }

    //    printf("ta: %d %d %d %d %d %x %x\n", rendered_page, next_rendered_page, visible_page, next_visible_page, next_page, list_complete, render_complete);

    ticks = jiffies + 60 * HZ / 100;
    /* wait until render on next_page has started */
#if 0
    while (next_rendered_page >= 0
	   && (jiffies < ticks)) {
      /* thd_pass() is ok here now because if a flip_complete1
	 happens while we're sleeping, the interrupt will 
	 reschedule us to run next */
      if (thd_enabled)
	thd_pass();
    }
#else
    sem_wait_timed(render_sema, 500);
#endif
    
    next_page ^= 1; /* flip next_page for next time */
    if (jiffies >= ticks) {
      dbglog(DBG_WARNING, "ta: timeout waiting for list-complete\n");
    } else
      ;//dbglog(DBG_WARNING, "ta: WAOU !!\n");
    //    printf("ta: %d %d %d %d %d %x %x\n", rendered_page, next_rendered_page, visible_page, next_visible_page, next_page, list_complete, render_complete);

      
  }

#if 0
  /* Clear all pending events */
  vuint32 *pvrevt = (vuint32*)0xa05f6900;
  *pvrevt = 0xffffffff;
  
  /* Start out with incomplete list */
  //list_complete = 0;
#endif
}

/* Setup the various TA variables for rendering the current frame into a
   texture buffer. Note that the tile array can't be rearranged on the fly,
   so the output frame will always be the same as the real display frame.
   However, the texture size will be scaled up to the next power-of-2,
   e.g., 640x480 will become 1024x512. */
void ta_begin_texture(uint32 txr, uint32 *rx, uint32 *ry) {
	int i;
	
	/* Find the real X and Y sizes */
	*rx = *ry = 0;
	for (i=0; i<32; i++) {
		if ( (vid_mode->width >= (1 << i))
				&& (vid_mode->width < (1 << (i+1))) ) {
			*rx = 1 << (i+1);
		}
		if ( (vid_mode->height >= (1 << i))
				&& (vid_mode->width < (1 << (i+1))) ) {
			*ry = 1 << (i+1);
		}

		if ((*rx != 0) && (*ry != 0))
			break;
	}
	
	/* Start out with incomplete lists */
	list_complete = 0;

	/* Mark the operation as a to-texture render */
	to_texture = 1;

	/* Set new render pitch */
	to_txr_rp = (*rx) * 2 / 8;

	/* Set new output address */
	to_txr_addr = ta_state.texture_base + txr;
}

/* Commit a polygon header to the TA */
void ta_commit_poly_hdr(void *polyhdr) {
	ta_send_queue(polyhdr, 32);
}

/* Commit a vertex to the TA */
void ta_commit_vertex(void *vertex, int size) {
	ta_send_queue(vertex, size);
}

/* Commit an end-of-list to the TA */
void ta_commit_eol() {
	uint32	words[8] = { 0 };
	ta_send_queue(words, 32);
}

/* Finish rendering a frame; this assumes you have written
   a completed display list to the TA. It sets everything up and
   waits for the next vertical blank period to switch buffers.

   VP : It does not wait anymore for vertical blank, 
   now it almost does nothing.
 */
void ta_finish_frame() {
  int	ticks;

  if (!to_texture) {
    if (visible_page >= 0)
      ta_curpage = visible_page;

    next_rendered_page = next_page; /* ask to render on next_page */
  } else {
    /* Wait for any current rendering process to complete */
    ticks = jiffies + 10 * HZ / 100;
    while (!render_complete && (jiffies < ticks))
      ;
    if (jiffies >= ticks)
      dbglog(DBG_WARNING, "ta: timeout waiting for list-complete during to_texture(1)\n");

    /* Reset the renderer */
    SETREG(0x8008, 2);
    SETREG(0x8008, 0);
		
    /* Finish up rendering the current frame (into texture buffer) */
    render_complete = 0;
    ta_render_target(ta_curpage^1);

    /* Wait for the render to complete */
    ticks = jiffies + 100 * HZ / 100;
    while (!render_complete && (jiffies < ticks))
      ;
    if (jiffies >= ticks)
      dbglog(DBG_WARNING, "ta: timeout waiting for list-complete during to_texture(2)\n");

    /* Finish to_texture operation */
    to_texture = 0;

    /* Reset everything back to the way it was (reuse same buffers) */
    ta_set_reg_target(ta_curpage^1);
  }
}

/* Build a polygon header from the given parameters; this is pretty
   incomplete right now but it's better than having to do it by hand. */
void ta_poly_hdr_col(poly_hdr_t *target, int translucent) {
	if (!translucent) {
		target->flags1 = 0x80840012;
		target->flags2 = 0x90800000;
		target->flags3 = 0x20800440;
		target->flags4 = 0x00000000;
		target->dummy1 = target->dummy2
			= target->dummy3 = target->dummy4 = 0xffffffff;
	} else  {
		target->flags1 = 0x82840012;
		target->flags2 = 0x90800000;
		target->flags3 = 0x949004c0;
		target->flags4 = 0x00000000;
		target->dummy1 = target->dummy2
			= target->dummy3 = target->dummy4 = 0xffffffff;
	}
}

void ta_poly_hdr_txr(poly_hdr_t *target, int translucent,
		int textureformat, int tw, int th, uint32 textureaddr,
		int filtering) {
	int i, ts = 8, n = 3;

	/* Take into account texture base */
	textureaddr += ta_state.texture_base;
	
	if (textureformat == TA_NO_TEXTURE) {
		ta_poly_hdr_col(target, translucent);
		return;
	}
	
	for (i=0; i<8 && n; i++) {
		if ((n&1) && tw == ts) {
			tw = i;
			n &= ~1;
		}
		if ((n&2) && th == ts) {
			th = i;
			n &= ~2;
		}
		ts <<= 1;
	}
	textureformat <<= 26;		
	
	if (!translucent) {
		target->flags1 = 0x8084001a;
		target->flags2 = 0x90800000;
		target->flags3 = 0x20800440 | (tw << 3) | th;
		if (filtering)
			target->flags3 |= 0x2000;
		target->flags4 = textureformat | (textureaddr >> 3);
	} else {
		target->flags1 = 0x8284001a;
		target->flags2 = 0x92800000;
		target->flags3 = 0x949004c0 | (tw << 3) | th;
		if (filtering)
			target->flags3 |= 0x2000;
		target->flags4 = textureformat | (textureaddr >> 3);
	}
}

#if 0
#include "controler.h"
extern controler_state_t controler68;
void dbp(const char * p)
{
  if (controler68.buttons & CONT_B)
    printf(p);
}
#else
#define dbp(...) (void)0
#endif

/* PVR interrupt handler; the way things are setup, we're gonna get
   one of these for each full vertical refresh and at the completion
   of TA data acceptance. The timing here is pretty critical. We need
   to flip pages during a vertical blank, and then signal to the program
   that it's ok to start playing with TA registers again. */
static void int_handler(uint32 code) {
  /* What kind of event did we get? Is it a list completion? */
  switch(code) {
  case G2EVT_TA_OPAQUEDONE:
    list_complete |= TA_LIST_OPAQUE_POLYS;
    dbp("1");
    return;
  case G2EVT_TA_TRANSDONE:
    list_complete |= TA_LIST_TRANS_POLYS;
    dbp("2");
    return;
  case G2EVT_TA_OPAQUEMODDONE:
    list_complete |= TA_LIST_OPAQUE_MODS;
    dbp("3");
    return;
  case G2EVT_TA_TRANSMODDONE:
    list_complete |= TA_LIST_TRANS_MODS;
    dbp("4");
    return;
  case G2EVT_TA_PTDONE:
    list_complete |= TA_LIST_PUNCH_THRU;
    dbp("5");
    break;
  case G2EVT_TA_RENDERDONE:	/* never gets called?! */
    render_complete = 1;
    dbp("6");
    //ta_stop_render();
    break;
  case G2EVT_TA_SCANINT1:
    ta_state.frame_counter++;
    if (ta_scan_cb)
      ta_scan_cb();

    dbp("7");
    break;
  }

  //static int next;

  /* All lists are complete, and render is done */
  /* Note: render-done doesn't seem to be reliable */
  /* VP : we now use it, this is NECESSARY and seems actually
     reliable ... */
  if (!to_texture) {
    int sc = 0;


    if (list_complete == ta_state.lists && rendered_page >= 0) {
      render_counter++;
      dbp("L");

      if (ta_render_done_cb)
	ta_render_done_cb();
    }

    if (render_complete && rendered_page >= 0) {
      render_counter2++;
      dbp("R");

    }

    /* Detect end of rendering. */
    if (
	list_complete == ta_state.lists 
	&& render_complete
	) {

      if (rendered_page >= 0) {
	/* Clear list completion */
	list_complete = 0;
	/* Clear render completion */
	render_complete = 0;

	/* Now that render is finished, we can send this page on the screen. */
	if (next_visible_page < 0 && visible_page != rendered_page) {
	  next_visible_page = rendered_page;
	  //next = ta_state.frame_counter+2;
	}

	/* Mark that no rendering is hapenning currently. */
	rendered_page = -1;
      }
    }

    /* Handle visible page */
    if(code == G2EVT_TA_SCANINT1) {
      /* Any request for a new visible page ? */
      if (
	  //ta_state.frame_counter >= next && 
	  next_visible_page >= 0) {
	visible_page = next_visible_page;
	
	/* Set visible page */
	ta_set_view_target(visible_page^1);

	/* Mark as done */
	next_visible_page = -1;
      }
    }

    /* A new render is requested ? */
    if (!ta_block_render && next_rendered_page >= 0) {
      /* Some condition are requiered, check them */
      if (
	  /* No rendering in progress already ? */
	  rendered_page == -1 
	  /* Make sure the page we want to render is not visible */
	  && (visible_page < 0 || 
	      visible_page != next_rendered_page)
	  /* Make sure the page we want to render is not the next visible */
	  && (next_visible_page < 0 || 
	      next_visible_page != next_rendered_page)
	  ) {

	/* OK, ready to render on requested page, mark rendering in progress */
	rendered_page = next_rendered_page;
	next_rendered_page = -1;

	/* Switch hardware registration buffers */
	ta_render_target(rendered_page^1);
	ta_start_render();
	ta_set_reg_target(rendered_page);
      
	/* Reschedule, probably ta_begin_render is waiting to hear
	   about the good new :) */
	sc = 1;

      }
    }

    /* Schedule any waiting thread next if necessary */
    if (sc && thd_enabled && waiting_thd != NULL) {
      sem_signal(render_sema);
      thd_schedule_next(waiting_thd);
      waiting_thd = NULL;
    }

  }
}

/* Do TA hardware init/shutdown */
void ta_hw_init() {
	/* Setup a default hardware background plane */
	ta_bkg->flags1 = 0x90800000;
	ta_bkg->flags2 = 0x20800440;
	ta_bkg->dummy = 0;
	ta_bkg->x1 = 0.0f;
	ta_bkg->y1 = 480.0f;
	ta_bkg->z1 = 0.2f;
	ta_bkg->argb1 = 0xff000000;
	ta_bkg->x2 = 0.0f;
	ta_bkg->y2 = 0.0f;
	ta_bkg->z3 = 0.2f;
	ta_bkg->argb2 = 0xff000000;
	ta_bkg->x3 = 640.0f;
	ta_bkg->y3 = 480.0f;
	ta_bkg->z3 = 0.2f;
	ta_bkg->argb3 = 0xff000000;

	/* Initialize 3D hardware */
	ta_hdwr_init();

	/* Zero texture RAM */
	ta_txr_release_all();
}

void ta_hw_shutdown() {
	/* De-init 3D hardware */
	ta_hdwr_shutdown();
}

/* Program init / shutdown */
void ta_init(int lists, int binsize, int vertbuf) {
  render_sema = sem_create(0);

	/* Set a default buffer configuration */
	ta_set_buffer_config(lists, binsize, vertbuf);

	/* Initialize the hardware */
	ta_hw_init();
}

void ta_init_defaults() {
	ta_init(TA_LIST_OPAQUE_POLYS | TA_LIST_TRANS_POLYS,
		TA_POLYBUF_32, 512*1024);
}

void ta_shutdown() {
	ta_hw_shutdown();
	sem_destroy(render_sema);
}
