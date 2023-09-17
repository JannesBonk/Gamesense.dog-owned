// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "animation_system.h"
#include "..\ragebot\aim.h"

std::string resolver_method[65];

float resolver::get_angle(player_t* player) {
	return math::NormalizedAngle(player->m_angEyeAngles().y);
}

float resolver::get_backward_yaw(player_t* player) {
	return math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y;
}

float resolver::get_forward_yaw(player_t* player) {
	return math::NormalizedAngle(get_backward_yaw(player) - 180.f);
}

bool CanSeeHitbox(player_t* entity, int HITBOX) {
	return g_ctx.local()->CanSeePlayer(entity, entity->hitbox_position(HITBOX));
}

void resolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
	player = e;
	player_record = record;

	original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
	original_pitch = math::normalize_pitch(pitch);
}

void resolver::reset()
{
	player = nullptr;
	player_record = nullptr;

	safe_matrix_shot = false;
	freestand_side = false;

	was_first_bruteforce = false;
	was_second_bruteforce = false;

	was_roll_first_bruteforce = false;
	was_roll_second_bruteforce = false;

	original_goal_feet_yaw = 0.0f;
	original_pitch = 0.0f;
}

bool resolver::desync_detect()
{
	if (!player->is_alive())
		return false;
	if (~player->m_fFlags() & FL_ONGROUND)
		return false;
	if (~player->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
		return false;
	if (!player->m_hActiveWeapon().Get()->can_fire(true))
		return false;
	if (player->get_move_type() == MOVETYPE_NOCLIP
		|| player->get_move_type() == MOVETYPE_LADDER)
		return false;

	return true;
}

bool resolver::Saw(player_t* entity)
{
	if (!(CanSeeHitbox(entity, HITBOX_HEAD) && CanSeeHitbox(entity, HITBOX_LEFT_FOOT) && CanSeeHitbox(entity, HITBOX_RIGHT_FOOT))
		|| (CanSeeHitbox(entity, HITBOX_HEAD) && CanSeeHitbox(entity, HITBOX_LEFT_FOOT) && CanSeeHitbox(entity, HITBOX_RIGHT_FOOT)))
		return false;

	return true;
}

resolver_side resolver::TraceSide(player_t* entity)
{
	auto first_visible = autowall::get().can_hit_floating_point(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first));
	auto second_visible = autowall::get().can_hit_floating_point(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second));
	auto main_visible = autowall::get().can_hit_floating_point(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.zero));

	if (main_visible)
	{
		if (second_visible)
			return RESOLVER_SECOND;
		else if (first_visible)
			return  RESOLVER_FIRST;
	}
	else
	{
		if (first_visible)
			return  RESOLVER_FIRST;
		else if (second_visible)
			return  RESOLVER_SECOND;
	}

	return RESOLVER_ORIGINAL;
}

bool resolver::does_have_jitter(player_t* player, int* new_side)
{
	static float LastAngle[64];
	static int LastBrute[64];
	static bool Switch[64];
	static float LastUpdateTime[64];

	if (!math::IsNearEqual(player->m_angEyeAngles().y, LastAngle[player->EntIndex()], 58.f))
	{
		Switch[player->EntIndex()] = !Switch[player->EntIndex()];
		LastAngle[player->EntIndex()] = player->m_angEyeAngles().y;
		*new_side = Switch[player->EntIndex()] ? 3 : -3;
		LastBrute[player->EntIndex()] = *new_side;
		LastUpdateTime[player->EntIndex()] = m_globals()->m_curtime;
		return true;
	}
	else
	{
		if (fabsf(LastUpdateTime[player->EntIndex()] - m_globals()->m_curtime >= TICKS_TO_TIME(17)) || player->m_flSimulationTime() != player->m_flOldSimulationTime())
			LastAngle[player->EntIndex()] = player->m_angEyeAngles().y;

		*new_side = LastBrute[player->EntIndex()];
	}

	return false;
}

