#include "Render.h"
#define PI			3.14159265358979323846
#include "../cheats/visuals/player_esp.h"
#include "../cheats/visuals/GrenadePrediction.h"
#include "../cheats/visuals/other_esp.h"
#include "../cheats/visuals/world_esp.h"
#include "../cheats/ragebot/aim.h"
#include "../menu/menu.h"
#include "../cheats/misc/misc.h"
#include "../Menu/MenuFramework/Renderer.h"

ImColor u32(Color col)
{
	return ImColor(col.r(), col.g(), col.b(), col.a());
}

void ImGuiRendering::BeginScene(IDirect3DDevice9* pDevice)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.0f, 0.0f, 0.0f, 0.0f });
	ImGui::Begin("##Backbuffer", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs);

	ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImGuiCond_Always);

	if (m_engine()->IsInGame() && m_engine()->IsConnected())
	{
		otheresp::get().automatic_peek_indicator();
	}
}

void ImGuiRendering::EndScene()
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->PushClipRectFullScreen();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::End();
}

void ImGuiRendering::arc(float x, float y, float radius, float min_angle, float max_angle, Color col, float thickness) 
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->PathArcTo(ImVec2(x, y), radius, DEG2RAD(min_angle), DEG2RAD(max_angle), 32);
	window->DrawList->PathStroke(col.u32(), false, thickness);
}

void ImGuiRendering::two_sided_arc(float x, float y, float radius, float scale, Color col, float thickness)
{
	arc(x, y, radius, -270, -90 - 90 * (scale * 2), col, thickness);
	arc(x, y, radius, -90 + 90 * (scale * 2), 90, col, thickness);
};

void ImGuiRendering::PolyLine(const ImVec2* points, const int points_count, Color clr, bool closed, float thickness, Color fill)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddConvexPolyFilled(points, points_count, u32(fill));
	window->DrawList->AddPolyline(points, points_count, u32(clr), closed, thickness);
}

void ImGuiRendering::build_lookup_table()
{
	if (!lookup_table.empty())
		return;

	for (auto i = 0; i <= points; i++)
		lookup_table.emplace_back(
			std::cos(2.f * D3DX_PI * (i / static_cast<float>(points))),
			std::sin(2.f * D3DX_PI * (i / static_cast<float>(points)))
		);
}

void ImGuiRendering::circle_filled_radial_gradient(IDirect3DDevice9* dev, Vector2D center, float radius, Color color1, Color color2)
{
	const auto col1 = color1.u32();
	const auto col2 = color2.u32();
	build_lookup_table();

	vertex vert[points + 2] = {};

	for (auto i = 1; i <= points; i++)
		vert[i] =
	{
		center.x + radius * lookup_table[i].x,
		center.y - radius * lookup_table[i].y,
		0.0f,
		1.0f,
		col1
	};

	vert[0] = { center.x, center.y, 0.0f, 1.0f, col2 };
	vert[points + 1] = vert[1];

	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, true);
	dev->SetTexture(0, nullptr);
	dev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, points, &vert, sizeof vertex);
	dev->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, false);
}


void ImGuiRendering::DrawRing3D1(int16_t x, int16_t y, int16_t z, int16_t radius, uint16_t points, Color color1, Color color2, float thickness, float prog, bool fill_prog)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	static float Step = (M_PI * 2.0f) / (points);
	std::vector<ImVec2> point;
	for (float lat = 0.f; lat <= M_PI * 2.0f; lat += Step)
	{
		const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
		Vector point2d;
		if (math::world_to_screen(Vector(x, y, z) + point3d, point2d))
			point.push_back(ImVec2(point2d.x, point2d.y));
	}
	window->DrawList->AddConvexPolyFilled(point.data(), point.size(), u32(color2));
	window->DrawList->AddPolyline(point.data(), max(0, point.size() * prog), u32(color1), prog == 1.f, thickness);
}

