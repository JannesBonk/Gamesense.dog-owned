#include "animation_system.h"
#include "..\ragebot\aim.h"

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

	side = false;
	fake = false;

	was_first_bruteforce = false;
	was_second_bruteforce = false;

	original_goal_feet_yaw = 0.0f;
	original_pitch = 0.0f;
}

bool resolver::IsAdjustingBalance()
{


	for (int i = 0; i < 13; i++)
	{
		const int activity = player->sequence_activity(player_record->layers[i].m_nSequence);
		if (activity == 979)
		{
			return true;
		}
	}
	return false;
}

bool resolver::is_breaking_lby(AnimationLayer cur_layer, AnimationLayer prev_layer)
{
	if (IsAdjustingBalance())
	{
		if (IsAdjustingBalance())
		{
			if ((prev_layer.m_flCycle != cur_layer.m_flCycle) || cur_layer.m_flWeight == 1.f)
			{
				return true;
			}
			else if (cur_layer.m_flWeight == 0.f && (prev_layer.m_flCycle > 0.92f && cur_layer.m_flCycle > 0.92f))
			{
				return true;
			}
		}
		return false;
	}

	return false;
}

bool resolver::is_slow_walking()
{
	auto entity = player;
	//float large = 0;
	float velocity_2D[64], old_velocity_2D[64];
	if (entity->m_vecVelocity().Length2D() != velocity_2D[entity->EntIndex()] && entity->m_vecVelocity().Length2D() != NULL) {
		old_velocity_2D[entity->EntIndex()] = velocity_2D[entity->EntIndex()];
		velocity_2D[entity->EntIndex()] = entity->m_vecVelocity().Length2D();
	}
	//if (large == 0)return false;
	Vector velocity = entity->m_vecVelocity();
	Vector direction = entity->m_angEyeAngles();

	float speed = velocity.Length();
	direction.y = entity->m_angEyeAngles().y - direction.y;
	//method 1
	if (velocity_2D[entity->EntIndex()] > 1) {
		int tick_counter[64];
		if (velocity_2D[entity->EntIndex()] == old_velocity_2D[entity->EntIndex()])
			tick_counter[entity->EntIndex()] += 1;
		else
			tick_counter[entity->EntIndex()] = 0;

		while (tick_counter[entity->EntIndex()] > (1 / m_globals()->m_intervalpertick) * fabsf(0.1f))//should give use 100ms in ticks if their speed stays the same for that long they are definetely up to something..
			return true;
	}


	return false;
}

void resolver::NemsisResolver()
{
	player_record->type = LAYERS;


	const auto& cur_layer6_weight = player_record->layers[6].m_flWeight;
	if (cur_layer6_weight <= 0.0099999998)
	{
		if (std::abs(math::AngleDiff(player_record->angles.y, prev_record->angles.y)) > 35.f)
			detect_side();

		return;
	}
	bool m_accelerating;
	const auto& cur_layer11_weight = player_record->layers[11].m_flWeight;
	const auto cur_layer12_weight = static_cast<int>(player_record->layers[12].m_flWeight * 1000.f);

	const auto cur_layer11_weight_valid = cur_layer11_weight > 0.f && cur_layer11_weight < 1.f;
	const auto prev_layer11_weight_valid = prev_record->layers[11].m_flWeight > 0.f && prev_record->layers[11].m_flWeight < 1.f;

	const auto prev_layer6_weight = static_cast<int>(prev_record->layers[6].m_flWeight * 1000.f);

	bool not_to_diff_vel_angle{};
	if (prev_record->layers[6].m_flWeight > 0.0099999998)
		not_to_diff_vel_angle = std::abs(
			math::AngleDiff(
				math::rad_to_deg(std::atan2(player_record->velocity.y, player_record->velocity.x)),
				math::rad_to_deg(std::atan2(prev_record->velocity.y, prev_record->velocity.x))
			)
		) < 10.f;

	const auto not_accelerating = static_cast<int>(cur_layer6_weight * 1000.f) == prev_layer6_weight && !cur_layer12_weight;
	const auto v74 = cur_layer11_weight_valid && (!cur_layer12_weight || (prev_layer11_weight_valid && not_to_diff_vel_angle));

	if (!not_accelerating
		&& !cur_layer11_weight_valid)
	{
		m_accelerating = false;

		goto ANOTHER_CHECK;
	}

	m_accelerating = true;

	if (!not_accelerating)
	{
	ANOTHER_CHECK:
		if (!v74)
		{
			if (std::abs(math::AngleDiff(player_record->angles.y, prev_record->angles.y)) > 35.f)
				detect_side();

			return;
		}
	}


	const auto& cur_layer6 = player_record->layers[6];

	const auto delta1 = std::abs(cur_layer6.m_flPlaybackRate - resolver_layers[MatrixBoneSide::LeftMatrix][6].m_flPlaybackRate);
	const auto delta2 = std::abs(cur_layer6.m_flPlaybackRate - resolver_layers[MatrixBoneSide::RightMatrix][6].m_flPlaybackRate);

	if (static_cast<int>(delta1 * 10000.f) == static_cast<int>(delta2 * 10000.f))
	{
		if (std::abs(math::AngleDiff(player_record->angles.y, prev_record->angles.y)) > 35.f)
			detect_side();
		return;
	}

	const auto delta0 = std::abs(cur_layer6.m_flPlaybackRate - resolver_layers[MatrixBoneSide::ZeroMatrix][6].m_flPlaybackRate);

	float best_delta{};

	if (delta0 <= delta1)
		best_delta = delta0;
	else
		best_delta = delta1;

	if (best_delta > delta2)
		best_delta = delta2;

	if (static_cast<int>(best_delta * 1000.f)
		|| static_cast<int>(best_delta * 10000.f) == static_cast<int>(delta0 * 10000.f))
	{
		if (std::abs(math::AngleDiff(player_record->angles.y, prev_record->angles.y)) > 35.f)
			detect_side();

		return;
	}

	if (best_delta == delta2)
	{

		player_record->curSide = RIGHTT;

		return;
	}

	if (best_delta != delta1)
	{
		if (std::abs(math::AngleDiff(player_record->angles.y, prev_record->angles.y)) > 35.f)
			detect_side();

		return;
	}

	player_record->curSide = LEFTT;

}

