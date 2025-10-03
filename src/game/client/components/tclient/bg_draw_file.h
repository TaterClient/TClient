#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_BG_DRAW_FILE_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_BG_DRAW_FILE_H

// bg_draw_file.{cpp,h} can be used seperatley

#include <cstdio>
#include <functional>
#include <vector>

class CBgDrawItemDataPoint
{
public:
	float x, y;
	float w;
	float r, g, b, a;

	CBgDrawItemDataPoint(float X, float Y, float W, float R, float G, float B, float A) :
		x(X), y(Y), w(W), r(R), g(G), b(B), a(A) {}

#if defined(BASE_VMATH_H) && defined(BASE_COLOR_H)
	CBgDrawItemDataPoint(vec2 Pos, float Width, ColorRGBA Color) :
		x(Pos.x), y(Pos.y), w(Width), r(Color.r), g(Color.g), b(Color.b), a(Color.a)
	{
	}
	ColorRGBA Color() const
	{
		return ColorRGBA(r, g, b, a);
	}
	vec2 Pos() const
	{
		return vec2(x, y);
	}
#endif
};

using CBgDrawItemData = std::vector<CBgDrawItemDataPoint>;

namespace BgDrawFile
{
	[[nodiscard]] bool Write(const std::function<bool(const char *)> &WriteLine, const CBgDrawItemData &Data);
	[[nodiscard]] bool Read(const std::function<bool(char *pBuf, int Length)> &ReadLine, CBgDrawItemData &Data);
	[[nodiscard]] bool Write(FILE *pFile, const CBgDrawItemData &Data);
	[[nodiscard]] bool Read(FILE *pFile, CBgDrawItemData &Data);
#ifdef BASE_SYSTEM_H
	[[nodiscard]] bool Write(IOHANDLE File, const CBgDrawItemData &Data)
	{
		return Write((FILE *)File, Data);
	}
	[[nodiscard]] bool Read(IOHANDLE File, CBgDrawItemData &Data)
	{
		return Read((FILE *)File, Data);
	}
#endif
}; // namespace BgDrawFile

#endif
