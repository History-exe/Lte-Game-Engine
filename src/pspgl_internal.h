/*
 * LTE Game Engine
 * Copyright (C) 2006-2008 SiberianSTAR <haxormail@gmail.com>
 * http://www.ltestudios.com
 *  
 * The LTE Game Engine is based on Irrlicht 1.0
 * Irrlicht Engine is Copyright (C) 2002-2006 Nikolaus Gebhardt
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */
 
#ifndef __pspgl_internal_h__
#define __pspgl_internal_h__

#define MAX_ATTRIB_STACK        16

struct pspgl_vertex_array {
	GLenum enabled;
	GLint size;
	GLenum type;
	GLsizei stride;
	GLboolean native;       /* size and type match hardware */
	const GLvoid *ptr;

	struct pspgl_bufferobj *buffer;
};


#define HASH_SIZE       (32)
#define HASH_NO_KEY     (~0)


struct hashtable {
	struct hashentry *buckets [HASH_SIZE];
	unsigned long maxkey;
};

struct pspgl_shared_context {
	int refcount;
	struct hashtable texture_objects;
	struct hashtable display_lists;
	struct hashtable buffers;
};

#define MF_DIRTY        (1<<0)  /* hardware needs updating (stack) */
#define MF_DISABLED     (1<<1)  /* always load with identity (stack) */
#define MF_IDENTITY     (1<<2)  /* is identity matrix (matrix) */
#define MF_VFPU         (1<<3)  /* matrix stack top is in VFPU (stack) */
#define MF_ADJUST       (1<<4)  /* some pre-use adjustments to apply */
#define VFPU_ALIGNMENT  (sizeof(float) * 4)     /* alignment required for VFPU matrix loads and stores */

#define EGLint int
#define EGLBoolean int

#define VFPU_STACKTOP   VMAT7   /* use matrix 7 for top-of-stack */

struct pspgl_matrix {
	/* Align matrix so VFPU can use aligned memory operations.
	   XXX separate flags so that we don't waste a pile of space
	   in padding. */
	GLfloat mat[16] __attribute__((aligned(VFPU_ALIGNMENT)));
	unsigned flags;
};

struct pspgl_matrix_stack {
	/* adjustments to be applied before use */
	float scale[4] __attribute__((aligned(VFPU_ALIGNMENT)));
	float trans[4] __attribute__((aligned(VFPU_ALIGNMENT)));

	struct pspgl_matrix *stack;

	unsigned limit;
	unsigned depth;
	unsigned flags;
};

#define VARRAY_MAX      5       /* number of arrays */
#define MAX_ATTRIB      VARRAY_MAX

#define VA_VERTEX_BIT   (1<<0)
#define VA_NORMAL_BIT   (1<<1)
#define VA_COLOR_BIT    (1<<2)
#define VA_TEXCOORD_BIT (1<<3)
#define VA_WEIGHT_BIT   (1<<4)

#define NBONES          8

struct vertex_format
{
	unsigned hwformat;
	unsigned vertex_size;

	unsigned arrays;                /* bitmask of arrays used by this format */

	int nattrib;
	struct attrib {
		unsigned offset;        /* offset into output vertex */
		unsigned size;          /* size of element in output vertex */

		struct pspgl_vertex_array *array; /* source array */

		void (*convert)(void *to, const void *from, const struct attrib *attr);
	} attribs[MAX_ATTRIB];
};

struct t2f_c4ub_n3f_v3f {
	GLfloat texcoord [2];
	unsigned long color;
	GLfloat normal [3];
	GLfloat vertex [3];
};

struct pspgl_context {
	struct pspvfpu_context *vfpu_context;

	GLboolean posmap;

	struct __texgen {
		GLboolean gen_s;
		GLboolean gen_t;
		unsigned int gen_mode_s;
		unsigned int gen_mode_t;
	} texgen;
	struct hwstate {
		unsigned dirty;
#define HWD_CLUT        (1 << 0)

		unsigned int ge_reg [256];
		unsigned int ge_reg_touched [256/32];
	} hw;

	struct {
		GLenum primitive;
		unsigned long vertex_count;
		void *vbuf_adr;
		struct t2f_c4ub_n3f_v3f line_loop_start;
	} beginend;
	struct current {
		GLfloat texcoord [4];
		unsigned long color;
		GLfloat normal [3];
	} current;

	struct varray {
		struct pspgl_vertex_array vertex;
		struct pspgl_vertex_array normal;
		struct pspgl_vertex_array color;
		struct pspgl_vertex_array texcoord;
		struct pspgl_vertex_array weight;

		struct locked_arrays {
			GLint first;
			GLsizei count;

			struct vertex_format vfmt;

			struct pspgl_buffer *cached_array;
			unsigned cached_array_offset;
			GLint cached_first;
		} locked;

