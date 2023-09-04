// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "player_esp.h"
#include "..\misc\misc.h"
#include "..\ragebot\aim.h"
#include "dormant_esp.h"
#include "../../Configuration/Config.h"
auto cfgsys = c_config::get();

class RadarPlayer_t
{
public:
	Vector pos; //0x0000
	Vector angle; //0x000C
	Vector spotted_map_angle_related; //0x0018
	DWORD tab_related; //0x0024
	char pad_0x0028[0xC]; //0x0028
	float spotted_time; //0x0034
	float spotted_fraction; //0x0038
	float time; //0x003C
	char pad_0x0040[0x4]; //0x0040
	__int32 player_index; //0x0044
	__int32 entity_index; //0x0048
	char pad_0x004C[0x4]; //0x004C
	__int32 health; //0x0050
	char name[32]; //0x785888
	char pad_0x0074[0x75]; //0x0074
	unsigned char spotted; //0x00E9
	char pad_0x00EA[0x8A]; //0x00EA
};

class CCSGO_HudRadar
{
public:
	char pad_0x0000[0x14C];
	RadarPlayer_t radar_info[65];
};

void playeresp::paint_traverse()
{
	static auto alpha = 1.0f;
	c_dormant_esp::get().start();

	if (g_cfg.player.arrows && g_ctx.local()->is_alive())
	{
		static auto switch_alpha = false;

		if (alpha <= 0.0f || alpha >= 1.0f)
			switch_alpha = !switch_alpha;

		alpha += switch_alpha ? 2.0f * m_globals()->m_frametime : -2.0f * m_globals()->m_frametime;
		alpha = math::clamp(alpha, 0.0f, 1.0f);
	}

	static auto FindHudElement = (DWORD(__thiscall*)(void*, const char*))util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28"));
	static auto hud_ptr = *(DWORD**)(util::FindSignature(crypt_str("client.dll"), crypt_str("81 25 ? ? ? ? ? ? ? ? 8B 01")) + 0x2);

	auto radar_base = FindHudElement(hud_ptr, "CCSGO_HudRadar");
	auto hud_radar = (CCSGO_HudRadar*)(radar_base - 0x14);

	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e->valid(false, false))
			continue;

		type = ENEMY;

		if (e == g_ctx.local())
			type = LOCAL;
		else if (e->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			type = TEAM;

		if (type == LOCAL)
			continue;
		

		auto valid_dormant = false;
		auto backup_flags = e->m_fFlags();
		auto backup_origin = e->GetAbsOrigin();

		if (e->IsDormant())
			valid_dormant = c_dormant_esp::get().adjust_sound(e);
		else
		{
			health[i] = e->m_iHealth();
			c_dormant_esp::get().m_cSoundPlayers[i].reset(true, e->GetAbsOrigin(), e->m_fFlags());
		}

		if (radar_base && hud_radar && e->IsDormant() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum() && e->m_bSpotted())
			health[i] = hud_radar->radar_info[i].health;

		if (!health[i])
		{
			if (e->IsDormant())
			{
				e->m_fFlags() = backup_flags;
				e->set_abs_origin(backup_origin);
			}

			continue;
		}

		auto fast = 2.5f * m_globals()->m_frametime; //-V807
		auto slow = 0.25f * m_globals()->m_frametime;

		if (e->IsDormant())
		{
			auto origin = e->GetAbsOrigin();

			if (origin.IsZero())
				esp_alpha_fade[i] = 0.0f;
			else if (!valid_dormant && esp_alpha_fade[i] > 0.0f)
				esp_alpha_fade[i] -= slow;
			else if (valid_dormant && esp_alpha_fade[i] < 1.0f)
				esp_alpha_fade[i] += fast;
		}
		else if (esp_alpha_fade[i] < 1.0f)
			esp_alpha_fade[i] += fast;

		esp_alpha_fade[i] = math::clamp(esp_alpha_fade[i], 0.0f, 1.0f);

		if (c_config::get()->b["skeleton"])
		{
			auto color = g_cfg.player.type[type].skeleton_color;
			color.SetAlpha(min(255.0f * esp_alpha_fade[i], color.a()));

#if RELEASE
			draw_skeleton(e, color, e->m_CachedBoneData().Base());
#else
			auto records = &player_records[i]; //-V826

			if (!records->empty() && g_ctx.local()->is_alive() && !e->IsDormant())
			{
				auto record = &records->front();

				draw_skeleton(e, color, record->m_Matricies[MatrixBoneSide::MiddleMatrix].data());
				draw_skeleton(e, Color::Red, record->m_Matricies[MatrixBoneSide::ZeroMatrix].data());
				draw_skeleton(e, Color::Green, record->m_Matricies[MatrixBoneSide::LeftMatrix].data());
				draw_skeleton(e, Color::Blue, record->m_Matricies[MatrixBoneSide::RightMatrix].data());
			}
			else
				draw_skeleton(e, color, e->m_CachedBoneData().Base());
#endif
		}

		Box box;

		if (util::get_bbox(e, box, true))
		{
			draw_box(e, box);
			draw_name(e, box);

			auto& hpbox = hp_info[i];

			if (hpbox.hp == -1)
				hpbox.hp = math::clamp(health[i], 0, 100);
			else
			{
				auto hp = math::clamp(health[i], 0, 100);

				if (hp != hpbox.hp)
				{
					if (hpbox.hp > hp)
					{
						if (hpbox.hp_difference_time) //-V550
							hpbox.hp_difference += hpbox.hp - hp;
						else
							hpbox.hp_difference = hpbox.hp - hp;

						hpbox.hp_difference_time = m_globals()->m_curtime;
					}
					else
					{
						hpbox.hp_difference = 0;
						hpbox.hp_difference_time = 0.0f;
					}

					hpbox.hp = hp;
				}

				if (m_globals()->m_curtime - hpbox.hp_difference_time > 0.2f && hpbox.hp_difference)
				{
					auto difference_factor = 4.0f * m_globals()->m_frametime * hpbox.hp_difference;

					hpbox.hp_difference -= difference_factor;
					hpbox.hp_difference = math::clamp(hpbox.hp_difference, 0, 100);

					if (!hpbox.hp_difference)
						hpbox.hp_difference_time = 0.0f;
				}
			}

			draw_health(e, box, hpbox);
			/*draw_weapon_icon*/(e, box, draw_ammobar(e, box));
			draw_flags(e, box);
		}

		if (type == ENEMY || type == TEAM)
		{
			//draw_lines(e);

			if (type == ENEMY)
			{
				if (cfgsys->b["oof"] && g_ctx.local()->is_alive())	
				{
					auto color = g_cfg.player.arrows_color;
					color.SetAlpha((int)(min(255.0f * esp_alpha_fade[i] * alpha, color.a())));

					if (e->IsDormant())
						color = Color(130, 130, 130, (int)(255.0f * esp_alpha_fade[i]));

					misc::get().PovArrows(e, color);
				}

				/*if (!e->IsDormant())
					draw_multi_points(e);*/
			}
		}

		if (e->IsDormant())
		{
			e->m_fFlags() = backup_flags;
			e->set_abs_origin(backup_origin);
		}
	}
}


