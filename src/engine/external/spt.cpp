#include "spt.h"

#include <math.h>

#define PI 3.14159265358979323846

#define BEVEL_ANGLE_SINGLE PI / 8
#define BEVEL_ANGLE_FULL PI / 7

#define SWAP(type, a, b) \
	do { \
		type temp = a; \
		a = b; \
		b = temp; \
	} while(0)

static inline SPT_vec2 vsub(SPT_vec2 a, SPT_vec2 b) {
	SPT_vec2 out;
	out.x = a.x - b.x;
	out.y = a.y - b.y;
	return out;
}
static inline SPT_vec2 vadd(SPT_vec2 a, SPT_vec2 b) {
	SPT_vec2 out;
	out.x = a.x + b.x;
	out.y = a.y + b.y;
	return out;
}
static inline SPT_vec2 vdivf(SPT_vec2 v, float f) {
	SPT_vec2 out;
	out.x = v.x / f;
	out.y = v.y / f;
	return out;
}
static inline SPT_vec2 vmulf(SPT_vec2 v, float f) {
	SPT_vec2 out;
	out.x = v.x * f;
	out.y = v.y * f;
	return out;
}
static inline SPT_vec2 vdir(float angle) {
	SPT_vec2 out;
	out.x = cosf(angle);
	out.y = sinf(angle);
	return out;
}
static inline float vangle(SPT_vec2 v) {
	return atan2f(v.y, v.x);
}
static inline float vcross(SPT_vec2 a, SPT_vec2 b) {
	return a.x * b.y - a.y * b.x;
}

static inline SPT_color cavg(SPT_color a, SPT_color b) {
	SPT_color color;
	color.r = (a.r + b.r) / 2.0f;
	color.g = (a.g + b.g) / 2.0f;
	color.b = (a.b + b.b) / 2.0f;
	color.a = (a.a + b.a) / 2.0f;
	return color;
}

