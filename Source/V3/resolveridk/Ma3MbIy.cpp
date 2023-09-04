//includes
#include "RageBot.h"
#include "RageIncludes.h"

void Rbot::UpdateConfig() {
    RageConfig::enable = cfg::g_cfg.ragebot.enable;
    RageConfig::zeus_bot = cfg::g_cfg.ragebot.zeus_bot;
    RageConfig::knife_bot = cfg::g_cfg.ragebot.knife_bot;
    RageConfig::autoshoot = cfg::g_cfg.ragebot.autoshoot;
    RageConfig::extrapolation = cfg::g_cfg.ragebot.extrapolation;
    RageConfig::headshot_only = cfg::g_cfg.ragebot.headshot_only;
    RageConfig::accurate_fd = cfg::g_cfg.ragebot.accurate_fd;
    RageConfig::dormant_aimbot = cfg::g_cfg.ragebot.dormant_aimbot;
    RageConfig::charge_time = cfg::g_cfg.ragebot.charge_time;
    RageConfig::double_tap = cfg::g_cfg.ragebot.double_tap && key_binds::get().get_key_bind_state(DOUBLE_TAP);
    RageConfig::exploit_modifiers = cfg::g_cfg.ragebot.exploit_modifiers;
    RageConfig::force_safe = key_binds::get().get_key_bind_state(FORCE_SAFE);
    RageConfig::force_body = key_binds::get().get_key_bind_state(FORCE_BODY);
    RageConfig::damage_override = key_binds::get().get_key_bind_state(DAMAGE_OVERRIDE);
    RageConfig::resolver = cfg::g_cfg.ragebot.resolver;
    RageConfig::roll_resolver = key_binds::get().get_key_bind_state(ROLL_RESOLVER);

    //Weapon
    for (auto i = 0; i < 9; i++)
    {
        RageConfig::weapon[i].dt_hitchance = cfg::g_cfg.ragebot.weapon[i].double_tap_hitchance_amount;
        RageConfig::weapon[i].hitchance = cfg::g_cfg.ragebot.weapon[i].hitchance_amount;
        RageConfig::weapon[i].min_dmg = cfg::g_cfg.ragebot.weapon[i].min_dmg;
        RageConfig::weapon[i].min_override_dmg = cfg::g_cfg.ragebot.weapon[i].min_override_dmg;
        RageConfig::weapon[i].hitboxes = cfg::g_cfg.ragebot.weapon[i].hitboxes;
        RageConfig::weapon[i].head_scale = cfg::g_cfg.ragebot.weapon[i].head_scale;
        RageConfig::weapon[i].body_scale = cfg::g_cfg.ragebot.weapon[i].body_scale;
        RageConfig::weapon[i].multipoints = cfg::g_cfg.ragebot.weapon[i].multipoints;
        RageConfig::weapon[i].prefer_baim = cfg::g_cfg.ragebot.weapon[i].prefer_body_aim;
        RageConfig::weapon[i].prefer_baim_mode = cfg::g_cfg.ragebot.weapon[i].prefer_body_aim_mode;
        RageConfig::weapon[i].autostop = cfg::g_cfg.ragebot.weapon[i].autostop;
        RageConfig::weapon[i].autostop_modifiers = cfg::g_cfg.ragebot.weapon[i].autostop_modifiers;
        RageConfig::weapon[i].autoscope = cfg::g_cfg.ragebot.weapon[i].autoscope;
        RageConfig::weapon[i].autoscope_mode = cfg::g_cfg.ragebot.weapon[i].autoscope_mode;
        RageConfig::weapon[i].targeting_mode = cfg::g_cfg.ragebot.weapon[i].selection_type;
    }
}

void Rbot::Reset() {
    backup.clear();
    targets.clear();
    scanned_targets.clear();
    final_target.reset();
    Should_AutoStop = false;
}