void playeresp::draw_skeleton(player_t* e, Color color, matrix3x4_t matrix[MAXSTUDIOBONES])
{
	auto model = e->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto get_bone_position = [&](int bone) -> Vector
	{
		return Vector(matrix[bone][0][3], matrix[bone][1][3], matrix[bone][2][3]);
	};

	auto upper_direction = get_bone_position(7) - get_bone_position(6);
	auto breast_bone = get_bone_position(6) + upper_direction * 0.5f;

	for (auto i = 0; i < studio_model->numbones; i++)
	{
		auto bone = studio_model->pBone(i);

		if (!bone)
			continue;

		if (bone->parent == -1)
			continue;

		if (!(bone->flags & BONE_USED_BY_HITBOX))
			continue;

		auto child = get_bone_position(i);
		auto parent = get_bone_position(bone->parent);

		auto delta_child = child - breast_bone;
		auto delta_parent = parent - breast_bone;

		if (delta_parent.Length() < 9.0f && delta_child.Length() < 9.0f)
			parent = breast_bone;

		if (i == 5)
			child = breast_bone;

		if (fabs(delta_child.z) < 5.0f && delta_parent.Length() < 5.0f && delta_child.Length() < 5.0f || i == 6)
			continue;

		auto schild = ZERO;
		auto sparent = ZERO;

		if (math::world_to_screen(child, schild) && math::world_to_screen(parent, sparent))
			render::get().line(schild.x, schild.y, sparent.x, sparent.y, color);
	}
}

