#pragma once
#include "../../source-sdk/math/vector3d.hpp"
#include "interfaces.hpp"
#include <cstddef>

struct animstate_pose_param_cache_t {
	std::uint8_t pad_0x0[0x4]; //0x0
	std::uint32_t m_idx; //0x4 
	char* m_name; //0x8

	void set_value(player_t* e, float val);
};

class animstate_t {
public:
	std::uint8_t pad_0x0000[0x4]; //0x0000
	bool m_force_update; //0x0005 
	std::uint8_t pad_0x0006[0x5A]; //0x0006
	entity_t* m_entity; //0x0060 
	weapon_t* m_weapon; //0x0064 
	weapon_t* m_last_weapon; //0x0068 
	float m_last_clientside_anim_update; //0x006C 
	uint32_t m_last_clientside_anim_framecount; //0x0070 
	float m_last_clientside_anim_update_time_delta; //0x0074 
	float m_eye_yaw; //0x0078 
	float m_pitch; //0x007C 
	float m_abs_yaw; //0x0080 
	float m_feet_yaw; //0x0084 
	float m_body_yaw; //0x0088 
	float m_body_yaw_clamped; //0x008C 
	float m_feet_vel_dir_delta; //0x0090 
	uint8_t pad_0x0094[0x4]; //0x0094
	float m_feet_cycle; //0x0098 
	float m_feet_yaw_rate; //0x009C 
	uint8_t pad_0x00A0[0x4]; //0x00A0
	float m_duck_amount; //0x00A4 
	float m_landing_duck_additive; //0x00A8 
	uint8_t pad_0x00AC[0x4]; //0x00AC
	vec3_t m_origin;
	vec3_t m_old_origin;
	vec3_t m_velocity;
	vec3_t m_velocity_normalized;
	vec3_t m_last_accelerating_vel;
	float m_speed2d;
	float m_abs_velocity;
	float m_speed_normalized;
	float m_run_speed; //0x00F8 
	float m_duck_speed; //0x00FC 
	float m_time_since_move; //0x0100 
	float m_time_since_stop; //0x0104 
	bool m_on_ground; //0x0108 
	bool m_hit_ground; //0x0109 
	uint8_t pad_0x010A[0x4]; //0x010A
	float m_time_in_air; //0x0110 
	uint8_t pad_0x0114[0x6]; //0x0114
	float m_ground_fraction; //0x011C 
	uint8_t pad_0x0120[0x2]; //0x0120
	float m_unk_fraction; //0x0124 
	uint8_t pad_0x0128[0xC]; //0x0128
	bool m_moving; //0x0134
	uint8_t pad_0x0135[0x7B]; //0x0135
	animstate_pose_param_cache_t m_lean_yaw_pose; //0x1B0
	animstate_pose_param_cache_t m_speed_pose; //0x01BC
	animstate_pose_param_cache_t m_ladder_speed_pose; //0x01C8
	animstate_pose_param_cache_t m_ladder_yaw_pose; //0x01D4
	animstate_pose_param_cache_t m_move_yaw_pose; //0x01E0
	animstate_pose_param_cache_t m_run_pose; //0x01EC 
	animstate_pose_param_cache_t m_body_yaw_pose; //0x01F8
	animstate_pose_param_cache_t m_body_pitch_pose; //0x0204
	animstate_pose_param_cache_t m_dead_yaw_pose; //0x0210
	animstate_pose_param_cache_t m_stand_pose; //0x021C
	animstate_pose_param_cache_t m_jump_fall_pose; //0x0228
	animstate_pose_param_cache_t m_aim_blend_stand_idle_pose; //0x0234
	animstate_pose_param_cache_t m_aim_blend_crouch_idle_pose; //0x0240
	animstate_pose_param_cache_t m_strafe_yaw_pose; //0x024C
	animstate_pose_param_cache_t m_aim_blend_stand_walk_pose; //0x0258
	animstate_pose_param_cache_t m_aim_blend_stand_run_pose; //0x0264
	animstate_pose_param_cache_t m_aim_blend_crouch_walk_pose; //0x0270
	animstate_pose_param_cache_t m_move_blend_walk_pose; //0x027C
	animstate_pose_param_cache_t m_move_blend_run_pose; //0x0288
	animstate_pose_param_cache_t m_move_blend_crouch_pose; //0x0294
	uint8_t pad_0x02A0[0x4]; //0x02A0
	float m_vel_unk; //0x02A4 
	uint8_t pad_0x02A8[0x86]; //0x02A8
	float m_min_yaw; //0x0330 
	float m_max_yaw; //0x0334 
	float m_max_pitch; //0x0338 
	float m_min_pitch; //0x033C

	void reset();
	void update(vec3_t& ang);

	float& time_since_in_air() {
		return *(float*)((uintptr_t)this + 0x110);
	}

	float& yaw_desync_adjustment() {
		return *(float*)((uintptr_t)this + 0x334);
	}

	bool& m_smooth_height_valid() {
		return *(bool*)((uintptr_t)this + 0x328);
	}
}; //Size=0x0340