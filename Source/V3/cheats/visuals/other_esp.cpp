#include "other_esp.h"
#include "..\autowall\autowall.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"
#include "..\misc\misc.h"
#include "../Configuration/Config.h"
#include "..\lagcompensation\local_animations.h"
#include "../Menu/MenuFramework/Framework.h"
#include "../Menu/MenuFramework/Renderer.h"
#include "../menu_alpha.h"

using namespace IdaLovesMe;

bool can_penetrate(weapon_t* weapon)
{
	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	trace_t trace;
	util::trace_line(g_ctx.globals.eye_pos, g_ctx.globals.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f) //-V550
		return false;

	auto eye_pos = g_ctx.globals.eye_pos;
	auto hits = 1;
	auto damage = (float)weapon_info->iDamage;
	auto penetration_power = weapon_info->flPenetration;

	static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
	static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

	return autowall::get().handle_bullet_penetration(weapon_info, trace, eye_pos, direction, hits, damage, penetration_power, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat());
}

void otheresp::penetration_reticle()
{
	if (!c_config::get()->auto_check(c_config::get()->i["esp_en"], c_config::get()->i["esp_en_type"]))
		return;

	if (!c_config::get()->b["penrect"])
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto color = Color::Red;

	if (!weapon->is_non_aim() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && can_penetrate(weapon))
		color = Color::Green;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	render::get().rect_filled(width / 2, height / 2 - 1, 1, 3, color);
	render::get().rect_filled(width / 2 - 1, height / 2, 3, 1, color);
}

void otheresp::velocity()
{
	if (!g_cfg.esp.Velocity_graph)
		return;

	if (!g_ctx.local())
		return;

	if (!m_engine()->IsInGame() || !g_ctx.local()->is_alive())
		return;

	static std::vector<float> velData(120, 0);

	Vector vecVelocity = g_ctx.local()->m_vecVelocity();
	float currentVelocity = sqrt(vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y);

	velData.erase(velData.begin());
	velData.push_back(currentVelocity);

	int vel = g_ctx.local()->m_vecVelocity().Length2D();

	static int width, height;
	m_engine()->GetScreenSize(width, height);
	render::get().text(fonts[VELOCITY], width / 2, height / 1.1, Color(0, 255, 100, 255), HFONT_CENTERED_X | HFONT_CENTERED_Y, "[%i]", vel);


	for (auto i = 0; i < velData.size() - 1; i++)
	{
		int cur = velData.at(i);
		int next = velData.at(i + 1);

		render::get().line(
			width / 2 + (velData.size() * 5 / 2) - (i - 1) * 5.f,
			height / 1.15 - (std::clamp(cur, 0, 450) * .2f),
			width / 2 + (velData.size() * 5 / 2) - i * 5.f,
			height / 1.15 - (std::clamp(next, 0, 450) * .2f), Color(255, 255, 255, 255)
		);
	}
}

enum key_bind_num
{
	_AUTOFIRE,
	_LEGITBOT,
	_DOUBLETAP,
	_SAFEPOINT,
	_MIN_DAMAGE,
	_ANTI_BACKSHOT = 12,
	_M_BACK,
	_M_LEFT,
	_M_RIGHT,
	_DESYNC_FLIP,
	_THIRDPERSON,
	_AUTO_PEEK,
	_EDGE_JUMP,
	_FAKEDUCK,
	_SLOWWALK,
	_BODY_AIM,
	_RAGEBOT,
	_TRIGGERBOT,
	_L_RESOLVER_OVERRIDE,
	_FAKE_PEEK,
};