void Rbot::Run(CUserCmd* cmd) {
    Reset();
    UpdateConfig();

    //return checks
    if (!RageConfig::enable
        || !cmd
        || !g_ctx.local()
        || util::is_button_down(MOUSE_LEFT)
        ) return;

    AutoR8(cmd);
    PrepareTargets();

    //return checks again
    if (g_ctx.globals.weapon->is_non_aim() || g_ctx.globals.current_weapon == -1)
        return;

    StartScan();

    QuickStop(cmd);

    bool empty = scanned_targets.empty();
    if (empty)
        return;

    FindOptimalTarget();

    bool valid = final_target.data.valid();
    if (!valid || !g_ctx.globals.weapon->can_fire(true))
        return;

    Fire(cmd);

    if (RageConfig::accurate_fd) {
        if (g_ctx.globals.fakeducking && g_ctx.local()->m_flDuckAmount() != 0) {
            cmd->m_buttons &= ~IN_ATTACK;
            return;
        }
    }
}

int Rbot::GetDamage(int health) {
    int bullets = g_ctx.globals.weapon->m_iClip1();
    auto minimum_damage = RageConfig::damage_override ? RageConfig::weapon[g_ctx.globals.current_weapon].min_override_dmg : RageConfig::weapon[g_ctx.globals.current_weapon].min_dmg;

    if (minimum_damage > 100)
        minimum_damage = health + minimum_damage - 100;
    else if (minimum_damage > health)
        minimum_damage = health + 1;

    return minimum_damage;
}

