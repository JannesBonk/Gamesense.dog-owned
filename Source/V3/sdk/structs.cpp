// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <array>
#include "structs.hpp"
#include "..\cheats\misc\misc.h"
#include "..\cheats\misc\logs.h"
#include "..\cheats\lagcompensation\local_animations.h"
#include "..\cheats\lagcompensation\setup_bones.h"

datamap_t* entity_t::GetPredDescMap()
{
	typedef datamap_t* (__thiscall* Fn)(void*);
	return call_virtual< Fn >(this, g_ctx.indexes.at(16))(this);
}

bool player_t::CanSeePlayer(player_t* player, const Vector& pos)
{
	CGameTrace tr;
	Ray_t ray;
	CTraceFilter filter;
	filter.pSkip = this;

	ray.Init(get_shoot_position(), pos);
	m_trace()->TraceRay(ray, MASK_SHOT | CONTENTS_GRATE, &filter, &tr);

	return tr.hit_entity == player || tr.fraction > 0.97f;
}

bool entity_t::is_player()
{
	if (!this) //-V704
		return false;

	auto client_class = GetClientClass();

	if (!client_class)
		return false;

	return client_class->m_ClassID == CCSPlayer;
}

void entity_t::set_model_index(int index)
{
	if (!this) //-V704
		return;

	using Fn = void(__thiscall*)(PVOID, int);
	return call_virtual<Fn>(this, g_ctx.indexes.at(7))(this, index);
}

void entity_t::set_abs_angles(const Vector& angle)
{
	if (!this) //-V704
		return;

	using Fn = void(__thiscall*)(void*, const Vector&);
	static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8")));

	return fn(this, angle);
}

void entity_t::set_abs_origin(const Vector& origin)
{
	if (!this) //-V704
		return;

	using Fn = void(__thiscall*)(void*, const Vector&);
	static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8")));

	return fn(this, origin);
}

weapon_info_t* weapon_t::get_csweapon_info()
{
	if (!this) //-V704
		return nullptr;

	if (!m_weaponsys())
		return nullptr;

	return m_weaponsys()->GetWpnData(this->m_iItemDefinitionIndex());
}

float weapon_t::get_inaccuracy()
{
	if (!this)
		return 0.0f;

	return call_virtual<float(__thiscall*)(void*)>(this, g_ctx.indexes.at(9))(this);
}

float weapon_t::get_spread()
{
	if (!this)
		return 0.0f;

	return call_virtual<float(__thiscall*)(void*)>(this, g_ctx.indexes.at(10))(this);
}

void weapon_t::update_accuracy_penality()
{
	if (!this) //-V704
		return;

	call_virtual<void(__thiscall*)(void*)>(this, g_ctx.indexes.at(11))(this);
}


bool weapon_t::is_empty()
{
	if (!this) //-V704
		return true;

	return m_iClip1() <= 0;
}

bool weapon_t::can_fire(bool check_revolver)
{
	if (!this) //-V704
		return false;

	if (!is_non_aim() && is_empty())
		return false;

	auto owner = (player_t*)m_entitylist()->GetClientEntityFromHandle(m_hOwnerEntity());

	if (owner == g_ctx.local() && antiaim::get().freeze_check)
		return false;

	if (!owner->valid(false))
		return false;

	if (owner->m_bIsDefusing())
		return false;

	auto server_time = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);

	if (server_time < m_flNextPrimaryAttack())
		return false;

	if (server_time < owner->m_flNextAttack())
		return false;

	if (check_revolver && m_iItemDefinitionIndex() == WEAPON_REVOLVER && m_flPostponeFireReadyTime() >= server_time)
		return false;

	return true;
}

int weapon_t::get_weapon_group(bool rage)
{
	if (!this) //-V704
		return -1;

	if (!IsGun())
		return -1;

	if (rage)
	{
		if (m_iItemDefinitionIndex() == WEAPON_REVOLVER || m_iItemDefinitionIndex() == WEAPON_DEAGLE)
			return 0;
		else if (is_pistol())
			return 1;
		else if (is_smg())
			return 2;
		else if (is_rifle())
			return 3;
		else if (m_iItemDefinitionIndex() == WEAPON_SCAR20 || m_iItemDefinitionIndex() == WEAPON_G3SG1)
			return 4;
		else if (m_iItemDefinitionIndex() == WEAPON_SSG08)
			return 5;
		else if (m_iItemDefinitionIndex() == WEAPON_AWP)
			return 6;
		else if (is_shotgun())
			return 7;
	}
	else
	{
		if (m_iItemDefinitionIndex() == WEAPON_DEAGLE)
			return 0;
		if (is_pistol())
			return 1;
		else if (is_rifle())
			return 2;
		else if (is_smg())
			return 3;
		else if (is_sniper())
			return 4;
		else if (is_shotgun())
			return 5;
	}

	return -1;
}

