#pragma once
#include "../../source-sdk/math/vector3d.hpp"
#include "../../source-sdk/classes/c_usercmd.hpp"

class player_move_helper {
public:
	bool	first_run_of_iunctions : 1;
	bool	game_code_moved_player : 1;
	int	player_handle;
	int	impulse_command;
	vec3_t	view_angles;
	vec3_t	abs_view_angles;
	int	buttons;
	int	old_buttons;
	float	forward_move;
	float	side_move;
	float	up_move;
	float	max_speed;
	float	client_max_speed;
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t	old_angles;
	float	out_step_height;
	vec3_t	wish_velocity;
	vec3_t	jump_velocity;
	vec3_t	constraint_center;
	float	constraint_radius;
	float	constraint_width;
	float	constraint_speed_factor;
	float	u0[5];
	vec3_t	abs_origin;
	virtual	void u1() = 0;
	virtual void set_host(player_t* host) = 0;
};

class player_move_data {
public:
	bool    first_run_of_instructions : 1;
	bool    game_code_moved_player : 1;
	int     player_handle;
	int     impulse_command;
	vec3_t	view_angles;
	vec3_t	abs_view_angles;
	int     buttons;
	int     old_buttons;
	float   fw_move;
	float   sd_move;
	float   up_move;
	float   max_speed;
	float   client_max_speed;
	vec3_t	velocity;
	vec3_t	angles;
	vec3_t	old_angles;
	float   step_height;
	vec3_t	wish_velocity;
	vec3_t	jump_velocity;
	vec3_t	constraint_center;
	float   constraint_radius;
	float   constraint_width;
	float   constraint_speed_factor;
	float   u0[5];
	vec3_t	abs_origin;
};

class virtual_game_movement {

public:
	virtual				~virtual_game_movement(void) {}
	virtual void			process_movement(player_t* player, player_move_data* move) = 0;
	virtual void			reset(void) = 0;
	virtual void			start_track_prediction_errors(player_t* player) = 0;
	virtual void			finish_track_prediction_errors(player_t* player) = 0;
	virtual void			diff_print(char const* fmt, ...) = 0;
	virtual vec3_t const& get_player_mins(bool ducked) const = 0;
	virtual vec3_t const& get_player_maxs(bool ducked) const = 0;
	virtual vec3_t const& get_player_view_offset(bool ducked) const = 0;
	virtual bool			is_moving_player_stuck(void) const = 0;
	virtual player_t* get_moving_player(void) const = 0;
	virtual void			unblock_posher(player_t* player, player_t* pusher) = 0;
	virtual void			setup_movement_bounds(player_move_data* move) = 0;
};

class player_game_movement : public virtual_game_movement {
public:
	virtual ~player_game_movement(void) { }
};

class player_prediction {
public:
	std::byte		pad0[0x4];						// 0x0000
	std::uintptr_t	hLastGround;					// 0x0004
	bool			bInPrediction;					// 0x0008
	bool			bIsFirstTimePredicted;			// 0x0009
	bool			bEnginePaused;					// 0x000A
	bool			bOldCLPredictValue;				// 0x000B
	int				iPreviousStartFrame;			// 0x000C
	int				nIncomingPacketNumber;			// 0x0010
	float			flLastServerWorldTimeStamp;		// 0x0014

	struct Split_t
	{
		bool		bIsFirstTimePredicted;			// 0x0018
		std::byte	pad0[0x3];						// 0x0019
		int			nCommandsPredicted;				// 0x001C
		int			nServerCommandsAcknowledged;	// 0x0020
		int			iPreviousAckHadErrors;			// 0x0024
		float		flIdealPitch;					// 0x0028
		int			iLastCommandAcknowledged;		// 0x002C
		bool		bPreviousAckErrorTriggersFullLatchReset; // 0x0030
	};
	Split_t			Split[1];						// 0x0018

public:
public:
	bool in_prediction() {
		typedef bool(__thiscall* fn)(void*);
		return utilities::call_virtual<fn>(this, 14)(this);
	}
	void run_command(player_t* player, c_usercmd* cmd, player_move_helper* helper) {
		typedef void(__thiscall* fn)(void*, player_t*, c_usercmd*, player_move_helper*);
		return utilities::call_virtual<fn>(this, 19)(this, player, cmd, helper);
	}
	void check_moving_ground(player_t* player, double frametime) {
		typedef void(__thiscall* fn)(void*, player_t*, double);
		return utilities::call_virtual<fn>(this, 18)(this, player, frametime);
	}
	void set_local_view_angles(vec3_t& ang) {
		typedef void(__thiscall* fn)(void*, vec3_t&);
		return utilities::call_virtual<fn>(this, 13)(this, ang);
	}
	void setup_move(player_t* player, c_usercmd* cmd, player_move_helper* helper, player_move_data* data) {
		typedef void(__thiscall* fn)(void*, player_t*, c_usercmd*, player_move_helper*, player_move_data*);
		return utilities::call_virtual<fn>(this, 20)(this, player, cmd, helper, data);
	}
	void finish_move(player_t* player, c_usercmd* cmd, player_move_data* data) {
		typedef void(__thiscall* fn)(void*, player_t*, c_usercmd*, player_move_data*);
		return utilities::call_virtual<fn>(this, 21)(this, player, cmd, data);
	}
};