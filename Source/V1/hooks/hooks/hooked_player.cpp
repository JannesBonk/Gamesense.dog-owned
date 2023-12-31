// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "..\..\cheats\misc\prediction_system.h"

_declspec(noinline)bool hooks::setupbones_detour(void* ecx, matrix3x4_t* bone_world_out, int max_bones, int bone_mask, float current_time)
{
    auto result = true;

    static auto r_jiggle_bones = m_cvar()->FindVar(crypt_str("r_jiggle_bones"));
    auto r_jiggle_bones_backup = r_jiggle_bones->GetInt();

    r_jiggle_bones->SetValue(0);

    if (!ecx)
        result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
    else if ((!c_config::get()->b["rage_enabled"] && !c_config::get()->auto_check(c_config::get()->i["rage_key_enabled"], c_config::get()->i["rage_key_enabled_st"])))
        result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
    else
    {
        auto player = (player_t*)((uintptr_t)ecx - 0x4);

        if (!player->valid(false, false))
            result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
        else
        {
            auto animstate = player->get_animation_state();
            auto previous_weapon = animstate ? animstate->m_pLastBoneSetupWeapon : nullptr;

            if (previous_weapon)
                animstate->m_pLastBoneSetupWeapon = animstate->m_pActiveWeapon; //-V1004

            if (g_ctx.globals.setuping_bones)
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (g_cfg.legitbot.enabled && player != g_ctx.local())
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (!g_ctx.local()->is_alive())
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (player == g_ctx.local())
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (!player->m_CachedBoneData().Count()) //-V807
                result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
            else if (bone_world_out && max_bones != -1)
                memcpy(bone_world_out, player->m_CachedBoneData().Base(), player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

            if (previous_weapon)
                animstate->m_pLastBoneSetupWeapon = previous_weapon;
        }
    }

    r_jiggle_bones->SetValue(r_jiggle_bones_backup);
    return result;
}

bool __fastcall hooks::hooked_setupbones(void* ecx, void* edx, matrix3x4_t* bone_world_out, int max_bones, int bone_mask, float current_time)
{
    return setupbones_detour(ecx, bone_world_out, max_bones, bone_mask, current_time);
}

_declspec(noinline)void hooks::standardblendingrules_detour(player_t* player, int i, CStudioHdr* hdr, Vector* pos, Quaternion* q, float curtime, int boneMask)
{
    if (!player || !player->is_player() || player->IsDormant()) {
        return ((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);
    }

    // disable interpolation on bones.
    *(uint32_t*)((uintptr_t)player + 0xf0) |= 8;

    ((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);

    *(uint32_t*)((uintptr_t)player + 0xf0) &= ~8;

    // disable only on hitbox.
    boneMask |= BONE_USED_BY_HITBOX;
}

void __fastcall hooks::hooked_standardblendingrules(player_t* player, int i, CStudioHdr* hdr, Vector* pos, Quaternion* q, float curtime, int boneMask)
{
    return standardblendingrules_detour(player, i, hdr, pos, q, curtime, boneMask);
}

_declspec(noinline)void hooks::doextrabonesprocessing_detour(player_t* player, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* context)
{
    // get animstate ptr.
    auto animstate = player->get_animation_state();

    // backup pointer.
    player_t* backup = nullptr;

    // zero out animstate player pointer so CCSGOPlayerAnimState::DoProceduralFootPlant will not do anything.
    if (animstate) {
        // backup player ptr.
        backup = animstate->m_pBaseEntity;

        // null player ptr, GUWOP gang.
        animstate->m_pBaseEntity = nullptr;
    }

    ((DoExtraBonesProcessingFn)original_doextrabonesprocessing)(player, hdr, pos, q, matrix, bone_list, context);

    // restore ptr.
    if (animstate && backup)
        animstate->m_pBaseEntity = backup;
}

void __fastcall hooks::hooked_doextrabonesprocessing(player_t* player, void* edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* context)
{
    return doextrabonesprocessing_detour(player, hdr, pos, q, matrix, bone_list, context);
}

_declspec(noinline)void hooks::updateclientsideanimation_detour(player_t* player)
{
    if (g_ctx.globals.updating_animation)
        return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

    if (player == g_ctx.local())
        return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

    if (!c_config::get()->b["rage_enabled"])
        return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

    if (!player->valid(false, false))
        return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);
}

void __fastcall hooks::hooked_updateclientsideanimation(player_t* player, uint32_t i)
{
    return updateclientsideanimation_detour(player);
}

_declspec(noinline)void hooks::physicssimulate_detour(player_t* player)
{
    auto& m_nSimulationTick = *reinterpret_cast<int*>(uintptr_t(player) + 0x2AC);
    auto cctx = reinterpret_cast<C_CommandContext*>(uintptr_t(player) + 0x34FC);

    if (!player
        || !player->is_alive()
        || m_globals()->m_tickcount == m_nSimulationTick
        || player != g_ctx.local()
        || !cctx->needsprocessing
        || m_engine()->IsPlayingDemo()
        || m_engine()->IsHLTV()
        || player->m_fFlags() & 0x40)
    {
        ((PhysicsSimulateFn)original_physicssimulate)(player);
        return;
    }

    player->m_vphysicsCollisionState() = 0;

    const bool bValid = (cctx->cmd.m_tickcount != 0x7FFFFFFF);

    if (!bValid)
    {
        auto ndata = &engineprediction::get().m_Data[cctx->cmd.m_command_number % 150];

        ndata->m_nTickBase = player->m_nTickBase();

        ndata->command_number = cctx->cmd.m_command_number;
        ndata->m_aimPunchAngle = player->m_aimPunchAngle();
        ndata->m_aimPunchAngleVel = player->m_aimPunchAngleVel();
        ndata->m_viewPunchAngle = player->m_viewPunchAngle();
        ndata->m_vecViewOffset = player->m_vecViewOffset();
        ndata->m_vecViewOffset.z = fminf(fmaxf(ndata->m_vecViewOffset.z, 46.0f), 64.0f);
        ndata->m_vecVelocity = player->m_vecVelocity();
        ndata->m_vecOrigin = player->m_vecOrigin();
        ndata->m_flFallVelocity = player->m_flFallVelocity();
        ndata->m_flDuckAmount = player->m_flDuckAmount();
        ndata->m_flVelocityModifier = player->m_flVelocityModifier();
        ndata->tick_count = cctx->cmd.m_tickcount;
        ndata->is_filled = true;

        m_nSimulationTick = m_globals()->m_tickcount;
        cctx->needsprocessing = false;

        return;
    }
    else
    {
        const auto data = &engineprediction::get().m_Data[cctx->cmd.m_command_number % 150];

        if (data && data->command_number == (cctx->cmd.m_command_number - 1) && abs(data->command_number - cctx->cmd.m_command_number) <= 150)
        {
            engineprediction::get().FixNetvarCompression(cctx->cmd.m_command_number - 1);
        }

        auto shift_data = &engineprediction::get().m_Data[cctx->cmd.m_command_number % 150];

        const auto pre_change = player->m_nTickBase();

        if (cctx->cmd.m_command_number == (m_clientstate()->iCommandAck + 1) && g_ctx.globals.last_velocity_modifier >= 0.0f)
            player->m_flVelocityModifier() = g_ctx.globals.last_velocity_modifier;

        ((PhysicsSimulateFn)original_physicssimulate)(player);

        auto ndata = &engineprediction::get().m_Data[cctx->cmd.m_command_number % 150];

        ndata->m_nTickBase = player->m_nTickBase();
        ndata->command_number = cctx->cmd.m_command_number;
        ndata->m_aimPunchAngle = player->m_aimPunchAngle();
        ndata->m_aimPunchAngleVel = player->m_aimPunchAngleVel();
        ndata->m_viewPunchAngle = player->m_viewPunchAngle();
        ndata->m_vecViewOffset = player->m_vecViewOffset();
        ndata->m_vecViewOffset.z = fminf(fmaxf(ndata->m_vecViewOffset.z, 46.0f), 64.0f);
        ndata->m_vecVelocity = player->m_vecVelocity();
        ndata->m_vecOrigin = player->m_vecOrigin();
        ndata->m_flFallVelocity = player->m_flFallVelocity();
        ndata->m_flDuckAmount = player->m_flDuckAmount();
        ndata->m_flVelocityModifier = player->m_flVelocityModifier();
        ndata->tick_count = m_globals()->m_tickcount;
        ndata->is_filled = true;
    }
}

void __fastcall hooks::hooked_physicssimulate(player_t* player)
{
    return physicssimulate_detour(player);
}

_declspec(noinline)void hooks::modifyeyeposition_detour(c_baseplayeranimationstate* state, Vector& position)
{
    if (state && g_ctx.globals.in_createmove)
        return ((ModifyEyePositionFn)original_modifyeyeposition)(state, position);
}

void __fastcall hooks::hooked_modifyeyeposition(c_baseplayeranimationstate* state, void* edx, Vector& position)
{
    return modifyeyeposition_detour(state, position);
}

_declspec(noinline)void hooks::calcviewmodelbob_detour(player_t* player, Vector& position)
{
    if (!g_cfg.esp.removals[REMOVALS_LANDING_BOB] || player != g_ctx.local() || !g_ctx.local()->is_alive())
        return ((CalcViewmodelBobFn)original_calcviewmodelbob)(player, position);
}

void __fastcall hooks::hooked_calcviewmodelbob(player_t* player, void* edx, Vector& position)
{
    return calcviewmodelbob_detour(player, position);
}

bool __fastcall hooks::hooked_shouldskipanimframe()
{
    return false;
}

int hooks::processinterpolatedlist()
{
    static auto allow_extrapolation = *(bool**)(util::FindSignature(crypt_str("client.dll"), crypt_str("A2 ? ? ? ? 8B 45 E8")) + 0x1);

    if (allow_extrapolation)
        *allow_extrapolation = false;

    return ((ProcessInterpolatedListFn)original_processinterpolatedlist)();
}

Vector* _fastcall hooks::eye_angles(void* ecx, void* edx)
{
    return nullptr;
}

void __fastcall hooks::hkClampBonesInBB(player_t* player, void* edx, matrix3x4_t* matrix, int mask)
{
    if (g_ctx.globals.setuping_bones)
        ((ClampBonesInBBox_t)original_oClampBonesInBBox)(player, matrix, mask);
}