		struct pspgl_bufferobj *arraybuffer;
		struct pspgl_bufferobj *indexbuffer;
	} vertex_array;

	struct clear {
		unsigned int color;
		GLint stencil;
		unsigned short depth;
	} clear;

	struct scissor_test {
		GLenum enabled;
		GLint x, y, width, height;
	} scissor_test;

	struct write_mask {
		unsigned char alpha;
		unsigned char stencil;
	} write_mask;

	struct lights {
		GLboolean positional[4];        /* set by glLight(N, GL_POSITION, ) */
		GLboolean spotlight[4]; /* set by glLight(N, GL_SPOT_CUTOFF, ) */
	} lights;

	struct material {
		unsigned long ambient;
	} material;

	/* cull_face = (front_cw ^ cull_front) ? ccw : cw */
	struct polygon {
		unsigned char front_cw;
		unsigned char cull_front;
	} polygon;

	struct pspgl_matrix_stack projection_stack;
	struct pspgl_matrix_stack modelview_stack;
	struct pspgl_matrix_stack texture_stack;
	struct pspgl_matrix_stack view_stack;
	struct pspgl_matrix_stack bone_stacks[NBONES];

	struct pspgl_matrix_stack *current_matrix_stack;
	struct pspgl_matrix *current_matrix;


	struct pspgl_shared_context *shared;

	struct pspgl_surface *read;
	struct pspgl_surface *draw;

	struct pspgl_saved_attrib *attribstack[MAX_ATTRIB_STACK];
	unsigned attribstackdepth;

	struct pspgl_saved_clattrib *clattribstack[MAX_ATTRIB_STACK];
	unsigned clattribstackdepth;

	GLenum glerror;
	unsigned int swap_interval;
	int initialized;
	int refcount;

	struct viewport {
		GLint x, y, width, height;
		GLfloat depth_offset;
	} viewport;
	struct fog {
		GLfloat near, far;
	} fog;
	struct texture {
		struct pspgl_texobj     *bound; /* currently bound texture */
	} texture;
	struct pixelstore {
		unsigned row_length;
		unsigned alignment;
		unsigned skip_pixels;
		unsigned skip_rows;
		GLboolean invert;

		struct pspgl_bufferobj *pbo;
	} pack, unpack;
	struct vertexblend {
		GLboolean enabled;
	} vertexblend;
	struct {
		GLboolean enabled;

		/* all times in microseconds */
		unsigned queuewait;      /* time spent waiting for queues */
		unsigned buffer_issues;  /* number of command buffers issued */
	} stats;
};


struct pspgl_surface {
	unsigned refcount;

	unsigned pixfmt;
	unsigned short width;
	unsigned short height;
	unsigned short pixelperline;
	unsigned char flags;
#define SURF_DISPLAYED  (1<<0)  /* displayed surface */
#define SURF_TEXTURE    (1<<1)  /* surface suitable for texture */
#define SURF_USEALPHA   (1<<2)  /* use surface alpha in texture */

	struct pspgl_buffer *color_front, *color_back;
	struct pspgl_buffer *depth_buffer;

	struct pspgl_buffer **read;
	struct pspgl_buffer **draw;

	unsigned alpha_mask, stencil_mask;

	/* timing stats */
	unsigned long long flip_start, flip_end, prev_end;
};
#ifdef __cplusplus
extern "C" {
#endif
/* glEnable */
void __pspgl_enable_state(GLenum, GLboolean);

/* pspgl_ge_init.c */
extern unsigned int __pspgl_context_register[];
extern void __pspgl_ge_init (struct pspgl_context *c);


/* pspgl_vidmem.c */
extern EGLint __pspgl_eglerror;
extern struct pspgl_context *__pspgl_curctx;
#define pspgl_curctx    __pspgl_curctx

extern int   __pspgl_vidmem_alloc (struct pspgl_buffer *buf);
extern void  __pspgl_vidmem_free (struct pspgl_buffer *buf);
extern EGLBoolean __pspgl_vidmem_setup_write_and_display_buffer (struct pspgl_surface *s);
extern GLboolean __pspgl_vidmem_compact(void);
extern size_t __pspgl_vidmem_avail(void);
extern size_t __pspgl_vidmem_evict(struct pspgl_buffer *buf);

/* glLockArraysEXT.c */
extern GLboolean __pspgl_cache_arrays(void);
extern void __pspgl_uncache_arrays(void);

/* glReadPixels.c */
extern void __pspgl_copy_pixels(const void *srcbuf, int srcstride, int srcx, int srcy,
                                void *dstbuf, int dststride, int dstx, int dsty,
                                int width, int height, unsigned hwpixfmt);
void __pspgl_copy_memory(const void *src, void *dst, size_t size);

#ifdef __cplusplus
}
#endif
/* pspgl_stats.c */

#include <time.h>
#include <psptypes.h>
#include <psprtc.h>




#endif