void otheresp::indicators()
{
	static int height, width;
	m_engine()->GetScreenSize(width, height);

	g_ctx.globals.indicator_pos = height / 2;

	static auto percent_col = [](int per) -> Color {
		int red = per < 50 ? 255 : floorf(255 - (per * 2 - 100) * 255.f / 100.f);
		int green = per > 50 ? 255 : floorf((per * 2) * 255.f / 100.f);

		return Color(red, green, 0);
	};
	auto cfg = c_config::get();

	if (cfg->m["ftind"][0] && cfg->auto_check(cfg->i["rage_sp_enabled"], cfg->i["rage_sp_enabled_style"]))
	{
		otheresp::get().m_indicators.push_back({ "SP", D3DCOLOR_RGBA(255, 255, 0,255) });
	}

	if (cfg->m["ftind"][1] && cfg->auto_check(cfg->i["rage_baim_enabled"], cfg->i["rage_baim_enabled_style"]))
	{
		otheresp::get().m_indicators.push_back({ "BAIM", D3DCOLOR_RGBA(255, 255, 255,255) });
	}

	Color PINGCOL = percent_col((float(std::clamp(g_ctx.globals.ping, 30, 200) - 64)) / 200.f * 100.f);
	if (cfg->m["ftind"][2] && (cfg->b["misc_ping_spike"] && cfg->auto_check(cfg->i["ping_spike_key"], cfg->i["ping_spike_key_style"])))
	{
		otheresp::get().m_indicators.push_back({ "PING", D3DCOLOR_RGBA(PINGCOL.r(), PINGCOL.g(), PINGCOL.b(),255) });
	}

	if (cfg->m["ftind"][3] && (cfg->b["rage_dt"] && cfg->auto_check(cfg->i["rage_dt_key"], cfg->i["rage_dt_key_style"])))
	{
		otheresp::get().m_indicators.push_back({ "DT", D3DCOLOR_RGBA(255, 255, 255, 255) });
	}

	if (cfg->m["ftind"][4] && cfg->auto_check(cfg->i["rage_fd_enabled"], cfg->i["rage_fd_enabled_style"]))
	{
		otheresp::get().m_indicators.push_back({ "DUCK", D3DCOLOR_RGBA(255, 255, 255,255) });
	}

	if (cfg->m["ftind"][5] && cfg->i["aa_fs"] == 0 && cfg->auto_check(cfg->i["freestand_key"], cfg->i["freestand_key_style"]))
	{
		otheresp::get().m_indicators.push_back({ "FS", D3DCOLOR_RGBA(255, 255, 255,255) });
	}

	Color FPSCOL = percent_col((float(std::clamp(g_ctx.globals.framerate, 30, 80) - 64)) / 200.f * 100.f);

	if (cfg->b["fps_warning"])
	{
		otheresp::get().m_indicators.push_back({ "FPS", D3DCOLOR_RGBA(FPSCOL.r(), FPSCOL.g(), FPSCOL.b(),255) });
	}

}

void otheresp::create_fonts() {

	static auto create_font2 = [](const char* name, int size, int weight, DWORD flags) -> vgui::HFont
	{
		g_ctx.last_font_name = name;

		auto font = m_surface()->FontCreate();
		m_surface()->SetFontGlyphSet(font, name, size, weight, 0, 0, flags);

		return font;
	};



	IndFont = create_font2("Calibri Bold", 27 * CMenu::get()->GetDPINum(), 560, FONTFLAG_ANTIALIAS);
	IndShadow = create_font2("Calibri Bold", 27 * CMenu::get()->GetDPINum(), 700, FONTFLAG_ANTIALIAS);
}

void otheresp::draw_indicators()
{

	//if (!g_ctx.local()->is_alive()) //-V807
		//return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	int h = 0;


	for (auto& indicator : m_indicators)
	{

		render::get().gradient(14, height - 300 - h - 3, render::get().text_width(IndShadow, indicator.str.c_str()) / 2, 33, Color(0, 0, 0, 0), Color(0, 0, 0, 165), GRADIENT_HORIZONTAL);
		render::get().gradient(14 + render::get().text_width(IndShadow, indicator.str.c_str()) / 2, height - 300 - h - 3, render::get().text_width(IndShadow, indicator.str.c_str()) / 2, 33, Color(0, 0, 0, 165), Color(0, 0, 0, 0), GRADIENT_HORIZONTAL);
		render::get().text(IndShadow, 23, height - 300 - h, Color(0, 0, 0, 200), HFONT_CENTERED_NONE, indicator.str.c_str());
		render::get().text(IndFont, 23, height - 300 - h, Color((int)get_r(indicator.color), (int)get_g(indicator.color), (int)get_b(indicator.color), (int)get_a(indicator.color)), HFONT_CENTERED_NONE, indicator.str.c_str());


		//DrawList::AddGradient(Vec2(14, height - 340 - h - 3), Vec2(Render::Draw->GetTextSize(Render::Fonts::Indicator, indicator.m_text).x, 33), D3DCOLOR_RGBA(0, 0, 0, 0), D3DCOLOR_RGBA(0, 0, 0, 125));
		//DrawList::AddGradient(Vec2(14 + Render::Draw->GetTextSize(Render::Fonts::Indicator, indicator.m_text).x, height - 340 - h - 3), Vec2(Render::Draw->GetTextSize(Render::Fonts::Indicator, indicator.m_text).x, 33), D3DCOLOR_RGBA(0, 0, 0, 125), D3DCOLOR_RGBA(0, 0, 0, 0));
	   // Render::Draw->Text(indicator.str.c_str(), 23 + 1, height - 340 - h + 1, LEFT, false, Render::Fonts::Indicator, D3DCOLOR_RGBA(0, 0, 0, 150));
		//Render::Draw->Text(indicator.str.c_str(), 23, height - 340 - h, LEFT, false, Render::Fonts::Indicator, indicator.color);

		h += 36;
	}


	m_indicators.clear();

}