void playeresp::draw_box(player_t* m_entity, const Box& box)
{
	if (!cfgsys->b["bounding"])
		return;

	auto alpha = 255.0f * esp_alpha_fade[m_entity->EntIndex()];
	auto outline_alpha = (int)(alpha * 0.6f);

	Color outline_color
	{
		0,
		0,
		0,
		outline_alpha
	};

	Color boxclr
	{
		cfgsys->c["boundingcoll"][0],
		cfgsys->c["boundingcoll"][1],
		cfgsys->c["boundingcoll"][2],
		cfgsys->c["boundingcoll"][3]
	};

	auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : boxclr;
	color.SetAlpha(min(alpha, color.a()));

	render::get().rect(box.x - 1, box.y - 1, box.w + 2, box.h + 2, outline_color);
	render::get().rect(box.x, box.y, box.w, box.h, color);
	render::get().rect(box.x + 1, box.y + 1, box.w - 2, box.h - 2, outline_color);
}

void playeresp::draw_health(player_t* m_entity, const Box& box, const HPInfo& hpbox)
{
	if (!cfgsys->b["healbar"])
		return;

	Color Healcolor
	{
		cfgsys->c["healbarcoll"][0],
		cfgsys->c["healbarcoll"][1],
		cfgsys->c["healbarcoll"][2],
		cfgsys->c["hearbarcoll"][3]
	};
	auto alpha = (int)(255.0f * esp_alpha_fade[m_entity->EntIndex()]);

	auto text_color = m_entity->IsDormant() ? Color(130, 130, 130, alpha) : Color(255, 255, 255, alpha);
	auto back_color = Color(3, 3, 3, 200);
	auto nigga = Color(0, 0, 0);
	auto color = m_entity->IsDormant() ? Color(130, 130, 130) : Healcolor;

	color.SetAlpha(alpha);
	render::get().rect(box.x - 6, box.y - 1, 4, box.h + 2, nigga);

	Box n_box =
	{
		box.x - 5,
		box.y,
		2,
		box.h
	};

	auto bar_height = (int)((float)hpbox.hp * (float)n_box.h / 100.0f);
	auto offset = n_box.h - bar_height;

	render::get().rect_filled(n_box.x, n_box.y - 1, 2, n_box.h + 2, back_color);
	render::get().rect_filled(n_box.x, n_box.y + offset, 2, bar_height, color);

	auto height = n_box.h - hpbox.hp * n_box.h / 100;

	if (hpbox.hp < 100)
		render::get().text(fonts[ESP], n_box.x + 1, n_box.y + offset, text_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, std::to_string(hpbox.hp).c_str());
}

bool playeresp::draw_ammobar(player_t* m_entity, const Box& box)
{
	if (!m_entity->is_alive())
		return false;

	if (!cfgsys->b["ammo"])
		return false;

	auto weapon = m_entity->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return false;

	auto ammo = weapon->m_iClip1();

	auto alpha = (int)(255.0f * esp_alpha_fade[m_entity->EntIndex()]);
	auto outline_alpha = (int)(alpha * 0.7f);
	auto inner_back_alpha = (int)(alpha * 0.6f);

	Color outline_color =
	{
		0,
		0,
		0,
		outline_alpha
	};

	Color inner_back_color =
	{
		0,
		0,
		0,
		inner_back_alpha
	};

	Color ammocol
	{
		cfgsys->c["ammocoll"][0],
		cfgsys->c["ammocoll"][1],
		cfgsys->c["ammocoll"][2],
		cfgsys->c["ammocoll"][3]
	};

	auto text_color = m_entity->IsDormant() ? Color(130, 130, 130, alpha) : Color(255, 255, 255, alpha);
	auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : ammocol;

	color.SetAlpha(min(alpha, color.a()));

	Box n_box =
	{
		box.x,
		box.y + box.h + 3,
		box.w + 2,
		2
	};

	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	auto bar_width = ammo * box.w / weapon_info->iMaxClip1;
	auto reloading = false;

	auto animlayer = m_entity->get_animlayers()[1];

	if (animlayer.m_nSequence)
	{
		auto activity = m_entity->sequence_activity(animlayer.m_nSequence);

		reloading = activity == ACT_CSGO_RELOAD && animlayer.m_flWeight;

		if (reloading && animlayer.m_flCycle < 1.0f)
			bar_width = animlayer.m_flCycle * box.w;
	}

	render::get().rect_filled(n_box.x - 1, n_box.y - 1, n_box.w, 4, inner_back_color);
	render::get().rect_filled(n_box.x, n_box.y, bar_width, 2, color);

	if (weapon->m_iClip1() != weapon_info->iMaxClip1 && !reloading)
		render::get().text(fonts[ESP], n_box.x + bar_width, n_box.y + 1, text_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, std::to_string(ammo).c_str());

	return true;
}