void ImGuiRendering::DrawCollisionRing3D(int16_t x, int16_t y, int16_t z, int16_t radius, uint16_t points, Color color1, Color color2, float thickness, float prog)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	static auto prevScreenPos = ZERO; //-V656
	static auto step = M_PI * 2.0f / points;
	Vector origin = Vector(x, y, z);
	auto screenPos = ZERO;
	std::vector<ImVec2> point;
	for (float rotation = 0.0f; rotation <= M_PI * 2.0f; rotation += step) //-V1034
	{
		Vector pos = Vector(x, y, z) + (Vector(sin(rotation), cos(rotation), 0.f) * radius);

		Ray_t ray;
		trace_t trace;
		CTraceFilterWorldOnly filter;

		ray.Init(origin, pos);
		m_trace()->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace);

		Ray_t ray2;
		CTraceFilterWorldAndPropsOnly filter2;
		CGameTrace trace2;

		ray2.Init(trace.endpos, trace.endpos - Vector(0.0f, 0.0f, 140.0f));
		m_trace()->TraceRay(ray2, MASK_SHOT_BRUSHONLY, &filter2, &trace2);
		Vector real_end;
		if (trace2.fraction < 1.0f)
			real_end = trace2.endpos + Vector(0.0f, 0.0f, 2.0f);
		if (math::world_to_screen(real_end, screenPos))
		{
			if (!prevScreenPos.IsZero())
				point.push_back(ImVec2(screenPos.x, screenPos.y));
			prevScreenPos = screenPos;
		}
	}

	window->DrawList->AddConvexPolyFilled(point.data(), point.size(), u32(color2));
	window->DrawList->AddPolyline(point.data(), max(0, point.size() * prog), u32(color1), prog == 1.f, thickness);
}

void ImGuiRendering::DrawRing3D(int16_t x, int16_t y, int16_t z, int16_t radius, uint16_t points, Color color1, Color color2, float thickness, float prog, bool fill_prog)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	static float Step = (M_PI * 2.0f) / (points);
	std::vector<ImVec2> point;
	for (float lat = 0.f; lat <= M_PI * 2.0f; lat += Step)
	{
		const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
		Vector point2d;
		if (math::world_to_screen(Vector(x, y, z) + point3d, point2d))
			point.push_back(ImVec2(point2d.x, point2d.y));
	}
	window->DrawList->AddConvexPolyFilled(point.data(), point.size(), u32(color2));
	window->DrawList->AddPolyline(point.data(), max(0, point.size() * prog), u32(color1), prog == 1.f, thickness);
}

void ImGuiRendering::DrawCircle(float x, float y, float r, float s, Color color)
{
	float Step = PI * 2.0 / s;
	for (float a = 0; a < (PI * 2.0); a += Step)
	{
		float x1 = r * cos(a) + x;
		float y1 = r * sin(a) + y;
		float x2 = r * cos(a + Step) + x;
		float y2 = r * sin(a + Step) + y;

		DrawLine(x1, y1, x2, y2, color);
	}
}

struct CUSTOMVERTEX 
{
	FLOAT x, y, z;
	FLOAT rhw;
	DWORD color;
};

void ImGuiRendering::DrawGradientCircle(float x, float y, float rad, float rotate, int type, int resolution, DWORD color, DWORD color2)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2;

	std::vector<CUSTOMVERTEX> circle(resolution + 2);

	float angle = rotate * D3DX_PI / 180, pi = D3DX_PI;

	if (type == 1)
		pi = D3DX_PI;
	if (type == 2)
		pi = D3DX_PI / 2; 
	if (type == 3)
		pi = D3DX_PI / 4; 

	pi = D3DX_PI / type; 

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0;
	circle[0].rhw = 1;
	circle[0].color = color2;

	for (int i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - rad * cos(pi * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - rad * sin(pi * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0;
		circle[i].rhw = 1;
		circle[i].color = color;
	}

	int _res = resolution + 2;
	for (int i = 0; i < _res; i++)
	{
		circle[i].x = x + cos(angle) * (circle[i].x - x) - sin(angle) * (circle[i].y - y);
		circle[i].y = y + sin(angle) * (circle[i].x - x) + cos(angle) * (circle[i].y - y);
	}

	if (g_pVB2 != NULL)
		g_pVB2->Release();
}

void ImGuiRendering::DrawString(float x, float y, Color color, int flags, ImFont* font, const char* message, ...)
{
	char output[4096] = {};
	va_list args;
	va_start(args, message);
	vsprintf(output, message, args);
	va_end(args);

	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->PushTextureID(font->ContainerAtlas->TexID);
	ImGui::PushFont(font);
	auto coord = ImVec2(x, y);
	auto size = ImGui::CalcTextSize(output);
	auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
	Color outline_clr = Color(0, 0, 0, color.a());

	int width = 0, height = 0;

	if (!(flags & render2::centered_x))
		size.x = 0;
	if (!(flags & render2::centered_y))
		size.y = 0;

	ImVec2 pos = ImVec2(coord.x - (size.x * .5), coord.y - (size.y * .5));

	if (flags & render2::outline)
	{
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y + 1), outline_clr.u32(), output);

		window->DrawList->AddText(ImVec2(pos.x, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y), outline_clr.u32(), output);
	}

	window->DrawList->AddText(pos, color.u32(), output);
	ImGui::PopFont();
}

