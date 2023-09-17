#include "animation_system.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"

std::deque <adjust_data> player_records[65];

enum ADVANCED_ACTIVITY : int
{
	ACTIVITY_NONE = 0,
	ACTIVITY_JUMP,
	ACTIVITY_LAND
};

void lagcompensation::fsn(ClientFrameStage_t stage)
{
	if (stage != FRAME_NET_UPDATE_END)
		return;

	if (!c_config::get()->b["rage_enabled"])
		return;

	for (auto i = 1; i < m_globals()->m_maxclients; i++) //-V807
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (e == g_ctx.local())
			continue;

		if (!valid(i, e))
			continue;

		auto time_delta = abs(TIME_TO_TICKS(e->m_flSimulationTime()) - m_globals()->m_tickcount);

		if (time_delta > 1.0f / m_globals()->m_intervalpertick)
			continue;

		auto update = player_records[i].empty() || e->m_flSimulationTime() != e->m_flOldSimulationTime(); //-V550

		if (update) //-V550
		{
			if (!player_records[i].empty() && (e->m_vecOrigin() - player_records[i].front().origin).LengthSqr() > 4096.0f)
				for (auto& record : player_records[i])
					record.invalid = true;

			player_records[i].emplace_front(adjust_data());
			update_player_animations(e);

			while (player_records[i].size() > 24)
				player_records[i].pop_back();
		}
	}
}