bool weapon_t::IsGun()
{
	if (!this)
		return false;

	int id = this->m_iItemDefinitionIndex();

	if (!id)
		return false;

	if (id == WEAPON_KNIFE && id == WEAPON_HEGRENADE && id == WEAPON_DECOY && id == WEAPON_MOLOTOV && id == WEAPON_C4
		&& id == WEAPON_FLASHBANG && id == WEAPON_SMOKEGRENADE || id == WEAPON_KNIFE_T )
		return false;
	else
		return true;
}

bool weapon_t::is_rifle()
{
	if (!this) //-V704
		return false;

	int weapon_id = m_iItemDefinitionIndex();

	return weapon_id == WEAPON_AK47 || weapon_id == WEAPON_M4A1 || weapon_id == WEAPON_M4A1_SILENCER || weapon_id == WEAPON_GALILAR ||
		weapon_id == WEAPON_FAMAS || weapon_id == WEAPON_AUG || weapon_id == WEAPON_SG556;
}

bool weapon_t::is_smg()
{
	if (!this) //-V704
		return false;

	int weapon_id = m_iItemDefinitionIndex();

	return weapon_id == WEAPON_MAC10 || weapon_id == WEAPON_MP7 || weapon_id == WEAPON_MP9 || weapon_id == WEAPON_P90 ||
		weapon_id == WEAPON_BIZON || weapon_id == WEAPON_UMP45 || weapon_id == WEAPON_MP5SD;
}

bool weapon_t::is_shotgun()
{
	if (!this) //-V704
		return false;

	int weapon_id = m_iItemDefinitionIndex();

	return weapon_id == WEAPON_XM1014 || weapon_id == WEAPON_NOVA || weapon_id == WEAPON_SAWEDOFF || weapon_id == WEAPON_MAG7 || weapon_id == WEAPON_M249 || weapon_id == WEAPON_NEGEV;
}

bool weapon_t::is_pistol()
{
	if (!this) //-V704
		return false;

	int weapon_id = m_iItemDefinitionIndex();

	return weapon_id == WEAPON_DEAGLE || weapon_id == WEAPON_ELITE || weapon_id == WEAPON_FIVESEVEN || weapon_id == WEAPON_P250 ||
		weapon_id == WEAPON_GLOCK || weapon_id == WEAPON_HKP2000 || weapon_id == WEAPON_CZ75A || weapon_id == WEAPON_USP_SILENCER || weapon_id == WEAPON_TEC9 || weapon_id == WEAPON_REVOLVER;
}

bool weapon_t::is_sniper()
{
	if (!this) //-V704
		return false;

	int weapon_id = m_iItemDefinitionIndex();

	return weapon_id == WEAPON_AWP || weapon_id == WEAPON_SCAR20 || weapon_id == WEAPON_G3SG1 || weapon_id == WEAPON_SSG08;
}

bool weapon_t::is_grenade()
{
	if (!this) //-V704
		return false;

	int idx = m_iItemDefinitionIndex();

	return idx == WEAPON_FLASHBANG || idx == WEAPON_HEGRENADE || idx == WEAPON_SMOKEGRENADE || idx == WEAPON_MOLOTOV || idx == WEAPON_DECOY || idx == WEAPON_INCGRENADE;
}

bool weapon_t::is_knife()
{
	if (!this) //-V704
		return false;

	int idx = m_iItemDefinitionIndex();

	return idx == WEAPON_KNIFE || idx == WEAPON_BAYONET || idx == WEAPON_KNIFE_BUTTERFLY || idx == WEAPON_KNIFE_FALCHION
		|| idx == WEAPON_KNIFE_FLIP || idx == WEAPON_KNIFE_GUT || idx == WEAPON_KNIFE_KARAMBIT || idx == WEAPON_KNIFE_M9_BAYONET
		|| idx == WEAPON_KNIFE_PUSH || idx == WEAPON_KNIFE_SURVIVAL_BOWIE || idx == WEAPON_KNIFE_T || idx == WEAPON_KNIFE_TACTICAL
		|| idx == WEAPON_KNIFEGG || idx == WEAPON_KNIFE_GHOST || idx == WEAPON_KNIFE_GYPSY_JACKKNIFE || idx == WEAPON_KNIFE_STILETTO
		|| idx == WEAPON_KNIFE_URSUS || idx == WEAPON_KNIFE_WIDOWMAKER || idx == WEAPON_KNIFE_CSS || idx == WEAPON_KNIFE_CANIS
		|| idx == WEAPON_KNIFE_CORD || idx == WEAPON_KNIFE_OUTDOOR || idx == WEAPON_KNIFE_SKELETON;
}

bool weapon_t::is_non_aim()
{
	if (!this) //-V704
		return true;

	auto idx = m_iItemDefinitionIndex();

	if (idx == WEAPON_C4 || idx == WEAPON_HEALTHSHOT)
		return true;

	if (is_knife())
		return true;

	if (is_grenade())
		return true;

	return false;
}