void resolver::antifreestand()
{
	if (!g_ctx.local()->is_alive())
		return;

	if (!g_ctx.globals.weapon->IsGun())
		return;


	for (int i = 0; i < m_engine()->GetMaxClients(); ++i)
	{
		auto player = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!player || !player->is_alive() || player->IsDormant()
			|| !player->is_player() || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() || g_ctx.globals.missed_shots[player->EntIndex()] > 0 || player->EntIndex() == m_engine()->GetLocalPlayer())
			continue;

		float angToLocal = math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y;
		Vector ViewPoint = g_ctx.local()->m_vecOrigin() + Vector(0, 0, 90);

		Vector2D Side1 = { (45 * sin(DEG2RAD(angToLocal))),(45 * cos(DEG2RAD(angToLocal))) };
		Vector2D Side2 = { (45 * sin(DEG2RAD(angToLocal + 180))) ,(45 * cos(DEG2RAD(angToLocal + 180))) };

		Vector2D Side3 = { (60 * sin(DEG2RAD(angToLocal))),(60 * cos(DEG2RAD(angToLocal))) };
		Vector2D Side4 = { (60 * sin(DEG2RAD(angToLocal + 180))) ,(60 * cos(DEG2RAD(angToLocal + 180))) };

		Vector Origin = player->m_vecOrigin();

		Vector2D OriginLeftRight[] = { Vector2D(Side1.x, Side1.y), Vector2D(Side2.x, Side2.y) };
		Vector2D OriginLeftRightLocal[] = { Vector2D(Side3.x, Side3.y), Vector2D(Side4.x, Side4.y) };

		// zZz...
		for (int side = 0; side < 2; side++)
		{
			Vector OriginAutowall = { Origin.x + OriginLeftRight[side].x,  Origin.y - OriginLeftRight[side].y , Origin.z + 90 };
			Vector ViewPointAutowall = { ViewPoint.x + OriginLeftRightLocal[side].x,  ViewPoint.y - OriginLeftRightLocal[side].y , ViewPoint.z };

			if (autowall::get().can_hit_floating_point(OriginAutowall, ViewPoint))
			{
				if (side == 0)
				{
					freestand_side = true;
				}
				else if (side == 1)
				{
					freestand_side = false;
				}
			}
			else
			{
				for (int sidealternative = 0; sidealternative < 2; sidealternative++)
				{
					Vector ViewPointAutowallalternative = { Origin.x + OriginLeftRight[sidealternative].x,  Origin.y - OriginLeftRight[sidealternative].y , Origin.z + 90 };

					if (autowall::get().can_hit_floating_point(ViewPointAutowallalternative, ViewPointAutowall))
					{
						if (sidealternative == 0)
						{
							freestand_side = true;
						}
						else if (sidealternative == 1)
						{
							freestand_side = false;
						}
					}
				}
			}
		}

		//if (!DetectSide(player, freestand_side))  //
			//return;
	}
}

