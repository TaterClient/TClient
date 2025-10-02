// https://github.com/SollyBunny/stroked-polyline-tesselation

#ifndef STROKED_POLYLINE_TESSELATION_H
#define STROKED_POLYLINE_TESSELATION_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
	float r, g, b, a;
} SPT_color;

typedef struct {
	float x, y;
} SPT_vec2;

typedef struct {
	SPT_vec2 a, b, c;
} SPT_tri;

typedef struct {
	SPT_vec2 a, b, c, d;
} SPT_quad;

typedef struct {
	SPT_vec2 pos;
	float w;
	SPT_color color;
} SPT_pt;

typedef bool (*SPT_cb_get_pt)(SPT_pt* pt, void* user);
typedef bool (*SPT_cb_add_tri)(SPT_tri tri, SPT_color color, void* user);
typedef bool (*SPT_cb_add_quad)(SPT_quad quad, SPT_color color, void* user);
typedef bool (*SPT_cb_add_arc)(SPT_vec2 pos, float a1, float a2, float r, SPT_color color, void* user);

void SPT_tris(SPT_cb_get_pt getPt, SPT_cb_add_tri addTri, void* user);
void SPT_prims(SPT_cb_get_pt getPt, SPT_cb_add_quad addQuad, SPT_cb_add_arc addArc, void* user);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <functional>
using SPT_cb_get_pt_f = std::function<bool(SPT_pt* pt)>;
using SPT_cb_add_tri_f = std::function<bool(SPT_tri tri, SPT_color color)>;
using SPT_cb_add_quad_f = std::function<bool(SPT_quad quad, SPT_color color)>;
using SPT_cb_add_arc_f = std::function<bool(SPT_vec2 pos, float a1, float a2, float r, SPT_color color)>;

static inline void SPT_tris(SPT_cb_get_pt_f getPt, SPT_cb_add_tri_f addTri) {
	using UserData = struct {
		SPT_cb_get_pt_f getPt;
		SPT_cb_add_tri_f addTri;
	};
	UserData userData = {getPt, addTri};
	auto getPtOld = [](SPT_pt* pt, void* user) { return ((UserData *)user)->getPt(pt); };
	auto addTriOld = [](SPT_tri tri, SPT_color color, void* user) { return ((UserData *)user)->addTri(tri, color); };
	SPT_tris(getPtOld, addTriOld, &userData);
}
static inline void SPT_prims(SPT_cb_get_pt_f getPt, SPT_cb_add_quad_f addQuad, SPT_cb_add_arc_f addArc) {
	using UserData = struct {
		SPT_cb_get_pt_f getPt;
		SPT_cb_add_quad_f addQuad;
		SPT_cb_add_arc_f addArc;
	};
	UserData userData = {getPt, addQuad, addArc};
	auto getPtOld = [](SPT_pt* pt, void* user) { return ((UserData *)user)->getPt(pt); };
	auto addQuadOld = [](SPT_quad quad, SPT_color color, void* user) { return ((UserData *)user)->addQuad(quad, color); };
	auto addQuadArc = [](SPT_vec2 pos, float a1, float a2, float r, SPT_color color, void* user) { return ((UserData *)user)->addArc(pos, a1, a2, r, color); };
	SPT_prims(getPtOld, addQuadOld, addQuadArc, &userData);
}

#endif

#endif