bool weapon_t::can_double_tap()
{
	if (!this) //-V704
		return false;

	if (is_non_aim())
		return false;

	auto idx = m_iItemDefinitionIndex();

	if (idx == WEAPON_TASER || idx == WEAPON_REVOLVER || idx == WEAPON_SSG08 || idx == WEAPON_AWP || idx == WEAPON_XM1014 || idx == WEAPON_NOVA || idx == WEAPON_SAWEDOFF || idx == WEAPON_MAG7)
		return false;

	return true;
}

int weapon_t::get_max_tickbase_shift()
{
	if (!can_double_tap())
		return m_gamerules()->m_bIsValveDS() ? 1 : 13;

	auto idx = m_iItemDefinitionIndex();
	auto max_tickbase_shift = 0;

	switch (idx)
	{
	case WEAPON_M249:
	case WEAPON_MAC10:
	case WEAPON_P90:
	case WEAPON_MP5SD:
	case WEAPON_NEGEV:
	case WEAPON_MP9:
		max_tickbase_shift = 13;
		break;
	case WEAPON_ELITE:
	case WEAPON_UMP45:
	case WEAPON_BIZON:
	case WEAPON_TEC9:
	case WEAPON_MP7:
		max_tickbase_shift = 13;
		break;
	case WEAPON_AK47:
	case WEAPON_AUG:
	case WEAPON_FAMAS:
	case WEAPON_GALILAR:
	case WEAPON_M4A1:
	case WEAPON_M4A1_SILENCER:
	case WEAPON_CZ75A:
		max_tickbase_shift = 13;
		break;
	case WEAPON_FIVESEVEN:
		max_tickbase_shift = 13;
		break;
	case WEAPON_GLOCK:
		max_tickbase_shift = 13;
		break;
	case WEAPON_P250:
		max_tickbase_shift = 13;
		break;
	case WEAPON_SG556:
		max_tickbase_shift = 13;
		break;
	case WEAPON_HKP2000:
	case WEAPON_USP_SILENCER:
		max_tickbase_shift = 13;
		break;
	case WEAPON_DEAGLE:
		max_tickbase_shift = 13;
		break;
	case WEAPON_G3SG1:
	case WEAPON_SCAR20:
		max_tickbase_shift = 16;
		break;
	}

	if (m_gamerules()->m_bIsValveDS())
		max_tickbase_shift = min(max_tickbase_shift, 6);

	return max_tickbase_shift;
}

char* weapon_t::get_icon()
{
	if (!this) //-V704
		return " ";

	switch (m_iItemDefinitionIndex())
	{
	case WEAPON_BAYONET:
		return "1";
	case WEAPON_KNIFE_SURVIVAL_BOWIE:
		return "7";
	case WEAPON_KNIFE_BUTTERFLY:
		return "8";
	case WEAPON_KNIFE_FALCHION:
		return "0";
	case WEAPON_KNIFE_FLIP:
		return "2";
	case WEAPON_KNIFE_GUT:
		return "3";
	case WEAPON_KNIFE_KARAMBIT:
		return "4";
	case WEAPON_KNIFE_M9_BAYONET:
		return "5";
	case WEAPON_KNIFE_TACTICAL:
		return "6";
	case WEAPON_KNIFE_PUSH:
		return "]";
	case WEAPON_DEAGLE:
		return "A";
	case WEAPON_ELITE:
		return "B";
	case WEAPON_FIVESEVEN:
		return "C";
	case WEAPON_GLOCK:
		return "D";
	case WEAPON_HKP2000:
		return "E";
	case WEAPON_P250:
		return "F";
	case WEAPON_USP_SILENCER:
		return "G";
	case WEAPON_TEC9:
		return "H";
	case WEAPON_REVOLVER:
		return "J";
	case WEAPON_MAC10:
		return "K";
	case WEAPON_UMP45:
		return "L";
	case WEAPON_BIZON:
		return "M";
	case WEAPON_MP7:
		return "N";
	case WEAPON_MP9:
		return "O";
	case WEAPON_P90:
		return "P";
	case WEAPON_GALILAR:
		return "Q";
	case WEAPON_FAMAS:
		return "R";
	case WEAPON_M4A1_SILENCER:
		return "T";
	case WEAPON_M4A1:
		return "S";
	case WEAPON_AUG:
		return "U";
	case WEAPON_SG556:
		return "V";
	case WEAPON_AK47:
		return "W";
	case WEAPON_G3SG1:
		return "X";
	case WEAPON_SCAR20:
		return "Y";
	case WEAPON_AWP:
		return "Z";
	case WEAPON_SSG08:
		return "a";
	case WEAPON_XM1014:
		return "b";
	case WEAPON_SAWEDOFF:
		return "c";
	case WEAPON_MAG7:
		return "d";
	case WEAPON_NOVA:
		return "e";
	case WEAPON_NEGEV:
		return "f";
	case WEAPON_M249:
		return "g";
	case WEAPON_TASER:
		return "h";
	case WEAPON_FLASHBANG:
		return "i";
	case WEAPON_HEGRENADE:
		return "j";
	case WEAPON_SMOKEGRENADE:
		return "k";
	case WEAPON_MOLOTOV:
		return "l";
	case WEAPON_DECOY:
		return "m";
	case WEAPON_INCGRENADE:
		return "n";
	case WEAPON_C4:
		return "o";
	case WEAPON_CZ75A:
		return "I";
	default:
		return " ";
	}
}