void SPT_prims(SPT_cb_get_pt getPt, SPT_cb_add_quad addQuad, SPT_cb_add_arc addArc, void* user) {
	SPT_pt pts[3];
	if (!getPt(&pts[0], user))
		return;
	if (!getPt(&pts[1], user)) {
		// Only 1 point
		addArc(pts[0].pos, 0.0f, 2.0f * PI, pts[0].w / 2.0f, pts[0].color, user);
		return;
	}
	if (!getPt(&pts[2], user)) {
		// Only 2 points
		const SPT_vec2 delta = vsub(pts[1].pos, pts[0].pos);
		const float angle = atan2f(delta.y, delta.x);
		addArc(pts[0].pos, angle + PI / 2.0f, angle + PI + PI / 2.0f, pts[0].w / 2.0f, pts[0].color, user);
		addArc(pts[1].pos, angle - PI / 2.0f, angle + PI / 2.0f, pts[1].w / 2.0f, pts[1].color, user);
		const SPT_vec2 perp1 = vmulf(vdir(angle + PI / 2.0f), pts[0].w / 2.0f);
		const SPT_vec2 perp2 = vmulf(vdir(angle + PI / 2.0f), pts[1].w / 2.0f);
		const SPT_quad quad = {
			vadd(pts[1].pos, perp2),
			vadd(pts[0].pos, perp1),
			vsub(pts[0].pos, perp1),
			vsub(pts[1].pos, perp2),
		};
		addQuad(quad, cavg(pts[0].color, pts[1].color), user);
		return;
	}
	// 3 points
	SPT_vec2 top0, bot0; // Required for next iteration
	for (int i = 0; true; ++i) {
		const SPT_vec2 v01 = vsub(pts[1].pos, pts[0].pos);
		const SPT_vec2 v12 = vsub(pts[2].pos, pts[1].pos);
		const SPT_vec2 v02 = vsub(pts[2].pos, pts[0].pos);
		const float a01 = vangle(v01);
		const float a12 = vangle(v12);
		if (i == 0) {
			// Start cap
			addArc(pts[0].pos, a01 + PI / 2.0f, a01 + PI + PI / 2.0f, pts[0].w / 2.0f, pts[0].color, user);
			const SPT_vec2 perp = vmulf(vdir(a01 + PI / 2.0f), pts[0].w / 2.0f);
			bot0 = vadd(pts[0].pos, perp);
			top0 = vsub(pts[0].pos, perp);
		}
		const float cross = vcross(v02, v01); // Whether 1 is left of right of 02
		float bevelAngle1 = a01 + (cross > 0.0f ? PI / -2.0f : PI / 2.0f) + PI;
		float bevelAngle2 = a12 + (cross > 0.0f ? PI / 2.0f : PI / -2.0f);
		if (cross > 0.0f) {
			SWAP(float, bevelAngle1, bevelAngle2); // Make sure bevelAngle2 > bevelAngle1 and shortest arc
			SWAP(SPT_vec2, bot0, top0); // Put last top and bot the right way around
		}
		while (bevelAngle2 < bevelAngle1)
			bevelAngle2 += PI * 2.0f;
		const float tAngle = vangle(vdivf(vadd(vdir(bevelAngle1), vdir(bevelAngle2)), 2.0f)) + PI;
		const SPT_vec2 t = vadd(pts[1].pos, vmulf(vdir(tAngle), pts[1].w / 2.0f));
		const SPT_vec2 l = vadd(pts[1].pos, vmulf(vdir(bevelAngle1), pts[1].w / 2.0f));
		const SPT_vec2 r = vadd(pts[1].pos, vmulf(vdir(bevelAngle2), pts[1].w / 2.0f));
		const float bevelAngle = bevelAngle2 - bevelAngle1;
		if (bevelAngle < BEVEL_ANGLE_SINGLE) { // No bevel
			const SPT_quad quad = {top0, bot0, t, cross > 0.0f ? l : r};
			addQuad(quad, cavg(pts[0].color, pts[1].color), user);
		} else if (bevelAngle < BEVEL_ANGLE_FULL) { // Small bevel
			const SPT_quad quad = {l, l, t, r};
			addQuad(quad, pts[1].color, user);
			// Draw 01
			const SPT_quad quad01 = {top0, bot0, t, cross > 0.0f ? r : l};
			addQuad(quad01, cavg(pts[0].color, pts[1].color), user);
		} else { // Full curved bevel
			addArc(pts[1].pos, bevelAngle1, bevelAngle2, pts[1].w / 2.0f, pts[1].color, user);
			const SPT_quad quad = {pts[1].pos, l, t, r};
			addQuad(quad, pts[1].color, user);
			// Draw 01
			const SPT_quad quad01 = {top0, bot0, t, cross > 0.0f ? r : l};
			addQuad(quad01, cavg(pts[0].color, pts[1].color), user);
		}
		// Setup for next
		top0 = t;
		bot0 = cross > 0.0f ? l : r;
		if(cross <= 0.0f)
			SWAP(SPT_vec2, bot0, top0);
		// Read next point
		SPT_pt next;
		if (!getPt(&next, user)) {
			// Draw 12
			const SPT_vec2 perp = vmulf(vdir(a12 + PI / 2.0f), pts[2].w / 2.0f);
			const SPT_vec2 bot2 = vadd(pts[2].pos, perp);
			const SPT_vec2 top2 = vsub(pts[2].pos, perp);
			const SPT_quad quad = {top0, bot0, bot2, top2};
			addQuad(quad, cavg(pts[1].color, pts[2].color), user);
			// Draw end cap
			addArc(pts[2].pos, a12 - PI / 2.0f, a12 + PI / 2.0f, pts[2].w / 2.0f, pts[2].color, user);
			break;
		};
		pts[0] = pts[1];
		pts[1] = pts[2];
		pts[2] = next;
	}
}

// SPT_tris uses SPT_prims under the hood

typedef struct {
	SPT_cb_get_pt getPt;
	SPT_cb_add_tri addTri;
	void* user;
} UserData;

static bool SPT_tris_getPt(SPT_pt* pt, void* user) {
	UserData *user2 = (UserData *)user;
	return user2->getPt(pt, user2->user);
}

static bool SPT_tris_addQuad(SPT_quad quad, SPT_color color, void* user) {
	const SPT_tri tri1 = {quad.a, quad.b, quad.c};
	const SPT_tri tri2 = {quad.a, quad.c, quad.d};
	UserData *user2 = (UserData *)user;
	bool out = true;
	out &= user2->addTri(tri1, color, user2->user);
	out &= user2->addTri(tri2, color, user2->user);
	return out;
}

static bool SPT_tris_addArc(SPT_vec2 pos, float a1, float a2, float r, SPT_color color, void* user) {
	UserData *user2 = (UserData *)user;
	const int segments = ceil((a2 - a1) / 0.5f);
	bool out = true;
	for (int i = 0; i < segments; ++i) {
		const float start = a1 + (a2 - a1) * ((float)i / segments);
		const float stop = a1 + (a2 - a1) * ((float)(i + 1) / segments);
		const SPT_tri tri = {pos, vadd(pos, vmulf(vdir(start), r)), vadd(pos, vmulf(vdir(stop), r))};
		out &= user2->addTri(tri, color, user2->user);
	}
	return out;
}

void SPT_tris(SPT_cb_get_pt getPt, SPT_cb_add_tri addTri, void* user) {
	UserData user2 = {getPt, addTri, user};
	SPT_prims(SPT_tris_getPt, SPT_tris_addQuad, SPT_tris_addArc, &user2);
}