float get_backward_side(player_t* player)
{
	return math::calculate_angle(g_ctx.local()->m_vecOrigin(), player->m_vecOrigin()).y;
}

void resolver::detect_side()
{
	player_record->type = ENGINE;
	/* externs */
	Vector src3D, dst3D, forward, right, up, src, dst;
	float back_two, right_two, left_two;
	CGameTrace tr;
	CTraceFilter filter;

	/* angle vectors */
	math::angle_vectors(Vector(0, get_backward_side(player), 0), &forward, &right, &up);

	/* filtering */
	filter.pSkip = player;
	src3D = g_ctx.globals.eye_pos;
	dst3D = src3D + (forward * 384);

	/* back engine tracers */
	m_trace()->TraceRay(Ray_t(src3D, dst3D), MASK_SHOT, &filter, &tr);
	back_two = (tr.endpos - tr.startpos).Length();

	/* right engine tracers */
	m_trace()->TraceRay(Ray_t(src3D + right * 35, dst3D + right * 35), MASK_SHOT, &filter, &tr);
	right_two = (tr.endpos - tr.startpos).Length();

	/* left engine tracers */
	m_trace()->TraceRay(Ray_t(src3D - right * 35, dst3D - right * 35), MASK_SHOT, &filter, &tr);
	left_two = (tr.endpos - tr.startpos).Length();

	/* fix side */
	if (left_two > right_two) {
		player_record->curSide = LEFTT;
	}
	else if (right_two > left_two) {
		player_record->curSide = RIGHTT;
	}
	else
		get_side_trace();
}

void resolver::get_side_trace()
{
	auto m_side = false;
	auto trace = false;
	if (m_globals()->m_curtime - lock_side > 2.0f)
	{
		auto first_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->m_Matricies[MatrixBoneSide::LeftMatrix].data()), player, g_ctx.local());
		auto second_visible = util::visible(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->m_Matricies[MatrixBoneSide::RightMatrix].data()), player, g_ctx.local());

		if (first_visible != second_visible)
		{
			trace = true;
			m_side = second_visible;
			lock_side = m_globals()->m_curtime;
		}
		else
		{
			auto first_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->m_Matricies[MatrixBoneSide::LeftMatrix].data()));
			auto second_position = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->m_Matricies[MatrixBoneSide::RightMatrix].data()));

			if (fabs(first_position - second_position) > 1.0f)
				m_side = first_position > second_position;
		}
	}
	else
		trace = true;

	if (m_side)
	{
		player_record->type = trace ? TRACE : DIRECTIONAL;
		player_record->curSide = RIGHTT;
	}
	else
	{
		player_record->type = trace ? TRACE : DIRECTIONAL;
		player_record->curSide = LEFTT;
	}
}