//void playeresp::draw_grenades_name(player_t* m_entity, const Box& box) {
//
//	if (!cfgsys->b["nadename"]) // &cfg->b["nadename"]
//		return;
//
//	Color Namecolornades
//	{
//		cfgsys->c["namenadecoll"][0],
//		cfgsys->c["namenadecoll"][1],
//		cfgsys->c["namenadecoll"][2],
//		cfgsys->c["namenadecoll"][3]
//	};
//
//	std::string name = "lkj";
//	render::get().text(fonts[GRENADES], box.x + box.w / 2, box.y - 26, Color(Namecolornades), HFONT_CENTERED_X, name.c_str());
//
//}

void playeresp::draw_name(player_t* m_entity, const Box& box)
{
	if (!cfgsys->b["name"])
		return;

	static auto sanitize = [](char* name) -> std::string
	{
		name[127] = '\0';

		std::string tmp(name);

		if (tmp.length() > 20)
		{
			tmp.erase(20, tmp.length() - 20);
			tmp.append("...");
		}

		return tmp;
	};

	player_info_t player_info;

	if (m_engine()->GetPlayerInfo(m_entity->EntIndex(), &player_info))
	{
		auto name = sanitize(player_info.szName);

		auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].name_color;
		color.SetAlpha(min(255.0f * esp_alpha_fade[m_entity->EntIndex()], color.a()));

		render::get().text(fonts[NAME], box.x + box.w / 2, box.y - 13, color, HFONT_CENTERED_X, name.c_str());
	}
}

//void playeresp::draw_name(player_t* m_entity, const Box& box)
//{
//	auto dangerous = m_entity->m_bIsScoped();
//
//	if (!cfgsys->b["name"])
//		return;
//
//	static auto sanitize = [](char* name) -> std::string
//	{
//		name[127] = '\0';
//
//		std::string tmp(name);
//
//		if (tmp.length() > 20)
//		{
//			tmp.erase(20, tmp.length() - 20);
//			tmp.append("...");
//		}
//
//		return tmp;
//	};
//
//	player_info_t player_info;
//
//	Color Namecolor
//	{
//		cfgsys->c["namecoll"][0],
//		cfgsys->c["namecoll"][1],
//		cfgsys->c["namecoll"][2],
//		cfgsys->c["namecoll"][3]
//	};
//
//	Color Namecolornew
//	{
//		cfgsys->c["namecoll2"][0],
//		cfgsys->c["namecoll2"][1],
//		cfgsys->c["namecoll2"][2],
//		cfgsys->c["namecoll2"][3]
//	};
//
//	Color Namecolorother
//	{
//		cfgsys->c["namecollesp"][0],
//		cfgsys->c["namecollesp"][1],
//		cfgsys->c["namecollesp"][2],
//		cfgsys->c["namecollesp"][3]
//	};
//
//	if (m_engine()->GetPlayerInfo(m_entity->EntIndex(), &player_info))
//	{
//		auto name = sanitize(player_info.szName);
//
//		auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : Namecolor;
//		color.SetAlpha(min(255.0f * esp_alpha_fade[m_entity->EntIndex()], color.a()));
//
//		render::get().text(fonts[NAME], box.x + box.w / 2, box.y - 13, color, HFONT_CENTERED_X, name.c_str());
//	}
//
//	if (!cfgsys->b["warningvis"])
//		return;
//
//		if (dangerous)
//		{
//			std::string name = "SHIFTING TICKBASE";
//			render::get().text(fonts[NAME], box.x + box.w / 2, box.y - 23, Color(Namecolornew), HFONT_CENTERED_X, name.c_str());
//		}
//}