void resolver::resolve_yaw()
{
	player_info_t player_info;

	auto animstate = player->get_animation_state();

	if (!m_engine()->GetPlayerInfo(player->EntIndex(), &player_info))
		return;

	if (player_info.fakeplayer || !g_ctx.local()->is_alive() || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() || player->get_move_type() == MOVETYPE_LADDER || player->get_move_type() == MOVETYPE_NOCLIP)
	{
		player_record->side = RESOLVER_ORIGINAL;
		return;
	}

	safe_matrix_shot = animstate->m_velocity > 180.0f;

	if (!animstate || !desync_detect() || !shot_resolver())
	{
		player_record->side = RESOLVER_ORIGINAL;
		return;
	}

	resolver_history res_history = HISTORY_UNKNOWN;

	for (auto it = lagcompensation::get().player_sets.begin(); it != lagcompensation::get().player_sets.end(); ++it)
	{
		if (it->id == player_info.steamID64)
		{
			res_history = it->res_type;
			is_player_faking = it->faking;
			break;
		}
	}

	if (res_history == HISTORY_ZERO)
		is_player_zero = true;

	resolver_type type = resolver_type(-1);

	auto delta = math::normalize_yaw(player->m_angEyeAngles().y - original_goal_feet_yaw);
	bool forward = fabsf(math::normalize_yaw(get_angle(player) - get_forward_yaw(player))) < 90.f;

	bool low_delta = false;

	auto first_side = low_delta ? RESOLVER_LOW_FIRST : RESOLVER_FIRST;
	auto second_side = low_delta ? RESOLVER_LOW_SECOND : RESOLVER_SECOND;

	auto jitter_first_side = low_delta ? RESOLVER_JITTER_FIRST : RESOLVER_FIRST;
	auto jitter_second_side = low_delta ? RESOLVER_JITTER_SECOND : RESOLVER_SECOND;

	int new_side = player_record->side = RESOLVER_ORIGINAL;

	if (player->m_vecVelocity().Length2D() > player->GetMaxPlayerSpeed() || !player->m_fFlags() && FL_ONGROUND)
	{
		if (g_ctx.globals.missed_shots[player->EntIndex()] == 0)
			player_record->side = freestand_side ? second_side : first_side;
		else if (Saw(player))
			if (TraceSide(player) != RESOLVER_ORIGINAL)
				player_record->side = TraceSide(player);

		if (does_have_jitter(player, &new_side) && player->m_angEyeAngles().x < 45.0f)
		{
			switch (g_ctx.globals.missed_shots[player->EntIndex()] % 2)
			{
			case 0:
				resolver_method[player->EntIndex()] = "Detect Jitter 1";
				animstate->m_flGoalFeetYaw = math::normalize_yaw(get_angle(player) +
					freestand_side ? jitter_second_side : jitter_first_side * new_side);
				break;
			case 1:
				resolver_method[player->EntIndex()] = "Detect Jitter 2";
				animstate->m_flGoalFeetYaw = math::normalize_yaw(get_angle(player) -
					freestand_side ? jitter_first_side : jitter_second_side * new_side);
				break;
			}
		}
	}
	else
		player_record->side = RESOLVER_ORIGINAL;

	if (player->m_vecVelocity().Length2D() > 50)
		records.move_lby[player->EntIndex()] = animstate->m_flGoalFeetYaw;

	auto valid_lby = true;
	bool move_anim = false;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.f)
		valid_lby = animstate->m_flTimeSinceStartedMoving < 0.22f;

	if (int(player_record->layers[3].m_flWeight == 0.f && player_record->layers[3].m_flCycle == 0.f && player_record->layers[6].m_flWeight == 0.f))
		move_anim = true;

	bool ducking = animstate->m_fDuckAmount && player->m_fFlags() & FL_ONGROUND && !animstate->m_bInHitGroundAnimation;

	if (valid_lby && move_anim || player->m_vecVelocity().Length() >= player->GetMaxPlayerSpeed() && ducking || player->m_vecVelocity().Length() >= player->GetMaxPlayerSpeed() && !ducking)
	{
		auto m_delta = math::AngleDiff(player->m_angEyeAngles().y, resolver_goal_feet_yaw[0]);
		player_record->side = m_delta <= 0.0f ? first_side : second_side;
	}
	else
	{
		if (fabs(delta) > 35.0f && valid_lby)
		{
			if (player->m_fFlags() & FL_ONGROUND || player->m_vecVelocity().Length2D() <= 2.0f)
			{
				if (player->sequence_activity(player_record->layers[3].m_nSequence) == 979)
					if (g_ctx.globals.missed_shots[player->EntIndex()])
						delta = -delta;
					else
						player_record->side = delta > 0 ? first_side : second_side;
			}
		}
		else if (!valid_lby && !(int(player_record->layers[12].m_flWeight * 1000.f)) && static_cast<int>(player_record->layers[6].m_flCycle * 1000.f) == static_cast<int>(previous_layers[6].m_flWeight * 1000.f))
		{
			float delta_third = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[0][6].m_flPlaybackRate);
			float delta_second = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[1][6].m_flPlaybackRate);
			float delta_first = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[2][6].m_flPlaybackRate);

			if (delta_first < delta_second || delta_third <= delta_second || (delta_second * 1000.f))
			{
				if (delta_first >= delta_third && delta_second > delta_third && !(delta_third * 1000.f))
				{
					player_record->type = ANIMATION;
					player_record->side = first_side;
				}
				else
				{
					if (forward)
					{
						player_record->type = DIRECTIONAL;
						player_record->side = freestand_side ? second_side : first_side;
					}
					else
					{
						player_record->type = DIRECTIONAL;
						player_record->side = freestand_side ? first_side : second_side;
					}
				}
			}
			else
			{
				player_record->type = ANIMATION;
				player_record->side = second_side;
			}
		}
		else
		{
			if (m_globals()->m_curtime - lock_side > 2.0f)
			{
				auto first_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player, g_ctx.local());
				auto second_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player, g_ctx.local());

				if (first_visible != second_visible)
				{
					freestand_side = true;
					side = second_visible;
					lock_side = m_globals()->m_curtime;
				}
				else
				{
					auto first_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first));
					auto second_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second));

					if (fabs(first_position - second_position) > 1.0f)
						freestand_side = first_position > second_position;
				}
			}
			else
				freestand_side = true;

			if (forward)
			{
				player_record->type = DIRECTIONAL;
				player_record->side = freestand_side ? second_side : first_side;
			}
			else
			{
				player_record->type = DIRECTIONAL;
				player_record->side = freestand_side ? first_side : second_side;
			}
		}
	}

	if (g_ctx.globals.missed_shots[player->EntIndex()] && aim::get().last_target[player->EntIndex()].record.type != LBY && aim::get().last_target[player->EntIndex()].record.type != JITTER)
	{
		switch (last_side)
		{
		case RESOLVER_ORIGINAL:
			g_ctx.globals.missed_shots[player->EntIndex()] = 0;
			fake = true;
			break;
		case RESOLVER_ZERO:
			player_record->type = BRUTEFORCE;

			if (!is_player_zero)
				player_record->side = /*freestand_side ? second_side : */ first_side;
			else
				player_record->side = /*freestand_side ? first_side : */ second_side;

			was_first_bruteforce = false; was_roll_first_bruteforce = false;
			was_second_bruteforce = false; was_roll_second_bruteforce = false;
			return;
		case RESOLVER_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = was_second_bruteforce ? RESOLVER_ZERO : RESOLVER_SECOND;

			was_first_bruteforce = true;
			return;
		case RESOLVER_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = was_first_bruteforce ? RESOLVER_ZERO : RESOLVER_FIRST;

			was_second_bruteforce = true;
			return;
		case RESOLVER_LOW_FIRST:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_LOW_SECOND;
			return;
		case RESOLVER_LOW_SECOND:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_FIRST;
			return;
		case RESOLVER_DEFAULT:
			player_record->type = BRUTEFORCE;
			player_record->side = RESOLVER_LOW_FIRST;
			return;
		}
	}

	player_record->type = BRUTEFORCE;
	player_record->side = RESOLVER_DEFAULT;

	// ghetto way
	auto choked = TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime());

	// if his pitch down, or he is choking or we already hitted in desync, we can say that he might use desync
	if (fabs(original_pitch) > 85.f || choked >= 1 || is_player_faking)
		fake = true;
	else if (!fake && !g_ctx.globals.missed_shots[player->EntIndex()])
	{
		player_record->type = ORIGINAL;
		return;
	}

	if (side)
	{
		player_record->type = type;
		player_record->side = freestand_side ? second_side : first_side;
	}
	else
	{
		player_record->type = type;
		player_record->side = freestand_side ? first_side : second_side;
	}

	if (is_player_zero && !g_ctx.globals.missed_shots[player->EntIndex()])
		player_record->side = RESOLVER_ZERO;

	if (!player->IsDormant()) {
		if (side >= -1)
			animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - 58;
		else
			animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + 58;

		for (; animstate->m_flGoalFeetYaw > 180.0f; animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - 360);
		for (; animstate->m_flGoalFeetYaw < -180.0f; animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + 360);
	}
	else {
		if (side <= 1)
			animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + 58;
		else
			animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - 58;

		for (; animstate->m_flGoalFeetYaw > 180.0f; animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw + 360);
		for (; animstate->m_flGoalFeetYaw < -180.0f; animstate->m_flGoalFeetYaw = animstate->m_flGoalFeetYaw - 360);
	}

	if ((player_record->player) && player_record->player->m_vecVelocity().Length2D() <= 0.15)
		return;
	else
		return;
	player_record->layers[3].m_flCycle == 0.f && player_record->layers[3].m_flWeight == 0.f;

	if (player) {
		freestand_side = player_record->player->m_flLowerBodyYawTarget();
	}
}

bool resolver::shot_resolver()
{
	bool m_shot;
	float m_flLastShotTime;

	m_flLastShotTime = player->m_hActiveWeapon() ? player->m_hActiveWeapon()->m_fLastShotTime() : 0.f;
	m_shot = m_flLastShotTime > player->m_flOldSimulationTime() && m_flLastShotTime <= player->m_flSimulationTime();

	if (m_flLastShotTime <= player->m_flSimulationTime() && m_shot)
		player_record->side = RESOLVER_ORIGINAL;

	return true;
}

float resolver::resolve_pitch()
{
	return original_pitch;
}