void ImGuiRendering::DrawString2(float x, float y, Color color, int flags, ImFont* font, const std::string message, ...)
{
	char output[1024] = {};
	va_list args;
	va_start(args, message);
	vsprintf_s(output, message.c_str(), args);
	va_end(args);

	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->PushTextureID(font->ContainerAtlas->TexID);
	ImGui::PushFont(font);
	auto coord = ImVec2(x, y);
	auto size = ImGui::CalcTextSize(output);
	auto coord_out = ImVec2{ coord.x + 1.f, coord.y + 1.f };
	Color outline_clr = Color(0, 0, 0, color.a());

	int width = 0, height = 0;

	if (!(flags & render2::centered_x))
		size.x = 0;
	if (!(flags & render2::centered_y))
		size.y = 0;

	ImVec2 pos = ImVec2(coord.x - (size.x * .5), coord.y - (size.y * .5));

	if (flags & render2::outline)
	{
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y + 1), outline_clr.u32(), output);

		window->DrawList->AddText(ImVec2(pos.x, pos.y + 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x, pos.y - 1), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x + 1, pos.y), outline_clr.u32(), output);
		window->DrawList->AddText(ImVec2(pos.x - 1, pos.y), outline_clr.u32(), output);
	}

	window->DrawList->AddText(pos, color.u32(), output);
	ImGui::PopFont();
}

void ImGuiRendering::DrawEspBox(Vector leftUpCorn, Vector rightDownCorn, Color clr, float width)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImVec2 min = ImVec2(leftUpCorn.x, leftUpCorn.y);
	ImVec2 max = ImVec2(rightDownCorn.x, rightDownCorn.y);
	window->DrawList->AddRect(min, max, clr.u32(), 0.0F, -1, width);
}

void ImGuiRendering::DrawLine(float x1, float y1, float x2, float y2, Color clr, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), clr.u32(), thickness);
}


void ImGuiRendering::DrawImage(float x1, float y1, float x2, float y2, Color clr, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), clr.u32(), thickness);
}

void ImGuiRendering::filled_rect_world(Vector center, Vector2D size, Color color, int angle) {
	Vector add_top_left, add_top_right, add_bot_left, add_bot_right;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	switch (angle) 
	{
	case 0: 
		add_top_left = Vector(-size.x, -size.y, 0);
		add_top_right = Vector(size.x, -size.y, 0);

		add_bot_left = Vector(-size.x, size.y, 0);
		add_bot_right = Vector(size.x, size.y, 0);

		break;
	case 1: 
		add_top_left = Vector(-size.x, 0, -size.y);
		add_top_right = Vector(size.x, 0, -size.y);

		add_bot_left = Vector(-size.x, 0, size.y);
		add_bot_right = Vector(size.x, 0, size.y);

		break;
	case 2: 
		add_top_left = Vector(0, -size.y, -size.x);
		add_top_right = Vector(0, -size.y, size.x);

		add_bot_left = Vector(0, size.y, -size.x);
		add_bot_right = Vector(0, size.y, size.x);

		break;
	}

	Vector c_top_left = center + add_top_left;
	Vector c_top_right = center + add_top_right;

	Vector c_bot_left = center + add_bot_left;
	Vector c_bot_right = center + add_bot_right;

	Vector top_left, top_right, bot_left, bot_right;
	if (math::world_to_screen(c_top_left, top_left) &&
		math::world_to_screen(c_top_right, top_right) &&
		math::world_to_screen(c_bot_left, bot_left) &&
		math::world_to_screen(c_bot_right, bot_right)) 
	{

		std::vector<ImVec2> vertices;

		vertices.push_back(ImVec2(top_left.x, top_left.y));
		vertices.push_back(ImVec2(top_right.x, top_right.y));
		vertices.push_back(ImVec2(bot_right.x, bot_right.y));
		vertices.push_back(ImVec2(bot_left.x, bot_left.y));

		window->DrawList->AddConvexPolyFilled(vertices.data(), vertices.size(), u32(color));

	}
}