void playeresp::draw_weapon_icon(player_t* m_entity, const Box& box, bool space)
{
	if (!cfgsys->b["wepicon"] && !cfgsys->b["weptext"])
		return;

	auto weapon = m_entity->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto pos = box.y + box.h + 2;

	if (space)
		pos += 5;

	Color wepcolor
	{
		cfgsys->c["wepcoll"][0],
		cfgsys->c["wepcoll"][1],
		cfgsys->c["wepcoll"][2],
		cfgsys->c["wepcoll"][3]
	};

	Color wepcolor2 // icon
	{
		cfgsys->c["wepcoll2"][0],
		cfgsys->c["wepcoll2"][1],
		cfgsys->c["wepcoll2"][2],
		cfgsys->c["wepcoll2"][3]
	};

	auto color = m_entity->IsDormant() ? Color(130, 130, 130, 130) : wepcolor;
	color.SetAlpha(min(255.0f * esp_alpha_fade[m_entity->EntIndex()], color.a()));

	auto color2 = m_entity->IsDormant() ? Color(130, 130, 130, 130) : wepcolor2;			  // icon
	color2.SetAlpha(min(255.0f * esp_alpha_fade[m_entity->EntIndex()], color2.a()));			  // icon

	if (cfgsys->b["weptext"])
	{
		render::get().text(fonts[ESP], box.x + box.w / 2, pos, color, HFONT_CENTERED_X, weapon->get_name().c_str());
		pos += 11;
	}

	if (cfgsys->b["wepicon"])
	{
		if (weapon->m_iItemDefinitionIndex() == WEAPON_KNIFE)
			render::get().text(fonts[KNIFES], box.x + box.w / 2, pos, color2, HFONT_CENTERED_X, "z");
		else if (weapon->m_iItemDefinitionIndex() == WEAPON_KNIFE_T)
			render::get().text(fonts[KNIFES], box.x + box.w / 2, pos, color2, HFONT_CENTERED_X, "W");
		else
			render::get().text(fonts[SUBTABWEAPONS], box.x + box.w / 2, pos, color2, HFONT_CENTERED_X, weapon->get_icon());
	}
}