bool lagcompensation::valid(int i, player_t* e)
{
	if (!c_config::get()->b["rage_enabled"] || !e->valid(false))
	{
		if (!e->is_alive())
		{
			is_dormant[i] = false;
			player_resolver[i].reset();

			g_ctx.globals.fired_shots[i] = 0;
			g_ctx.globals.missed_shots[i] = 0;
		}
		else if (e->IsDormant())
			is_dormant[i] = true;

		player_records[i].clear();
		return false;
	}

	return true;
}
template < class T >
static T interpolate(const T& current, const T& target, const int progress, const int maximum)
{
	return current + ((target - current) / maximum) * progress;
}
void lagcompensation::update_player_animations(player_t* e)
{
	auto animstate = e->get_animation_state();

	if (!animstate)
		return;

	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(e->EntIndex(), &player_info))
		return;

	auto records = &player_records[e->EntIndex()]; //-V826

	if (records->empty())
		return;

	adjust_data* previous_record = nullptr;

	if (records->size() >= 2 && !records->at(1).invalid)
		previous_record = &records->at(1);

	auto record = &records->front();

	AnimationLayer animlayers[15];

	memcpy(animlayers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));
	memcpy(record->layers, animlayers, e->animlayer_count() * sizeof(AnimationLayer));

	auto backup_lower_body_yaw_target = e->m_flLowerBodyYawTarget();
	auto backup_duck_amount = e->m_flDuckAmount();
	auto backup_flags = e->m_fFlags();
	auto backup_eflags = e->m_iEFlags();

	auto backup_curtime = m_globals()->m_curtime; //-V807
	auto backup_frametime = m_globals()->m_frametime;
	auto backup_realtime = m_globals()->m_realtime;
	auto backup_framecount = m_globals()->m_framecount;
	auto backup_tickcount = m_globals()->m_tickcount;
	auto backup_interpolation_amount = m_globals()->m_interpolation_amount;

	m_globals()->m_curtime = e->m_flSimulationTime();
	m_globals()->m_frametime = m_globals()->m_intervalpertick;

	bool bIsLeftDormancy = is_dormant[e->EntIndex()];
	if (bIsLeftDormancy)
	{
		is_dormant[e->EntIndex()] = false;
		float flLastUpdateTime = e->m_flSimulationTime() - m_globals()->m_intervalpertick;

		if (e->m_fFlags() & FL_ONGROUND) {
			e->get_animation_state()->m_bInHitGroundAnimation = false;
			e->get_animation_state()->m_bOnGround = true;

			float flLandTime = 0.f;
			if (e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flCycle > 0.f && e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flPlaybackRate > 0.f) {
				int iLandActivity = e->sequence_activity(e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_nSequence);

				if (iLandActivity == ACT_CSGO_LAND_LIGHT || iLandActivity == ACT_CSGO_LAND_HEAVY) {
					flLandTime = e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flCycle / e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flPlaybackRate;

					if (flLandTime > 0.f)
						flLastUpdateTime = e->m_flSimulationTime() - flLandTime;
				}
			}

			e->m_vecVelocity().z = 0.f;
		}
		else {
			float flJumpTime = 0.f;
			if (e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flCycle > 0.f && e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate > 0.f) {
				int iJumpActivity = e->sequence_activity(e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_nSequence);

				if (iJumpActivity == ACT_CSGO_JUMP) {
					flJumpTime = e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flCycle / e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate;

					if (flJumpTime > 0.f)
						flLastUpdateTime = e->m_flSimulationTime() - flJumpTime;
				}
			}

			e->get_animation_state()->m_bOnGround = false;
			e->get_animation_state()->time_since_in_air() = flJumpTime - m_globals()->m_intervalpertick;
		}

		float flWeight = e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight;
		if (e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_MOVE].m_flPlaybackRate < 0.00001f)
			e->m_vecVelocity() = ZERO;
		else if (flWeight > 0.f && flWeight < 0.95f) {
			float flMaxSpeed = 260.f;
			// get player weapon.
			auto m_weapon = e->m_hActiveWeapon().Get();

			if (m_weapon)
			{
				// get wpninfo.
				auto m_weapon_info = m_weapon->get_csweapon_info();

				// get the max possible speed whilest player are still accurate.
				if (m_weapon_info)
					flMaxSpeed = (e->m_bIsScoped() ? m_weapon_info->flMaxPlayerSpeedAlt : m_weapon_info->flMaxPlayerSpeed);
			}

			float flPostVelocityLength = e->m_vecVelocity().Length();

			if (flPostVelocityLength > 0.f) {
				float flMaxSpeedMultiply = 1.f;

				// FL_DUCKING & FL_AIMDUCKING - fully ducked.
				// !FL_DUCKING & !FL_AIMDUCKING - fully unducked.
				// FL_DUCKING & !FL_AIMDUCKING - previously fully ducked, unducking in progress.
				// !FL_DUCKING & FL_AIMDUCKING - previously fully unducked, ducking in progress.
				if (e->m_fFlags() & (FL_DUCKING | FL_AIMDUCKING))
					flMaxSpeedMultiply = 0.34f;
				else if (e->m_bIsWalking())
					flMaxSpeedMultiply = 0.52f;

				e->m_vecVelocity().x = (e->m_vecVelocity().x / flPostVelocityLength) * (flWeight * (flMaxSpeed * flMaxSpeedMultiply));
				e->m_vecVelocity().y = (e->m_vecVelocity().y / flPostVelocityLength) * (flWeight * (flMaxSpeed * flMaxSpeedMultiply));
			}
		}

		e->get_animation_state()->m_flLastClientSideAnimationUpdateTime = flLastUpdateTime;
	}

	if (previous_record)
	{
		auto velocity = e->m_vecVelocity();
		auto was_in_air = e->m_fFlags() & FL_ONGROUND && previous_record->flags & FL_ONGROUND;

		auto time_difference = max(m_globals()->m_intervalpertick, e->m_flSimulationTime() - previous_record->simulation_time);
		auto origin_delta = e->m_vecOrigin() - previous_record->origin;

		auto animation_speed = 0.0f;

		if (!origin_delta.IsZero() && TIME_TO_TICKS(time_difference) > 0)
		{
			e->m_vecVelocity() = origin_delta * (1.0f / time_difference);

			if (e->m_fFlags() & FL_ONGROUND && animlayers[11].m_flWeight > 0.0f && animlayers[11].m_flWeight < 1.0f && animlayers[11].m_flCycle > previous_record->layers[11].m_flCycle)
			{
				auto weapon = e->m_hActiveWeapon().Get();

				if (weapon)
				{
					auto max_speed = 260.0f;
					auto weapon_info = e->m_hActiveWeapon().Get()->get_csweapon_info();

					if (weapon_info)
						max_speed = e->m_bIsScoped() ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed;

					auto modifier = 0.35f * (1.0f - animlayers[11].m_flWeight);

					if (modifier > 0.0f && modifier < 1.0f)
						animation_speed = max_speed * (modifier + 0.55f);
				}
			}

			if (animation_speed > 0.0f)
			{
				animation_speed /= e->m_vecVelocity().Length2D();

				e->m_vecVelocity().x *= animation_speed;
				e->m_vecVelocity().y *= animation_speed;
			}

			if (records->size() >= 3 && time_difference > m_globals()->m_intervalpertick)
			{
				auto previous_velocity = (previous_record->origin - records->at(2).origin) * (1.0f / time_difference);

				if (!previous_velocity.IsZero() && !was_in_air)
				{
					auto current_direction = math::normalize_yaw(RAD2DEG(atan2(e->m_vecVelocity().y, e->m_vecVelocity().x)));
					auto previous_direction = math::normalize_yaw(RAD2DEG(atan2(previous_velocity.y, previous_velocity.x)));

					auto average_direction = current_direction - previous_direction;
					average_direction = DEG2RAD(math::normalize_yaw(current_direction + average_direction * 0.5f));

					auto direction_cos = cos(average_direction);
					auto dirrection_sin = sin(average_direction);

					auto velocity_speed = e->m_vecVelocity().Length2D();

					e->m_vecVelocity().x = direction_cos * velocity_speed;
					e->m_vecVelocity().y = dirrection_sin * velocity_speed;
				}
			}


			if (!(e->m_fFlags() & FL_ONGROUND))
			{
				static auto sv_gravity = m_cvar()->FindVar("sv_gravity");

				auto fixed_timing = math::clamp(time_difference, m_globals()->m_intervalpertick, 1.0f);
				e->m_vecVelocity().z -= sv_gravity->GetFloat() * fixed_timing * 0.5f;
			}
			else
				e->m_vecVelocity().z = 0.0f;
		}
	}

	if (e->m_fFlags() & FL_ONGROUND)
	{
		if (animlayers[6].m_flPlaybackRate == 0.f)
		{
			e->m_vecVelocity().Zero();
		}
		else
		{
			const auto avg_speed = e->m_vecVelocity().Length2D();
			if (avg_speed != 0.f)
			{
				const auto weight = animlayers[11].m_flWeight;

				const auto speed_as_portion = 0.58f - (weight - 1.f) * 0.38f;

				const auto avg_speed_modifier = speed_as_portion * e->GetMaxPlayerSpeed();

				if (weight >= 1.f && avg_speed > avg_speed_modifier || weight < 1.f && (avg_speed_modifier > avg_speed || weight > 0.f))
				{
					e->m_vecVelocity().x /= avg_speed;
					e->m_vecVelocity().y /= avg_speed;

					e->m_vecVelocity().x *= avg_speed_modifier;
					e->m_vecVelocity().y *= avg_speed_modifier;
				}
			}
		}
	}

	e->m_iEFlags() &= ~0x1800;

	if (animlayers[6].m_flWeight == 0.0f || animlayers[6].m_flPlaybackRate == 0.0f)
		e->m_vecVelocity().Zero();

	e->m_iEFlags() &= ~0x1000;

	auto updated_animations = false;

	c_baseplayeranimationstate state;
	memcpy(&state, animstate, sizeof(c_baseplayeranimationstate));

	if (previous_record)
	{
		auto ticks_chocked = 1;

		memcpy(e->get_animlayers(), previous_record->layers, e->animlayer_count() * sizeof(AnimationLayer));

		auto simulation_ticks = TIME_TO_TICKS(e->m_flSimulationTime() - previous_record->simulation_time);

		if (simulation_ticks > 0 && simulation_ticks < 17)
			ticks_chocked = simulation_ticks;

		if (ticks_chocked > 1)
		{
			int iActivityTick = 0;
			int iActivityType = 0;

			if (animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight > 0.f && previous_record->layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight <= 0.f) {
				int iLandSequence = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_nSequence;

				if (iLandSequence > 2) {
					int iLandActivity = e->sequence_activity(iLandSequence);

					if (iLandActivity == ACT_CSGO_LAND_LIGHT || iLandActivity == ACT_CSGO_LAND_HEAVY) {
						float flCurrentCycle = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flCycle;
						float flCurrentRate = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flPlaybackRate;

						if (flCurrentCycle > 0.f && flCurrentRate > 0.f) {
							float flLandTime = (flCurrentCycle / flCurrentRate);

							if (flLandTime > 0.f) {
								iActivityTick = TIME_TO_TICKS(e->m_flSimulationTime() - flLandTime) + 1;
								iActivityType = ACTIVITY_LAND;
							}
						}
					}
				}
			}

			if (animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flCycle > 0.f && animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate > 0.f) {
				int iJumpSequence = animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_nSequence;

				if (iJumpSequence > 2) {
					int iJumpActivity = e->sequence_activity(iJumpSequence);

					if (iJumpActivity == ACT_CSGO_JUMP) {
						float flCurrentCycle = animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flCycle;
						float flCurrentRate = animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate;

						if (flCurrentCycle > 0.f && flCurrentRate > 0.f) {
							float flJumpTime = (flCurrentCycle / flCurrentRate);

							if (flJumpTime > 0.f) {
								iActivityTick = TIME_TO_TICKS(e->m_flSimulationTime() - flJumpTime) + 1;
								iActivityType = ACTIVITY_JUMP;
							}
						}
					}
				}
			}

			for (auto i = 0; i < ticks_chocked; ++i)
			{
				auto simulated_time = previous_record->simulation_time + TICKS_TO_TIME(i);

				// lerp duck amt.
				e->m_flDuckAmount() = interpolate(previous_record->duck_amount, e->m_flDuckAmount(), i, ticks_chocked);

				// lerp velocity.
				e->m_vecVelocity() = interpolate(previous_record->velocity, e->m_vecVelocity(), i, ticks_chocked);
				e->m_vecAbsVelocity() = e->m_vecVelocity();//e->m_vecAbsVelocity() = interpolate(previous_record->velocity, e->m_vecVelocity(), i, ticks_chocked);
				e->set_abs_origin(e->m_vecOrigin());


				int iCurrentSimulationTick = TIME_TO_TICKS(simulated_time);

				if (iActivityType > ACTIVITY_NONE) {
					bool bIsOnGround = e->m_fFlags() & FL_ONGROUND;

					if (iActivityType == ACTIVITY_JUMP) {
						if (iCurrentSimulationTick == iActivityTick - 1)
							bIsOnGround = true;
						else if (iCurrentSimulationTick == iActivityTick)
						{
							// reset animation layer.
							e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flCycle = 0.f;
							e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_nSequence = animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_nSequence;
							e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate = animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flPlaybackRate;

							// reset player ground state.
							bIsOnGround = false;
						}

					}
					else if (iActivityType == ACTIVITY_LAND) {
						if (iCurrentSimulationTick == iActivityTick - 1)
							bIsOnGround = false;
						else if (iCurrentSimulationTick == iActivityTick)
						{
							// reset animation layer.
							e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flCycle = 0.f;
							e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_nSequence = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_nSequence;
							e->get_animlayers()[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flPlaybackRate = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flPlaybackRate;

							// reset player ground state.
							bIsOnGround = true;
						}
					}

					if (bIsOnGround)
						e->m_fFlags() |= FL_ONGROUND;
					else
						e->m_fFlags() &= ~FL_ONGROUND;
				}

				auto simulated_ticks = TIME_TO_TICKS(simulated_time);

				m_globals()->m_realtime = simulated_time;
				m_globals()->m_curtime = simulated_time;
				m_globals()->m_framecount = simulated_ticks;
				m_globals()->m_tickcount = simulated_ticks;
				m_globals()->m_interpolation_amount = 0.0f;

				g_ctx.globals.updating_animation = true;
				if (animstate->m_iLastClientSideAnimationUpdateFramecount >= m_globals()->m_framecount) //-V614
					animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount - 1;
				e->m_bClientSideAnimation() = true;
				e->update_clientside_animation();
				e->m_bClientSideAnimation() = false;
				g_ctx.globals.updating_animation = false;

				m_globals()->m_realtime = backup_realtime;
				m_globals()->m_curtime = backup_curtime;
				m_globals()->m_framecount = backup_framecount;
				m_globals()->m_tickcount = backup_tickcount;
				m_globals()->m_interpolation_amount = backup_interpolation_amount;

				updated_animations = true;
			}
		}
	}

	if (!updated_animations)
	{
		e->m_vecAbsVelocity() = e->m_vecVelocity();
		g_ctx.globals.updating_animation = true;
		if (animstate->m_iLastClientSideAnimationUpdateFramecount >= m_globals()->m_framecount) //-V614
			animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount - 1;
		e->m_bClientSideAnimation() = true;
		e->update_clientside_animation();
		e->m_bClientSideAnimation() = false;
		g_ctx.globals.updating_animation = false;
	}

	memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

	auto setup_matrix = [&](player_t* e, AnimationLayer* layers, const int& matrix) -> void
	{
		e->invalidate_physics_recursive(8);

		AnimationLayer backup_layers[15];
		memcpy(backup_layers, e->get_animlayers(), e->animlayer_count() * sizeof(AnimationLayer));

		memcpy(e->get_animlayers(), layers, e->animlayer_count() * sizeof(AnimationLayer));

		switch (matrix)
		{
		case MAIN:
			e->setup_bones_fixed(record->matrixes_data.main, BONE_USED_BY_ANYTHING);
			break;
		case FIRST:
			e->setup_bones_fixed(record->matrixes_data.first, BONE_USED_BY_HITBOX);
			break;
		case NONE:
			e->setup_bones_fixed(record->matrixes_data.zero, BONE_USED_BY_HITBOX);
			break;
		case SECOND:
			e->setup_bones_fixed(record->matrixes_data.second, BONE_USED_BY_HITBOX);
			break;
		}

		memcpy(e->get_animlayers(), backup_layers, e->animlayer_count() * sizeof(AnimationLayer));
	};

	if (!player_info.fakeplayer && g_ctx.local()->is_alive() && e->m_iTeamNum() != g_ctx.local()->m_iTeamNum()) //-V807
	{
		animstate->m_flGoalFeetYaw = previous_goal_feet_yaw[e->EntIndex()];

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		previous_goal_feet_yaw[e->EntIndex()] = animstate->m_flGoalFeetYaw;
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, NONE);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		//		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + 60.0f);

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, FIRST);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		//		animstate->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - 60.0f);

		g_ctx.globals.updating_animation = true;
		e->update_clientside_animation();
		g_ctx.globals.updating_animation = false;

		setup_matrix(e, animlayers, SECOND);
		memcpy(animstate, &state, sizeof(c_baseplayeranimationstate));

		player_resolver[e->EntIndex()].initialize(e, record, state.m_flGoalFeetYaw, e->m_angEyeAngles().x);
		player_resolver[e->EntIndex()].resolve_yaw();

		switch (record->side)
		{
		case RESOLVER_ORIGINAL:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
			break;
		case RESOLVER_ZERO:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y);
			break;
		case RESOLVER_FIRST:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + e->get_max_desync_delta());
			break;
		case RESOLVER_SECOND:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - e->get_max_desync_delta());
			break;
		case RESOLVER_LOW_FIRST:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y + min(e->get_max_desync_delta(), 35.0f));
			break;
		case RESOLVER_LOW_SECOND:
			e->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(e->m_angEyeAngles().y - min(e->get_max_desync_delta(), 35.0f));
			break;
		}

		e->m_angEyeAngles().x = player_resolver[e->EntIndex()].resolve_pitch();
	}

	g_ctx.globals.updating_animation = true;
	e->m_bClientSideAnimation() = true;
	e->update_clientside_animation();
	e->m_bClientSideAnimation() = false;
	g_ctx.globals.updating_animation = false;

	setup_matrix(e, animlayers, MAIN);
	memcpy(e->m_CachedBoneData().Base(), record->matrixes_data.main, e->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

	m_globals()->m_curtime = backup_curtime;
	m_globals()->m_frametime = backup_frametime;

	e->m_flLowerBodyYawTarget() = backup_lower_body_yaw_target;
	e->m_flDuckAmount() = backup_duck_amount;
	e->m_fFlags() = backup_flags;
	e->m_iEFlags() = backup_eflags;

	memcpy(e->get_animlayers(), animlayers, e->animlayer_count() * sizeof(AnimationLayer));


	e->invalidate_physics_recursive(8);
	e->invalidate_bone_cache();

	record->store_data(e, true);
	if (e->m_flSimulationTime() < e->m_flOldSimulationTime())
		record->invalid = true;
}