void ImGuiRendering::DrawLineGradient(float x1, float y1, float x2, float y2, Color clr1, Color cl2, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilledMultiColor(ImVec2(x1, y1), ImVec2(x2, y2),
		clr1.u32(), cl2.u32(), cl2.u32(), clr1.u32());
}

void ImGuiRendering::Rect(float x, float y, float w, float h, Color clr, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), clr.u32(), rounding);
}

void ImGuiRendering::FilledRect(float x, float y, float w, float h, Color clr, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), clr.u32(), rounding);
}

void ImGuiRendering::filled_rect_gradient(float x, float y, float w, float h, Color col_upr_left,
	Color col_upr_right, Color col_bot_right, Color col_bot_left)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + w, y + h),
		col_upr_left.u32(), col_upr_right.u32(), col_bot_right.u32(), col_bot_left.u32());
}

void ImGuiRendering::FilledRect2(float x, float y, float w, float h, Color clr, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddRectFilled(ImVec2(x, y), ImVec2(w, h), clr.u32(), rounding);
}

void ImGuiRendering::Triangle(float x1, float y1, float x2, float y2, float x3, float y3, Color clr, float thickness)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddTriangle(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), clr.u32(), thickness);
}

void ImGuiRendering::TriangleFilled(float x1, float y1, float x2, float y2, float x3, float y3, Color clr)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), clr.u32());
}

void ImGuiRendering::QuadFilled(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, Color clr)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddQuadFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImVec2(x4, y4), clr.u32());
}

void ImGuiRendering::CircleFilled(float x1, float y1, float radius, Color col, int segments)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	window->DrawList->AddCircleFilled(ImVec2(x1, y1), radius, col.u32(), segments);
}

void ImGuiRendering::RadialGradient3D(Vector pos, float radius, Color in, Color out, bool one) {
	ImVec2 center; Vector g_pos;

	// Use arc with automatic segment count
	static float m_flAnim = 0.f;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	m_flAnim += ImGui::GetIO().DeltaTime; if (m_flAnim > 1.f) m_flAnim = 0.f;
	math::world_to_screen(Vector(pos), g_pos);
	center = ImVec2(g_pos.x, g_pos.y);
	window->DrawList->_PathArcToFastEx(center, radius, 0, 48, 0);
	const int count = window->DrawList->_Path.Size - 1;
	float step = (3.141592654f * 2.0f) / (count + 1);
	std::vector<ImVec2> point;
	for (float lat = 0.f; lat <= 3.141592654f * 2.0f; lat += step)
	{
		const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
		Vector point2d;
		if (math::world_to_screen(Vector(pos) + point3d, point2d))
			point.push_back(ImVec2(point2d.x, point2d.y));
	}
	if (in.a() == 0 && out.a() == 0 || radius < 0.5f || point.size() < count + 1)
		return;

	unsigned int vtx_base = window->DrawList->_VtxCurrentIdx;
	window->DrawList->PrimReserve(count * 3, count + 1);

	// Submit vertices
	const ImVec2 uv = window->DrawList->_Data->TexUvWhitePixel;
	window->DrawList->PrimWriteVtx(center, uv, ImColor(in.r(), in.g(), in.b(), in.a()));
	for (int n = 0; n < count; n++)
		window->DrawList->PrimWriteVtx(point[n + 1], uv, ImColor(out.r(), out.g(), out.b(), out.a()));

	// Submit a fan of triangles
	for (int n = 0; n < count; n++)
	{
		window->DrawList->PrimWriteIdx((ImDrawIdx)(vtx_base));
		window->DrawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + n));
		window->DrawList->PrimWriteIdx((ImDrawIdx)(vtx_base + 1 + ((n + 1) % count)));
	}
	window->DrawList->_Path.Size = 0;
}

void ImGuiRendering::DrawWave(Vector loc, float radius, Color color, float thickness)
{
	static float Step = PI * 3.0f / 40;
	Vector prev;
	for (float lat = 0; lat <= PI * 3.0f; lat += Step)
	{
		float sin1 = sin(lat);
		float cos1 = cos(lat);
		float sin3 = sin(0.0f);
		float cos3 = cos(0.0f);

		Vector point1;
		point1 = Vector(sin1 * cos3, cos1, sin1 * sin3) * radius;
		Vector point3 = loc;
		Vector Out;
		point3 += point1;

		if (math::world_to_screen(point3, Out))
		{
			if (lat > 0.000f)
				DrawLine(prev.x, prev.y, Out.x, Out.y, color, thickness);
		}
		prev = Out;
	}
}