void playeresp::draw_flags(player_t* e, const Box& box)
{
	auto weapon = e->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	auto _x = box.x + box.w + 3, _y = box.y - 3;

	if (cfgsys->b["moneycustom"])
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(170, 190, 80);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		Color custommoney
		{
			cfgsys->c["moneycust"][0],
			cfgsys->c["moneycust"][1],
			cfgsys->c["moneycust"][2],
			cfgsys->c["moneycust"][3]
		}; 

		render::get().text(fonts[ESP], _x, _y, Color(custommoney), HFONT_CENTERED_NONE, "$%i", e->m_iAccount());
		_y += 8;
	}

	if (cfgsys->b["money"])
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(97, 149, 37); // 170, 190, 80
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "$%i", e->m_iAccount());
		_y += 8;
	}

	if (cfgsys->b["flags"])
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(255, 235, 229);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		auto kevlar = e->m_ArmorValue() > 0;
		auto helmet = e->m_bHasHelmet();

		std::string text;

		if (helmet && kevlar)
			text = "HK";
		else if (kevlar)
			text = "K";

		if (kevlar)
		{
			render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, text.c_str());
			_y += 8;
		}
	}

	if (cfgsys->b["flagskit"])
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(240, 240, 240);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);
	
		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "KIT");
		_y += 8;
	}

	if (cfgsys->b["oclusionflag"]) { // "O"
		
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(255, 255, 255);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "O");
		_y += 8;
	}

	if (cfgsys->b["xexploitflag"]) { // "X"
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(230, 46, 46);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "X");
		_y += 8;
	}

	if (cfgsys->b["fakeflag"]) {
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(230, 46, 46);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "FAKE");
		_y += 8;
	}

	//if (cfgsys->b["moveflag"]) {															                        // EDITED!!!
	//	auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(230, 46, 46);		                        // EDITED!!!
	//	color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);								                        // EDITED!!!

	//	Color moveflagcolor
	//	{
	//		cfgsys->c["moveflagcol"][0],
	//		cfgsys->c["moveflagcol"][1],
	//		cfgsys->c["moveflagcol"][2],
	//		cfgsys->c["moveflagcol"][3]
	//	};

	//	render::get().text(fonts[ESP], _x, _y, Color(moveflagcolor), HFONT_CENTERED_NONE, c_config::get()->flag_name);			    // EDITED!!!
	//	_y += 8;																			                        // EDITED!!!
	//}																						                        // EDITED!!!

	if (cfgsys->b["flags"])
	{
		auto scoped = e->m_bIsScoped();

		if (e == g_ctx.local())
			scoped = g_ctx.globals.scoped;

		if (scoped)
		{
			auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(65, 169, 205);
			color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

			render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "ZOOM");
			_y += 8;
		}
	}

	if (cfgsys->b["flags"])
	{
		auto animstate = e->get_animation_state();

		if (animstate)
		{
			auto fakeducking = [&]() -> bool
			{
				static auto stored_tick = 0;
				static int crouched_ticks[65];

				if (animstate->m_fDuckAmount) //-V550
				{
					if (animstate->m_fDuckAmount < 0.9f && animstate->m_fDuckAmount > 0.5f) //-V550
					{
						if (stored_tick != m_globals()->m_tickcount)
						{
							crouched_ticks[e->EntIndex()]++;
							stored_tick = m_globals()->m_tickcount;
						}

						return crouched_ticks[e->EntIndex()] > 16;
					}
					else
						crouched_ticks[e->EntIndex()] = 0;
				}

				return false;
			};

			if (fakeducking() && e->m_fFlags() & FL_ONGROUND && !animstate->m_bInHitGroundAnimation)
			{
				auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(215, 20, 20);
				color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

				render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "FD");
				_y += 8;
			}
		}
	}

	if (cfgsys->b["flags"] && weapon->is_non_aim())
	{
		auto color = e->IsDormant() ? Color(255, 255,255, 255) : Color(255, 255, 255);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "HIT");
		_y += 8;
	}

	if (cfgsys->b["flags"] && e->m_flFlashDuration() > 0)
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(72, 170, 209);
		color.SetAlpha(float(255 * e->m_flFlashDuration()) * esp_alpha_fade[e->EntIndex()]);
		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "BLIND");
		_y += 10;
	}


	if (cfgsys->b["flags"] && e->EntIndex() == g_ctx.globals.bomb_carrier)
	{
		auto color = e->IsDormant() ? Color(130, 130, 130, 130) : Color(255, 0, 0);
		color.SetAlpha(255.0f * esp_alpha_fade[e->EntIndex()]);

		render::get().text(fonts[ESP], _x, _y, color, HFONT_CENTERED_NONE, "B");
		_y += 8;
	}
}