void Rbot::AutoR8(CUserCmd* cmd) {
    if (!m_engine()->IsActiveApp())
        return;

    if (g_ctx.globals.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
        return;

    if (cmd->m_buttons & IN_ATTACK)
        return;

    cmd->m_buttons &= ~IN_ATTACK2;

    static auto r8cock_time = 0.0f;
    auto server_time = TICKS_TO_TIME(g_ctx.globals.backup_tickbase);

    if (g_ctx.globals.weapon->can_fire(false)) {
        if (r8cock_time <= server_time) {
            if (g_ctx.globals.weapon->m_flNextSecondaryAttack() <= server_time)
                r8cock_time = server_time + 0.234375f;
            else
                cmd->m_buttons |= IN_ATTACK2;
        }
        else
            cmd->m_buttons |= IN_ATTACK;
    }
    else {
        r8cock_time = server_time + 0.234375f;
        cmd->m_buttons &= ~IN_ATTACK;
    }

    g_ctx.globals.revolver_working = true;
}

void Rbot::PrepareTargets() {
    player_info_t player_info;

    for (auto i = 1; i < m_globals()->m_maxclients; i++) {
        if (!m_engine()->GetPlayerInfo(i, &player_info) || player_info.iSteamID == 522657078)
            continue;

        auto e = (player_t*)m_entitylist()->GetClientEntity(i);
        if (!e->valid(!m_cvar()->FindVar("mp_teammates_are_enemies")->GetBool(), RageConfig::dormant_aimbot))
            continue;

        auto records = &player_records[i];
        if (records->empty())
            continue;

        auto adjust = RecieveRecords(records, false);
        if (!adjust)
            continue;

        targets.emplace_back(target(e, adjust, RecieveRecords(records, true)));
    }

    for (auto& target : targets)
        backup.emplace_back(adjust_data(target.e));
}

static bool CompareRecords(const optimized_adjust_data& first, const optimized_adjust_data& second) {
    if (first.shot)
        return first.shot > second.shot;
    else if (second.shot)
        return second.shot > first.shot;

    return first.simulation_time > second.simulation_time;
}

adjust_data* Rbot::RecieveRecords(std::deque < adjust_data >* records, bool history) {
    if (history) {
        std::deque <optimized_adjust_data> optimized_records;
        for (auto& record : *records) {
            optimized_adjust_data optimized_record;
            optimized_record.player = record.player;
            optimized_record.simulation_time = record.simulation_time;
            optimized_record.shot = record.shot;

            optimized_records.emplace_back(optimized_record);
        }

        if (optimized_records.size() < 2)
            return nullptr;

        std::sort(optimized_records.begin(), optimized_records.end(), CompareRecords);

        for (auto& optimized_record : optimized_records) {
            auto record = &records->at(optimized_record.i);
            if (record->valid())
                return record;
        }
    }
    else {
        for (auto i = 0; i < records->size(); ++i) {
            auto record = &records->at(i);
            if (!record->valid())
                continue;

            return record;
        }
    }

    return nullptr;
}

void Rbot::StartScan() {
    if (targets.empty())
        return;

    for (auto& target : targets) {
        scan_data last_data;
        scan_data history_data;

        if (target.last_record->valid()) {
            target.last_record->adjust_player();
            Scan(target.last_record, last_data);
        }

        if (target.history_record->valid()) {
            target.history_record->adjust_player();
            Scan(target.history_record, history_data);

            if (history_data.valid() && (last_data.valid() ? last_data.damage < history_data.damage : true))
                scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
        }

        if (last_data.valid())
            scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
    }
}

int Rbot::GetTicksToShoot() {
    if (g_ctx.globals.weapon->can_fire(true))
        return -1;

    auto flServerTime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
    auto flNextPrimaryAttack = g_ctx.local()->m_hActiveWeapon()->m_flNextPrimaryAttack();

    return TIME_TO_TICKS(fabsf(flNextPrimaryAttack - flServerTime));
}

int Rbot::GetTicksToStop() {
    static auto predict_velocity = [](Vector* velocity)
    {
        float speed = velocity->Length2D();
        static auto sv_friction = m_cvar()->FindVar("sv_friction");
        static auto sv_stopspeed = m_cvar()->FindVar("sv_stopspeed");

        if (speed >= 1.f)
        {
            float friction = sv_friction->GetFloat();
            float stop_speed = std::max< float >(speed, sv_stopspeed->GetFloat());
            float time = std::max< float >(m_globals()->m_intervalpertick, m_globals()->m_frametime);
            *velocity *= std::max< float >(0.f, speed - friction * stop_speed * time / speed);
        }
    };
    Vector vel = g_ctx.local()->m_vecVelocity();
    int ticks_to_stop = 0;
    for (;;)
    {
        if (vel.Length2D() < 1.f)
            break;
        predict_velocity(&vel);
        ticks_to_stop++;
    }
    return ticks_to_stop;
}

void Rbot::QuickStop(CUserCmd* cmd) {
    if (!Should_AutoStop)
        return;

    if (!RageConfig::weapon[g_ctx.globals.current_weapon].autostop)
        return;

    if (g_ctx.globals.slowwalking)
        return;

    if (!(g_ctx.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
        return;

    if (g_ctx.globals.weapon->is_empty())
        return;

    if (!RageConfig::weapon[g_ctx.globals.current_weapon].autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !g_ctx.globals.weapon->can_fire(false))
        return;


    auto animlayer = g_ctx.local()->get_animlayers()[1];
    if (animlayer.m_nSequence) {
        auto activity = g_ctx.local()->sequence_activity(animlayer.m_nSequence);

        if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
            return;
    }

    auto weapon_info = g_ctx.globals.weapon->get_csweapon_info();
    if (!weapon_info)
        return;

    auto max_speed = 0.33f * (g_ctx.globals.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);
    if (engineprediction::get().backup_data.velocity.Length2D() < max_speed) {
        slowwalk::get().create_move(cmd);
        return;
    }

    Vector direction;
    Vector real_view;

    math::vector_angles(engineprediction::get().backup_data.velocity, direction);
    m_engine()->GetViewAngles(real_view);

    direction.y = real_view.y - direction.y;

    Vector forward;
    math::angle_vectors(direction, forward);

    static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
    static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

    auto negative_forward_speed = -cl_forwardspeed->GetFloat();
    auto negative_side_speed = -cl_sidespeed->GetFloat();

    auto negative_forward_direction = forward * negative_forward_speed;
    auto negative_side_direction = forward * negative_side_speed;

    cmd->m_forwardmove = negative_forward_direction.x;
    cmd->m_sidemove = negative_side_direction.y;
}

bool Rbot::IsSafePoint(adjust_data* record, Vector start_position, Vector end_position, int hitbox) {
    if (!HitboxIntersection(record->player, record->matrixes_data.zero, hitbox, start_position, end_position))
        return false;
    else if (!HitboxIntersection(record->player, record->matrixes_data.first, hitbox, start_position, end_position))
        return false;
    else if (!HitboxIntersection(record->player, record->matrixes_data.second, hitbox, start_position, end_position))
        return false;

    return true;
}

void Rbot::Scan(adjust_data* record, scan_data& data, const Vector& shoot_position) {
    auto weapon = g_ctx.globals.weapon;
    if (!weapon)
        return;

    auto weapon_info = weapon->get_csweapon_info();
    if (!weapon_info)
        return;

    auto hitboxes = GetHitboxes(record);
    if (hitboxes.empty()) //we should not be getting this.
        return;

    auto best_hitbox = 0;
    auto best_damage = 0;
    auto minimum_damage = GetDamage(record->player->m_iHealth());

    std::vector <scan_point> points;
    for (auto hitbox : hitboxes) {
        auto current_points = Rbot::GetPoints(record, hitbox, g_ctx.globals.framerate > 60);

        for (auto& point : current_points) {
            if (!record->bot)
                point.safe = IsSafePoint(record, shoot_position, point.point, hitbox);
            else
                point.safe = true;

            points.emplace_back(point);
        }
    }

    if (points.empty())
        return;

    bool found = false;
    scan_data best;
    for (auto& point : points) {
        autowall::returninfo_t fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player);

        if (!fire_data.valid) //break iteration in this loop and continue to next.
            continue;

        if (fire_data.damage > minimum_damage) {
            Should_AutoStop = GetTicksToShoot() <= GetTicksToStop() || RageConfig::weapon[g_ctx.globals.current_weapon].autostop_modifiers.at(1) && !g_ctx.globals.weapon->can_fire(true);

            if (best.damage < fire_data.damage && fire_data.hitbox != CSGOHitboxID::Head) {
                best.point = point;
                best.visible = fire_data.visible;
                best.damage = fire_data.damage;
                best.hitbox = fire_data.hitbox;
            }

            if (fire_data.hitbox == CSGOHitboxID::Head && found == false) {
                best.point = point;
                best.visible = fire_data.visible;
                best.damage = fire_data.damage;
                best.hitbox = fire_data.hitbox;
            }

            data.point = best.point;
            data.visible = best.visible;
            data.damage = best.damage;
            data.hitbox = best.hitbox;
            found = true;
        }
    }
}

std::vector < int > Rbot::GetHitboxes(adjust_data* record) {
    std::vector <int> hitboxes;

    if (RageConfig::headshot_only) {
        hitboxes.emplace_back(CSGOHitboxID::Head);
        return hitboxes;
    }

    if (RageConfig::force_body) {
        hitboxes.emplace_back(CSGOHitboxID::UpperChest);
        hitboxes.emplace_back(CSGOHitboxID::Chest);
        hitboxes.emplace_back(CSGOHitboxID::LowerChest);
        hitboxes.emplace_back(CSGOHitboxID::Stomach);
        hitboxes.emplace_back(CSGOHitboxID::Pelvis);
        return hitboxes;
    }

    if (RageConfig::weapon[g_ctx.globals.current_weapon].hitboxes.at(3)) {
        hitboxes.emplace_back(CSGOHitboxID::RightUpperArm);
        hitboxes.emplace_back(CSGOHitboxID::LeftUpperArm);
    }

    if (RageConfig::weapon[g_ctx.globals.current_weapon].hitboxes.at(1)) {
        hitboxes.emplace_back(CSGOHitboxID::Neck);
        hitboxes.emplace_back(CSGOHitboxID::UpperChest);
        hitboxes.emplace_back(CSGOHitboxID::Chest);
        hitboxes.emplace_back(CSGOHitboxID::LowerChest);
        hitboxes.emplace_back(CSGOHitboxID::Stomach);
    }

    if (RageConfig::weapon[g_ctx.globals.current_weapon].hitboxes.at(2))
        hitboxes.emplace_back(CSGOHitboxID::Pelvis);

    if (RageConfig::weapon[g_ctx.globals.current_weapon].hitboxes.at(4)) {
        hitboxes.emplace_back(CSGOHitboxID::RightThigh);
        hitboxes.emplace_back(CSGOHitboxID::LeftThigh);

        hitboxes.emplace_back(CSGOHitboxID::RightCalf);
        hitboxes.emplace_back(CSGOHitboxID::LeftCalf);
    }

    if (RageConfig::weapon[g_ctx.globals.current_weapon].hitboxes.at(5)) {
        hitboxes.emplace_back(CSGOHitboxID::RightFoot);
        hitboxes.emplace_back(CSGOHitboxID::LeftFoot);
    }

    if (RageConfig::weapon[g_ctx.globals.current_weapon].hitboxes.at(0))
        hitboxes.emplace_back(CSGOHitboxID::Head);

    return hitboxes;
}

std::vector < scan_point > Rbot::GetPoints(adjust_data* record, int hitbox, bool optimized) {
    std::vector<scan_point> points;

    auto model = record->player->GetModel();

    if (!model)
        return points;

    auto hdr = m_modelinfo()->GetStudioModel(model);

    if (!hdr)
        return points;

    auto set = hdr->pHitboxSet(record->player->m_nHitboxSet());

    if (!set)
        return points;

    auto bbox = set->pHitbox(hitbox);

    if (!bbox)
        return points;

    auto modifier = bbox->radius != -1.f ? bbox->radius : 0.f;

    auto min = ZERO, max = ZERO;
    math::vector_transform(bbox->bbmin, record->matrixes_data.main[bbox->bone], min);
    math::vector_transform(bbox->bbmax, record->matrixes_data.main[bbox->bone], max);

    auto center = (min + max) * 0.5f;

    auto angle = math::calculate_angle(g_ctx.globals.eye_pos, center);

    auto cos = cosf(DEG2RAD(angle.y));
    auto sin = sinf(DEG2RAD(angle.y));

    auto head_resize = modifier * RageConfig::weapon[g_ctx.globals.current_weapon].head_scale;
    auto body_resize = modifier * RageConfig::weapon[g_ctx.globals.current_weapon].body_scale;

    auto& upper = min;

    //Add multipoints another time because before they dropped hella fps.
    points.emplace_back(scan_point(center, hitbox, true));

    return points;
}

static bool Compare(const scanned_target& first, const scanned_target& second) {
    switch (RageConfig::weapon[g_ctx.globals.current_weapon].targeting_mode) {
    case 0:
        return first.health < second.health;
    case 1:
        return first.health > second.health;
    case 2:
        return first.fov < second.fov;
    case 3:
        return first.data.damage > second.data.damage;
    }
}

void Rbot::FindOptimalTarget() {
    if (RageConfig::weapon[g_ctx.globals.current_weapon].targeting_mode)
        std::sort(scanned_targets.begin(), scanned_targets.end(), Compare);

    for (auto& target : scanned_targets) {
        if (target.fov > 180)
            continue;

        final_target = target;
        final_target.record->adjust_player();

        break;
    }
}

bool Rbot::HitboxIntersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end) {
    trace_t Trace;
    Ray_t Ray = Ray_t(start, end);

    Trace.fraction = 1.0f;
    Trace.startsolid = false;

    auto studio_model = m_modelinfo()->GetStudioModel(e->GetModel());
    if (!studio_model)
        return false;

    auto studio_set = studio_model->pHitboxSet(e->m_nHitboxSet());
    if (!studio_set)
        return false;

    auto studio_hitbox = studio_set->pHitbox(hitbox);
    if (!studio_hitbox)
        return false;

    return ClipRayToHitbox(Ray, studio_hitbox, matrix[studio_hitbox->bone], Trace);
}

bool DoAutoScope(CUserCmd* cmd, bool MetHitchance) {
    if (!RageConfig::weapon[g_ctx.globals.current_weapon].autoscope)
        return false;

    bool IsHitchanceFail =
        RageConfig::weapon[g_ctx.globals.current_weapon].autoscope_mode > 0;

    bool is_zoomable_weapon =
        g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 ||
        g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 ||
        g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 ||
        g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP ||
        g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AUG ||
        g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SG553;

    if (IsHitchanceFail && !MetHitchance) {
        if (is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
            cmd->m_buttons |= IN_ATTACK2;

        return true;
    }
    else if (!IsHitchanceFail) {
        if (is_zoomable_weapon && !g_ctx.globals.weapon->m_zoomLevel())
            cmd->m_buttons |= IN_ATTACK2;

        return true;
    }

    return false;
}

bool DoBacktrack(int ticks, scanned_target target) {
    auto net_channel_info = m_engine()->GetNetChannelInfo();
    if (net_channel_info) {
        auto original_tickbase = g_ctx.globals.backup_tickbase;
        auto max_tickbase_shift = m_gamerules()->m_bIsValveDS() ? 6 : 16;

        static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

        auto correct = math::clamp(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());
        auto delta_time = correct - (TICKS_TO_TIME(original_tickbase) - target.record->simulation_time);

        ticks = TIME_TO_TICKS(fabs(delta_time));

        return true;
    }

    return false;
}

bool UseDoubleTapHitchance() {
    if (g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08
        || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_AWP
        || g_ctx.globals.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER
        || g_ctx.globals.weapon->is_shotgun())
       // || exploit::get().shots != 1)
        return false;

    return true;
}

void Rbot::Fire(CUserCmd* cmd) {
    if (!RageConfig::autoshoot && !(cmd->m_buttons & IN_ATTACK))
        return;

    auto aim_angle = math::calculate_angle(g_ctx.globals.eye_pos, final_target.data.point.point).Clamp();

    auto hitchance = UseDoubleTapHitchance() ? RageConfig::weapon[g_ctx.globals.current_weapon].dt_hitchance : RageConfig::weapon[g_ctx.globals.current_weapon].hitchance;
    bool Hitchance_Valid = //g_hit_chance->can_hit(hitchance, final_target.record, g_ctx.globals.weapon, aim_angle, final_target.data.hitbox);

    DoAutoScope(cmd, Hitchance_Valid);

    if (!Hitchance_Valid)
        return;

    auto bt_ticks = 0;
    bool BackTracked = DoBacktrack(bt_ticks, final_target);

    player_info_t player_info;
    m_engine()->GetPlayerInfo(final_target.record->i, &player_info);

    cmd->m_viewangles = aim_angle;
    cmd->m_buttons |= IN_ATTACK;
    cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time + util::get_interpolation());

    last_target_index = final_target.record->i;
    last_shoot_position = g_ctx.globals.eye_pos;
    last_target[last_target_index] = Last_target{ *final_target.record, final_target.data, final_target.distance };

    ++shots_fired;

    auto shot = &g_ctx.shots.emplace_back();
    shot->last_target = last_target_index;
    shot->side = final_target.record->side;
    shot->fire_tick = m_globals()->m_tickcount;
    shot->shot_info.target_name = player_info.szName;
    shot->shot_info.client_hitbox = get_hitbox_name(final_target.data.hitbox, true);
    shot->shot_info.client_damage = final_target.data.damage;
    shot->shot_info.hitchance = hitchance; //for cfg currently.
    shot->shot_info.backtrack_ticks = BackTracked ? bt_ticks : -1;
    shot->shot_info.aim_point = final_target.data.point.point;

    g_ctx.globals.aimbot_working = true;
    g_ctx.globals.revolver_working = false;
    g_ctx.globals.last_aimbot_shot = m_globals()->m_tickcount;
}