std::string weapon_t::get_name()
{
	if (!this) //-V704
		return " ";

	switch (m_iItemDefinitionIndex())
	{
	case WEAPON_KNIFE:
		return "KNIFE";
	case WEAPON_KNIFE_T:
		return "KNIFE";
	case WEAPON_BAYONET:
		return "KNIFE";
	case WEAPON_KNIFE_SURVIVAL_BOWIE:
		return "KNIFE";
	case WEAPON_KNIFE_BUTTERFLY:
		return "KNIFE";
	case WEAPON_KNIFE_FALCHION:
		return "KNIFE";
	case WEAPON_KNIFE_FLIP:
		return "KNIFE";
	case WEAPON_KNIFE_GUT:
		return "KNIFE";
	case WEAPON_KNIFE_KARAMBIT:
		return "KNIFE";
	case WEAPON_KNIFE_M9_BAYONET:
		return "KNIFE";
	case WEAPON_KNIFE_TACTICAL:
		return "KNIFE";
	case WEAPON_KNIFE_PUSH:
		return "KNIFE";
	case WEAPON_DEAGLE:
		return "DEAGLE";
	case WEAPON_ELITE:
		return "DUAL BERETTAS";
	case WEAPON_FIVESEVEN:
		return "FIVE-SEVEN";
	case WEAPON_GLOCK:
		return "GLOCK 18";
	case WEAPON_HKP2000:
		return "P2000";
	case WEAPON_P250:
		return "P250";
	case WEAPON_USP_SILENCER:
		return "USP-S";
	case WEAPON_TEC9:
		return "TEC-9";
	case WEAPON_REVOLVER:
		return "REVOLVER";
	case WEAPON_MAC10:
		return "MAC-10";
	case WEAPON_UMP45:
		return "UMP-45";
	case WEAPON_BIZON:
		return "PP-BIZON";
	case WEAPON_MP7:
		return "MP7";
	case WEAPON_MP9:
		return "MP9";
	case WEAPON_P90:
		return "P90";
	case WEAPON_GALILAR:
		return "GALIL AR";
	case WEAPON_FAMAS:
		return "FAMAS";
	case WEAPON_M4A1_SILENCER:
		return "M4A1-S";
	case WEAPON_M4A1:
		return "M4A4";
	case WEAPON_AUG:
		return "AUG";
	case WEAPON_SG556:
		return "SG 553";
	case WEAPON_AK47:
		return "AK-47";
	case WEAPON_G3SG1:
		return "G3SG1";
	case WEAPON_SCAR20:
		return "SCAR-20";
	case WEAPON_AWP:
		return "AWP";
	case WEAPON_SSG08:
		return "SSG 08";
	case WEAPON_XM1014:
		return "XM1014";
	case WEAPON_SAWEDOFF:
		return "SAWED-OFF";
	case WEAPON_MAG7:
		return "MAG-7";
	case WEAPON_NOVA:
		return "NOVA";
	case WEAPON_NEGEV:
		return "NEGEV";
	case WEAPON_M249:
		return "M249";
	case WEAPON_TASER:
		return "ZEUS X27";
	case WEAPON_FLASHBANG:
		return "FLASHBANG";
	case WEAPON_HEGRENADE:
		return "HE GRENADE";
	case WEAPON_SMOKEGRENADE:
		return "SMOKE";
	case WEAPON_MOLOTOV:
		return "MOLOTOV";
	case WEAPON_DECOY:
		return "DECOY";
	case WEAPON_INCGRENADE:
		return "INCENDIARY";
	case WEAPON_C4:
		return "C4";
	case WEAPON_CZ75A:
		return "CZ75-AUTO";
	default:
		return " ";
	}
}

std::array <float, 24>& entity_t::m_flPoseParameter()
{
	static auto _m_flPoseParameter = netvars::get().get_offset(crypt_str("CCSPlayer"), crypt_str("m_flPoseParameter"));
	return *(std::array <float, 24>*)((uintptr_t)this + _m_flPoseParameter);
}

Vector player_t::get_shoot_position()
{
	if (!this) //-V704
		return ZERO;

	auto shoot_position = m_vecOrigin() + m_vecViewOffset();

	if (this != g_ctx.local()) //-V648
		return shoot_position;

	modify_eye_position(shoot_position);
	return shoot_position;
}