void playeresp::draw_health_gradient(player_t* m_entity, const Box& box, const HPInfo& hpbox)
{
	if (!cfgsys->b["healbargrd"]) // &cfg->["healbargrd"]
		return;

	Color Healcolorgrd
	{
		cfgsys->c["healbarcollgrd"][0],
		cfgsys->c["healbarcollgrd"][1],
		cfgsys->c["healbarcollgrd"][2],
		cfgsys->c["hearbarcollgrd"][3]
	};
	auto alpha = (int)(255.0f * esp_alpha_fade[m_entity->EntIndex()]);

	auto text_color = m_entity->IsDormant() ? Color(130, 130, 130, alpha) : Color(255, 255, 255, alpha);
	auto back_color = Color(255, 255, 255, 200);
	auto back_color2 = Color(50, 50, 50, 40);
	auto nigga = Color(0, 0, 0);
	auto color = m_entity->IsDormant() ? Color(130, 130, 130) : Healcolorgrd;

	color.SetAlpha(alpha);
	render::get().rect(box.x - 6, box.y - 1, 4, box.h + 2, nigga);

	Box n_box =
	{
		box.x - 5,
		box.y,
		2,
		box.h
	};

	auto bar_height = (int)((float)hpbox.hp * (float)n_box.h / 100.0f);
	auto offset = n_box.h - bar_height;

	render::get().rect_filled(n_box.x, n_box.y - 1, 2, n_box.h + 2, back_color);
	render::get().rect_filled(n_box.x, n_box.y - 1, 2, n_box.h + 2, back_color2);
	render::get().rect_filled(n_box.x, n_box.y + offset, 2, bar_height, color);

	auto height = n_box.h - hpbox.hp * n_box.h / 100;

	if (hpbox.hp < 100)
		render::get().text(fonts[ESP], n_box.x + 1, n_box.y + offset, text_color, HFONT_CENTERED_X | HFONT_CENTERED_Y, std::to_string(hpbox.hp).c_str());
}

//void playeresp::draw_lines(player_t* e)
//{
//	if (!c_config::get()->b["los"])
//		return;
//
//	if (!g_ctx.local()->is_alive())
//		return;
//
//	static int width, height;
//	m_engine()->GetScreenSize(width, height);
//
//	Vector angle;
//
//	if (!math::world_to_screen(e->GetAbsOrigin(), angle))
//		return;
//
//	auto color = e->IsDormant() ? Color(130, 130, 130, 130) : g_cfg.player.type[type].snap_lines_color;
//	color.SetAlpha(min(255.0f * esp_alpha_fade[e->EntIndex()], color.a()));
//
//	Color loscolor{
//		cfgsys->c["loscoll"][0],
//		cfgsys->c["loscoll"][1],
//		cfgsys->c["loscoll"][2],
//		cfgsys->c["loscoll"][3]
//	};
//
//	render::get().line(width / 2, height, angle.x, angle.y, Color(loscolor));
//}

void playeresp::draw_multi_points(player_t* e)
{
	if (!cfgsys->b["rage_enabled"])
		return;

	if (!cfgsys->b["visaimbot"])
		return;

	if (!g_ctx.local()->is_alive()) //-V807
		return;

	if (g_ctx.local()->get_move_type() == MOVETYPE_NOCLIP)
		return;

	if (g_ctx.globals.current_weapon == -1)
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return;


	auto records = &player_records[e->EntIndex()]; //-V826

	if (records->empty())
		return;

	auto record = &records->front();

	if (!record->valid(false))
		return;

	std::vector <scan_point> points; //-V826
	auto hitboxes = aim::get().get_hitboxes(record);

	for (auto& hitbox : hitboxes)
	{
		auto current_points = aim::get().get_points(record, hitbox, false);

		for (auto& point : current_points)
			points.emplace_back(point);
	}

	for (auto& point : points)
	{
		if (points.empty())
			break;

		if (point.hitbox == HITBOX_HEAD)
			continue;

		for (auto it = points.begin(); it != points.end(); ++it)
		{
			if (point.point == it->point)
				continue;

			auto first_angle = math::calculate_angle(g_ctx.globals.eye_pos, point.point);
			auto second_angle = math::calculate_angle(g_ctx.globals.eye_pos, it->point);

			auto distance = g_ctx.globals.eye_pos.DistTo(point.point);
			auto fov = math::fast_sin(DEG2RAD(math::get_fov(first_angle, second_angle))) * distance;

			if (fov < 5.0f)
			{
				points.erase(it);
				break;
			}
		}
	}

	if (points.empty())
		return;

	Color mpcolor
	{
		cfgsys->c["visabcoll"][0],
		cfgsys->c["visabcoll"][1],
		cfgsys->c["visabcoll"][2],
		cfgsys->c["visabcoll"][3]
	};

	for (auto& point : points)
	{
		Vector screen;

		if (!math::world_to_screen(point.point, screen))
			continue;

		//render::get().Draw3DCircleGradient(position, alpha * 15.f, Color(mpcolor), 1.0f);
		render::get().circle_filled(screen.x - 1, screen.y - 1, 5, 3, Color(mpcolor));
	}
}