void otheresp::hitmarker_paint()
{
	auto cfg = c_config::get();
	if (!cfg->b["hitmarker"])
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (!g_ctx.local()->is_alive())
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (hitmarker.hurt_time + 0.7f > m_globals()->m_curtime)
	{
		if (cfg->b["hitmarker"])
		{
			static int width, height;
			m_engine()->GetScreenSize(width, height);

			auto alpha = (int)((hitmarker.hurt_time + 0.7f - m_globals()->m_curtime) * 255.0f);
			hitmarker.hurt_color.SetAlpha(alpha);

			auto offset = 7.0f - (float)alpha / 255.0f * 7.0f;

			render::get().line(width / 2 + 5 + offset, height / 2 - 5 - offset, width / 2 + 12 + offset, height / 2 - 12 - offset, hitmarker.hurt_color);
			render::get().line(width / 2 + 5 + offset, height / 2 + 5 + offset, width / 2 + 12 + offset, height / 2 + 12 + offset, hitmarker.hurt_color);
			render::get().line(width / 2 - 5 - offset, height / 2 + 5 + offset, width / 2 - 12 - offset, height / 2 + 12 + offset, hitmarker.hurt_color);
			render::get().line(width / 2 - 5 - offset, height / 2 - 5 - offset, width / 2 - 12 - offset, height / 2 - 12 - offset, hitmarker.hurt_color);
		}


	}
}

void otheresp::damage_marker_paint()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		if (damage_marker[i].hurt_time + 2.0f > m_globals()->m_curtime)
		{
			Vector screen;

			if (!math::world_to_screen(damage_marker[i].position, screen))
				continue;

			auto alpha = (int)((damage_marker[i].hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);
			damage_marker[i].hurt_color.SetAlpha(alpha);

			render::get().text(fonts[DAMAGE_MARKER], screen.x, screen.y, damage_marker[i].hurt_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, "%i", damage_marker[i].damage);
		}
	}
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device);

void otheresp::spread_crosshair(LPDIRECT3DDEVICE9 device)
{
	//
}

void otheresp::custom_scopes_lines()
{
	if (!g_ctx.local()->is_alive()) //-V807
		return;

	if (!g_cfg.esp.removals[REMOVALS_SCOPE])
		return;

	if (c_config::get()->b["customscope"])
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto is_scoped = g_ctx.globals.scoped && weapon->is_sniper() && weapon->m_zoomLevel();

	if (!is_scoped)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto offset = g_cfg.esp.scopes_line_offset;
	auto leng = g_cfg.esp.scopes_line_width;
	auto accent = g_cfg.esp.scopes_line_color;
	auto accent2 = Color(g_cfg.esp.scopes_line_color.r(), g_cfg.esp.scopes_line_color.g(), g_cfg.esp.scopes_line_color.b(), 0);

	render::get().gradient(width / 2 + offset, height / 2, leng, 1, accent, accent2, GradientType::GRADIENT_HORIZONTAL);
	render::get().gradient(width / 2 - leng - offset, height / 2, leng, 1, accent2, accent, GradientType::GRADIENT_HORIZONTAL);
	render::get().gradient(width / 2, height / 2 + offset, 1, leng, accent, accent2, GradientType::GRADIENT_VERTICAL);
	render::get().gradient(width / 2, height / 2 - leng - offset, 1, leng, accent2, accent, GradientType::GRADIENT_VERTICAL);
}

void draw_circe(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2 = nullptr;
	std::vector <CUSTOMVERTEX2> circle(resolution + 2);

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0.0f;

	circle[0].rhw = 1.0f;
	circle[0].color = color2;

	for (auto i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - radius * cos(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - radius * sin(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0.0f;

		circle[i].rhw = 1.0f;
		circle[i].color = color;
	}

	device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX2), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, nullptr); //-V107

	if (!g_pVB2)
		return;

	void* pVertices;

	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX2), (void**)&pVertices, 0); //-V107
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX2));
	g_pVB2->Unlock();

	device->SetTexture(0, nullptr);
	device->SetPixelShader(nullptr);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX2));
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);

	g_pVB2->Release();
}

void otheresp::automatic_peek_indicator()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();
	auto color_main = Color(c_config::get()->c["rage_quick_peek_assist_col"][0], c_config::get()->c["rage_quick_peek_assist_col"][1], c_config::get()->c["rage_quick_peek_assist_col"][2]);

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!g_ctx.globals.start_position.IsZero())
		position = g_ctx.globals.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 2.f;

	if (!weapon->is_non_aim() && c_config::get()->b["rage_quick_peek_assist"] && c_config::get()->auto_check(c_config::get()->i["rage_quickpeek_enabled"], c_config::get()->i["rage_quickpeek_enabled_style"])) {

		render::get().Draw3DCircleGradient(position, 32, color_main, alpha);
	}
}