void player_t::modify_eye_position(Vector& eye_position)
{
	if (!this)
		return;

	if (!local_animations::get().local_data.prediction_animstate)
		return;

	if (!local_animations::get().local_data.prediction_animstate->m_bInHitGroundAnimation && local_animations::get().local_data.prediction_animstate->m_fDuckAmount <= 0.0f)
		return;

	local_animations::get().local_data.prediction_animstate->m_pBaseEntity = this;

	static auto lookup_bone = reinterpret_cast <int(__thiscall*)(void*, const char*)> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 53 56 8B F1 57 83 BE ?? ?? ?? ?? ?? 75 14")));
	auto head_bone = lookup_bone(local_animations::get().local_data.prediction_animstate->m_pBaseEntity, crypt_str("head_0"));

	if (head_bone == -1)
		return;

	auto head_position = Vector(g_ctx.globals.prediction_matrix[head_bone][0][3], g_ctx.globals.prediction_matrix[head_bone][1][3], g_ctx.globals.prediction_matrix[head_bone][2][3] + 1.7f);

	if (head_position.z >= eye_position.z)
		return;

	eye_position.z = math::lerp2(math::simple_spline_remap_val_clamped(fabs(eye_position.z - head_position.z), 4.0f, 10.0f, 0.0f, 1.0f), eye_position.z, head_position.z);
}

bool player_t::is_alive()
{
	if (!this) //-V704
		return false;

	if (m_iTeamNum() != 2 && m_iTeamNum() != 3)
		return false;

	if (m_lifeState() != LIFE_ALIVE)
		return false;

	return true;
}

int	player_t::get_move_type()
{
	if (!this) //-V704
		return 0;

	return *(int*)((uintptr_t)this + 0x25C);
}

int player_t::get_hitbox_bone_id(int hitbox_id)
{
	if (!this) //-V704
		return -1;

	auto hdr = m_modelinfo()->GetStudioModel(GetModel());

	if (!hdr)
		return -1;

	auto hitbox_set = hdr->pHitboxSet(m_nHitboxSet());

	if (!hitbox_set)
		return -1;

	auto hitbox = hitbox_set->pHitbox(hitbox_id);

	if (!hitbox)
		return -1;

	return hitbox->bone;
}

Vector player_t::hitbox_position(int hitbox_id)
{
	if (!this) //-V704
		return ZERO;

	auto hdr = m_modelinfo()->GetStudioModel(GetModel());

	if (!hdr)
		return ZERO;

	auto hitbox_set = hdr->pHitboxSet(m_nHitboxSet());

	if (!hitbox_set)
		return ZERO;

	auto hitbox = hitbox_set->pHitbox(hitbox_id);

	if (!hitbox)
		return ZERO;

	Vector min, max;

	math::vector_transform(hitbox->bbmin, m_CachedBoneData().Base()[hitbox->bone], min);
	math::vector_transform(hitbox->bbmax, m_CachedBoneData().Base()[hitbox->bone], max);

	return (min + max) * 0.5f;
}

bool player_t::is_on_ground()
{
	return this->m_hGroundEntity().IsValid() && this->m_hGroundEntity().Get();
}

Vector player_t::hitbox_position_matrix(int hitbox_id, matrix3x4_t matrix[MAXSTUDIOBONES])
{
	if (!this) //-V704
		return ZERO;

	auto hdr = m_modelinfo()->GetStudioModel(GetModel());

	if (!hdr)
		return ZERO;

	auto hitbox_set = hdr->pHitboxSet(m_nHitboxSet());

	if (!hitbox_set)
		return ZERO;

	auto hitbox = hitbox_set->pHitbox(hitbox_id);

	if (!hitbox)
		return ZERO;

	Vector min, max;

	math::vector_transform(hitbox->bbmin, matrix[hitbox->bone], min);
	math::vector_transform(hitbox->bbmax, matrix[hitbox->bone], max);

	return (min + max) * 0.5f;
}

CUtlVector <matrix3x4_t>& player_t::m_CachedBoneData()
{
	return *(CUtlVector <matrix3x4_t>*)(uintptr_t(this) + 0x2914);
}

CBoneAccessor& player_t::m_BoneAccessor()
{
	static auto m_nForceBone = netvars::get().get_offset(crypt_str("CBaseAnimating"), crypt_str("m_nForceBone"));
	static auto BoneAccessor = m_nForceBone + 0x1C;

	return *(CBoneAccessor*)((uintptr_t)this + BoneAccessor);
}

void player_t::invalidate_bone_cache()
{
	if (!this) //-V704
		return;

	m_flLastBoneSetupTime() = -FLT_MAX;
	m_iMostRecentModelBoneCounter() = UINT_MAX;
}