void resolver::resolve_yaw()
{
	player_info_t player_info;

	if (!m_engine()->GetPlayerInfo(player->EntIndex(), &player_info))
		return;


	if (player_info.fakeplayer || !g_ctx.local()->is_alive() || player->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
	{
		player_record->side = RESOLVER_ORIGINAL;
		return;
	}

	auto animstate = player->get_animation_state();
	if (!animstate)
	{
		player_record->side = RESOLVER_ORIGINAL;
		return;
	}

	bool is_player_zero = false;
	bool is_player_faking = false;
	int positives = 0;
	int negatives = 0;

	fake = false;

	resolver_history res_history = HISTORY_UNKNOWN;

	for (auto it = lagcompensation::get().player_sets.begin(); it != lagcompensation::get().player_sets.end(); ++it)
		if (it->id == player_info.steamID64)
		{
			res_history = it->res_type;
			is_player_faking = it->faking;
			positives = it->pos;
			negatives = it->neg;

			break;
		}

	if (res_history == HISTORY_ZERO)
		is_player_zero = true;

	resolver_type type = resolver_type(-1);

	auto valid_move = true;
	if (animstate->m_velocity > 0.1f)
		valid_move = animstate->m_flTimeSinceStartedMoving < 0.22f;

	if (valid_move && player_record->layers[3].m_flWeight == 0.f && player_record->layers[3].m_flCycle == 0.f && player_record->layers[6].m_flWeight == 0.f)
	{
		auto m_delta = math::AngleDiff(player->m_angEyeAngles().y, resolver_goal_feet_yaw[0]);
		side = 2 * (m_delta <= 0.0f) - 1;
		type = LBY;
	}
	else if (!valid_move && !(int(player_record->layers[12].m_flWeight * 1000.f)) && static_cast<int>(player_record->layers[6].m_flWeight * 1000.f) == static_cast<int>(previous_layers[6].m_flWeight * 1000.f))
	{
		float delta_first = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[0][6].m_flPlaybackRate);
		float delta_second = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[2][6].m_flPlaybackRate);
		float delta_third = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[1][6].m_flPlaybackRate);

		if (delta_first < delta_second || delta_third <= delta_second || (delta_second * 1000.f))
		{
			if (delta_first >= delta_third && delta_second > delta_third && !(delta_third * 1000.f))
				side = 1;
		}
		else
			side = -1;

		type = ANIMATION;
	}
	else
	{

		if (m_globals()->m_curtime - lock_side > 2.0f)
		{
			auto left_damage = autowall::get().wall_penetration(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player).damage;
			auto right_damage = autowall::get().wall_penetration(g_ctx.globals.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player).damage;

			if (abs(left_damage - right_damage) > 10.f)
			{
				if (left_damage > right_damage)
					player_record->side = RESOLVER_FIRST;
				else if (left_damage < right_damage)
					player_record->side = RESOLVER_SECOND;
				if ((fabs(animstate->m_flGoalFeetYaw + 90.f), fabs(animstate->m_flEyeYaw), 40.f))
				{
					player_record->side = RESOLVER_SECOND;
				}
				else
				{
					if (int(left_damage * 10000.f) > int(right_damage * 10000.f))
						player_record->side = RESOLVER_SECOND;
					else
						player_record->side = RESOLVER_FIRST;

					if (right_damage < 1.0f)
						player_record->side = RESOLVER_FIRST;
					if (left_damage < 1.0f)
						player_record->side = RESOLVER_SECOND;
					if (right_damage < left_damage)
						player_record->side = RESOLVER_SECOND;
					else
						player_record->side = RESOLVER_FIRST;
				}
			}
		}
	}

	if (g_ctx.globals.missed_shots[player->EntIndex()] >= 1 || g_ctx.globals.missed_shots[player->EntIndex()] && aim::get().last_target[player->EntIndex()].record.type != LBY)
	{
		switch (last_side)
		{
		case RESOLVER_ORIGINAL:
			g_ctx.globals.missed_shots[player->EntIndex()] = 0;
			fake = true;
			break;
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
			player_record->side = RESOLVER_LOW_FIRST;
			return;
		}
	}
}

float resolver::resolve_pitch()
{
	return original_pitch;
}