void player_t::set_abs_velocity(const Vector& velocity)
{
	if (!this) //-V704
		return;

	using Fn = void(__thiscall*)(void*, const Vector&);
	static auto fn = reinterpret_cast<Fn>(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 0C 53 56 57 8B 7D 08 8B F1 F3")));

	return fn(this, velocity);
}

Vector player_t::get_render_angles()
{
	if (!this) //-V704
		return ZERO;

	return *(Vector*)((uintptr_t)this + 0x31D8);
}

void player_t::set_render_angles(const Vector& angles)
{
	if (!this) //-V704
		return;

	*(Vector*)((uintptr_t)this + 0x31D8) = angles;
}

void player_t::update_clientside_animation()
{
	if (!this) //-V704
		return;

	auto animstate = get_animation_state();

	if (!animstate)
		return;

	if (animstate->m_iLastClientSideAnimationUpdateFramecount >= m_globals()->m_framecount) //-V614
		animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount - 1;

	using Fn = void(__thiscall*)(void*);
	call_virtual<Fn>(this, g_ctx.indexes.at(13))(this);
}

uint32_t& player_t::m_iMostRecentModelBoneCounter()
{
	static auto invalidate_bone_cache = util::FindSignature(crypt_str("client.dll"), crypt_str("80 3D ?? ?? ?? ?? ?? 74 16 A1 ?? ?? ?? ?? 48 C7 81"));
	static auto most_recent_model_bone_counter = *(uintptr_t*)(invalidate_bone_cache + 0x1B);

	return *(uint32_t*)((uintptr_t)this + most_recent_model_bone_counter);
}

float& player_t::m_flLastBoneSetupTime()
{
	static auto invalidate_bone_cache = util::FindSignature(crypt_str("client.dll"), crypt_str("80 3D ?? ?? ?? ?? ?? 74 16 A1 ?? ?? ?? ?? 48 C7 81"));
	static auto last_bone_setup_time = *(uintptr_t*)(invalidate_bone_cache + 0x11);

	return *(float*)((uintptr_t)this + last_bone_setup_time);
}

void player_t::select_item(const char* string, int sub_type)
{
	static auto select_item_fn = reinterpret_cast <void(__thiscall*)(void*, const char*, int)> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 56 8B F1 ? ? ? 85 C9 74 71 8B 06")));
	select_item_fn(this, string, sub_type);
}

bool player_t::using_standard_weapons_in_vechile()
{
	static auto using_standard_weapons_in_vechile_fn = reinterpret_cast <bool(__thiscall*)(void*)> (util::FindSignature(crypt_str("client.dll"), crypt_str("56 57 8B F9 8B 97 ? ? ? ? 83 FA FF 74 41")));
	return using_standard_weapons_in_vechile_fn(this);
}

bool player_t::physics_run_think(int index)
{
	static auto physics_run_think_fn = reinterpret_cast <bool(__thiscall*)(void*, int)> (util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 EC 10 53 56 57 8B F9 8B 87 ? ? ? ? C1 E8 16")));
	return physics_run_think_fn(this, index);
}

VarMapping_t* player_t::var_mapping()
{
	return reinterpret_cast<VarMapping_t*>((DWORD)this + 0x24);
}

CUserCmd*& player_t::current_command()
{
	static auto current_command = *reinterpret_cast<uint32_t*> (util::FindSignature(crypt_str("client.dll"), crypt_str("89 BE ? ? ? ? E8 ? ? ? ? 85 FF")) + 2);
	return *reinterpret_cast<CUserCmd**> (reinterpret_cast<uintptr_t> (this) + current_command);
}

bool player_t::valid(bool check_team, bool check_dormant)
{
	if (!this) //-V704
		return false;

	if (!g_ctx.local())
		return false;

	if (!is_player())
		return false;

	if (!is_alive())
		return false;

	if (IsDormant() && check_dormant)
		return false;

	if (check_team && g_ctx.local()->m_iTeamNum() == m_iTeamNum())
		return false;

	return true;
}

int player_t::animlayer_count()
{
	if (!this) //-V704
		return 0;

	return *(int*)((DWORD)this + 0x299C);
}

AnimationLayer* player_t::get_animlayers()
{
	return *(AnimationLayer**)((DWORD)this + 0x2990);
}

int player_t::sequence_activity(int sequence)
{
	if (!this) //-V704
		return 0;

	auto hdr = m_modelinfo()->GetStudioModel(this->GetModel());

	if (!hdr)
		return 0;

	static auto sequence_activity = reinterpret_cast<int(__fastcall*)(void*, studiohdr_t*, int)>(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 53 8B 5D 08 56 8B F1 83")));
	return sequence_activity(this, hdr, sequence);
}

c_baseplayeranimationstate* player_t::get_animation_state()
{
	return *reinterpret_cast<c_baseplayeranimationstate**>(reinterpret_cast<void*>(uintptr_t(this) + 0x9960));
}

float player_t::GetMaxPlayerSpeed()
{
	weapon_t* pWeapon = this->m_hActiveWeapon().Get();

	if (pWeapon)
	{
		weapon_info_t* pWeaponData = pWeapon->GetWeaponData();

		if (pWeaponData)
			return this->m_bIsScoped() ? pWeaponData->flMaxPlayerSpeedAlt : pWeaponData->flMaxPlayerSpeed;
	}

	return 260.0f;
}

CStudioHdr* player_t::m_pStudioHdr()
{
	static auto studio_hdr = util::FindSignature(crypt_str("client.dll"), crypt_str("8B B7 ?? ?? ?? ?? 89 74 24 20"));
	return *(CStudioHdr**)((uintptr_t)this + *(uintptr_t*)(studio_hdr + 0x2) + 0x4);
}

uint32_t& player_t::m_iOcclusionFlags()
{
	return *(uint32_t*)((uintptr_t)this + 0xA28);
}

uint32_t& player_t::m_iOcclusionFramecount()
{
	return *(uint32_t*)((uintptr_t)this + 0xA30);
}


bool player_t::setup_bones_rebuild(matrix3x4_t* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	if (!this)
		return false;
	bool record;
	void* pRenderable = reinterpret_cast<void*>(uintptr_t(this) + 0x4);

	if (!pRenderable)
		return false;

	std::array < AnimationLayer, 13 > aAnimationLayers;

	const auto backup_bone_array = m_BoneAccessor().GetBoneArrayForWrite();
	float flCurTime = m_globals()->m_curtime;
	float flRealTime = m_globals()->m_realtime;
	float flFrameTime = m_globals()->m_frametime;
	float flAbsFrameTime = m_globals()->m_absoluteframetime;
	int iFrameCount = m_globals()->m_framecount;
	int iTickCount = m_globals()->m_tickcount;
	float flInterpolation = m_globals()->m_interpolation_amount;

	m_globals()->m_curtime = m_flSimulationTime();
	m_globals()->m_realtime = m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;
	m_globals()->m_absoluteframetime = m_globals()->m_intervalpertick;
	m_globals()->m_framecount = TIME_TO_TICKS(m_flSimulationTime());
	m_globals()->m_tickcount = TIME_TO_TICKS(m_flSimulationTime());
	m_globals()->m_interpolation_amount = 0.0f;

	auto iOcclusionFramecount = m_iOcclusionFramecount();
	auto iOcclusionFlags = m_iOcclusionFlags();
	auto nLastSkipFramecount = m_nLastSkipFramecount();
	auto pInverseKinematics = m_pInverseKinematics();
	auto bMaintainSequenceTransition = m_bMaintainSequenceTransition();
	auto vecAbsOrigin = GetAbsOrigin();

	std::memcpy(aAnimationLayers.data(), get_animlayers(), sizeof(AnimationLayer) * 13);

	invalidate_bone_cache();
	m_BoneAccessor().m_ReadableBones = m_BoneAccessor().m_WritableBones = 0;

	if (get_animation_state())
		get_animation_state()->m_pLastBoneSetupWeapon = get_animation_state()->m_pActiveWeapon;

	g_ctx.globals.setuping_bones = true;

	m_iOcclusionFramecount() = 0;
	m_iOcclusionFlags() = 0;
	m_nLastSkipFramecount() = 0;

	if (this != g_ctx.local())
		set_abs_origin(m_vecOrigin());

	m_fEffects() |= 8;
	m_pInverseKinematics() = nullptr;
	m_bMaintainSequenceTransition() = false;

	get_animlayers()[12].m_flWeight = 0.0f;

	if (this == g_ctx.local())
	{
		if (sequence_activity(get_animlayers()[3].m_nSequence) == 979)
		{
			get_animlayers()[3].m_flCycle = 0.0f;
			get_animlayers()[3].m_flWeight = 0.0f;
		}
	}

	typedef bool(__thiscall* Fn)(void*, matrix3x4_t*, int, int, float);

	if (record)
		m_BoneAccessor().SetBoneArrayForWrite(pBoneToWorldOut);

	if (this == g_ctx.local())
		g_ctx.globals.updating_animation = true;

	auto res = call_virtual<Fn>(pRenderable, 13)(pRenderable, record ? nullptr : pBoneToWorldOut, nMaxBones, boneMask, currentTime);

	if (this == g_ctx.local())
		g_ctx.globals.updating_animation = false;

	if (record)
		m_BoneAccessor().SetBoneArrayForWrite(backup_bone_array);

	m_bMaintainSequenceTransition() = false;
	m_pInverseKinematics() = pInverseKinematics;
	m_nLastSkipFramecount() = nLastSkipFramecount;
	m_iOcclusionFlags() = iOcclusionFlags;
	m_iOcclusionFramecount() = iOcclusionFramecount;

	g_ctx.globals.setuping_bones = false;

	if (this != g_ctx.local())
		set_abs_origin(vecAbsOrigin);

	std::memcpy(get_animlayers(), aAnimationLayers.data(), sizeof(AnimationLayer) * 13);

	m_globals()->m_curtime = flCurTime;
	m_globals()->m_realtime = flRealTime;
	m_globals()->m_frametime = flFrameTime;
	m_globals()->m_absoluteframetime = flAbsFrameTime;
	m_globals()->m_framecount = iFrameCount;
	m_globals()->m_tickcount = iTickCount;
	m_globals()->m_interpolation_amount = flInterpolation;

	return res;
}

bool player_t::setup_bones_rebuilt(matrix3x4_t* matrix, int mask)
{
	if (!this) //-V704
		return false;

	auto setuped = false;

	auto backup_value = *(uint8_t*)((uintptr_t)this + 0x274);
	*(uint8_t*)((uintptr_t)this + 0x274) = 0;

	auto backup_effects = m_fEffects();
	m_fEffects() |= 8;

	auto animstate = get_animation_state();
	auto previous_weapon = animstate ? animstate->m_pLastBoneSetupWeapon : nullptr;

	if (previous_weapon)
		animstate->m_pLastBoneSetupWeapon = animstate->m_pActiveWeapon;

	auto backup_abs_origin = GetAbsOrigin();

	if (this != g_ctx.local())
		set_abs_origin(m_vecOrigin());

	g_ctx.globals.setuping_bones = true;
	invalidate_bone_cache();

	SetupBones(matrix, matrix ? MAXSTUDIOBONES : -1, mask, m_flSimulationTime());

	g_ctx.globals.setuping_bones = false;

	if (this != g_ctx.local())
		set_abs_origin(backup_abs_origin);

	if (previous_weapon)
		animstate->m_pLastBoneSetupWeapon = previous_weapon;

	m_fEffects() = backup_effects;
	*(uint8_t*)((uintptr_t)this + 0x274) = backup_value;

	return setuped;
}

uint32_t& player_t::m_fEffects()
{
	static auto m_fEffects = util::find_in_datamap(GetPredDescMap(), crypt_str("m_fEffects"));
	return *(uint32_t*)(uintptr_t(this) + m_fEffects);
}

uint32_t& player_t::m_iEFlags()
{
	static auto m_iEFlags = util::find_in_datamap(GetPredDescMap(), crypt_str("m_iEFlags"));
	return *(uint32_t*)(uintptr_t(this) + m_iEFlags);
}

float& player_t::m_surfaceFriction()
{
	static auto m_surfaceFriction = util::find_in_datamap(GetPredDescMap(), crypt_str("m_surfaceFriction"));
	return *(float*)(uintptr_t(this) + m_surfaceFriction);
}

Vector& player_t::m_vecAbsVelocity()
{
	if (!this) //-V704
		return ZERO;

	static auto m_vecAbsVelocity = util::find_in_datamap(GetPredDescMap(), crypt_str("m_vecAbsVelocity"));
	return *(Vector*)(uintptr_t(this) + m_vecAbsVelocity);
}

float player_t::get_max_desync_delta()
{
	if (!this) //-V704
		return 0.0f;

	auto animstate = get_animation_state();

	if (!animstate)
		return 0.0f;

	auto speedfactor = math::clamp(animstate->m_flFeetSpeedForwardsOrSideWays, 0.0f, 1.0f);
	auto avg_speedfactor = (animstate->m_flStopToFullRunningFraction * -0.3f - 0.2f) * speedfactor + 1.0f;

	auto duck_amount = animstate->m_fDuckAmount;

	if (duck_amount) //-V550
	{
		auto max_velocity = math::clamp(animstate->m_flFeetSpeedUnknownForwardOrSideways, 0.0f, 1.0f);
		auto duck_speed = duck_amount * max_velocity;

		avg_speedfactor += duck_speed * (0.5f - avg_speedfactor);
	}

	return animstate->yaw_desync_adjustment() * avg_speedfactor;
}

void player_t::invalidate_physics_recursive(int change_flags)
{
	static auto m_uInvalidatePhysics = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56 83 E0 04"));
	reinterpret_cast <void(__thiscall*)(void*, int)> (m_uInvalidatePhysics)(this, change_flags);
}

float& viewmodel_t::m_flCycle()
{
	static auto m_flCycle = util::find_in_datamap(GetPredDescMap(), crypt_str("m_flCycle"));
	return *(float*)(uintptr_t(this) + m_flCycle);
}

float& viewmodel_t::m_flAnimTime()
{
	static auto m_flAnimTime = util::find_in_datamap(GetPredDescMap(), crypt_str("m_flAnimTime"));
	return *(float*)(uintptr_t(this) + m_flAnimTime);
}

void viewmodel_t::SendViewModelMatchingSequence(int sequence)
{
	using Fn = void(__thiscall*)(void*, int);
	call_virtual <Fn>(this, g_ctx.indexes.at(14))(this, sequence);
}

void CHudChat::chat_print(const char* fmt, ...)
{
	char msg[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, 1024, fmt, args);
	call_virtual <void(__cdecl*)(void*, int, int, const char*, ...)>(this, g_ctx.indexes.at(15))(this, 0, 0, fmt);
	va_end(args);
}