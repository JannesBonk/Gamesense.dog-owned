#include "prediction_system.h"

void engineprediction::FixNetvarCompression(int time)
{
    // Retrieve a pointer to the netvars data for the given time
    auto netvarsData = &netvars_data[time % MULTIPLAYER_BACKUP];

    // Check if the data is valid and filled, and if the command number matches the given time
    if (!netvarsData || !netvarsData->m_bIsFilled || netvarsData->command_number != time || (time - netvarsData->command_number) > 150)
        return;

    // Calculate the delta values for several variables
    auto aimPunchAngleDelta = netvarsData->m_aimPunchAngle - g_ctx.local()->m_aimPunchAngle();
    auto aimPunchAngleVelDelta = netvarsData->m_aimPunchAngleVel - g_ctx.local()->m_aimPunchAngleVel();
    auto viewPunchAngleDelta = netvarsData->m_viewPunchAngle - g_ctx.local()->m_viewPunchAngle();
    auto viewOffsetDelta = netvarsData->m_vecViewOffset - g_ctx.local()->m_vecViewOffset();
    auto velocityDelta = netvarsData->m_vecVelocity - g_ctx.local()->m_vecVelocity();

    auto duckAmountDelta = netvarsData->m_flDuckAmount - g_ctx.local()->m_flDuckAmount();
    auto fallVelocityDelta = netvarsData->m_flFallVelocity - g_ctx.local()->m_flFallVelocity();
    auto velocityModifierDelta = netvarsData->m_flVelocityModifier - g_ctx.local()->m_flVelocityModifier();

    // Check if the absolute value of any of the delta values is less than or equal to a certain threshold
    if (std::fabs(aimPunchAngleDelta.x) <= 0.03125f && std::fabs(aimPunchAngleDelta.y) <= 0.03125f && std::fabs(aimPunchAngleDelta.z) <= 0.03125f)
        g_ctx.local()->m_aimPunchAngle() = netvarsData->m_aimPunchAngle;

    if (std::fabs(aimPunchAngleVelDelta.x) <= 0.03125f && std::fabs(aimPunchAngleVelDelta.y) <= 0.03125f && std::fabs(aimPunchAngleVelDelta.z) <= 0.03125f)
        g_ctx.local()->m_aimPunchAngleVel() = netvarsData->m_aimPunchAngleVel;

    if (std::fabs(viewPunchAngleDelta.x) <= 0.03125f && std::fabs(viewPunchAngleDelta.y) <= 0.03125f && std::fabs(viewPunchAngleDelta.z) <= 0.03125f)
        g_ctx.local()->m_viewPunchAngle() = netvarsData->m_viewPunchAngle;

    if (std::fabs(viewOffsetDelta.x) <= 0.03125f && std::fabs(viewOffsetDelta.y) <= 0.03125f && std::fabs(viewOffsetDelta.z) <= 0.03125f)
        g_ctx.local()->m_vecViewOffset() = netvarsData->m_vecViewOffset;

    if (std::fabs(velocityDelta.x) <= 0.03125f && std::fabs(velocityDelta.y) <= 0.03125f && std::fabs(velocityDelta.z) <= 0.03125f)
        g_ctx.local()->m_vecVelocity() = netvarsData->m_vecVelocity;

    if (std::fabs(duckAmountDelta) <= 0.03125f)
        g_ctx.local()->m_flDuckAmount() = netvarsData->m_flDuckAmount;

    if (std::fabs(fallVelocityDelta) <= 0.03125f)
        g_ctx.local()->m_flFallVelocity() = netvarsData->m_flFallVelocity;

    if (std::fabs(velocityModifierDelta) <= 0.00625f)
        g_ctx.local()->m_flVelocityModifier() = netvarsData->m_flVelocityModifier;
}

void engineprediction::store_netvars()
{
    auto data = &netvars_data[m_clientstate()->iCommandAck % MULTIPLAYER_BACKUP];

    data->tickbase = g_ctx.local()->m_nTickBase();
    data->m_aimPunchAngle = g_ctx.local()->m_aimPunchAngle();
    data->m_aimPunchAngleVel = g_ctx.local()->m_aimPunchAngleVel();
    data->m_viewPunchAngle = g_ctx.local()->m_viewPunchAngle();
    data->m_vecViewOffset = g_ctx.local()->m_vecViewOffset();
    data->m_duckAmount = g_ctx.local()->m_flDuckAmount();
    data->m_duckSpeed = g_ctx.local()->m_flDuckSpeed();
}

void engineprediction::store_data()
{
    int          tickbase;
    StoredData_t* data;

    if (!g_ctx.local() && !g_ctx.local()->is_alive()) {
        reset_data();
        return;
    }

    tickbase = g_ctx.local()->m_nTickBase();

    data = &m_data[tickbase % MULTIPLAYER_BACKUP];

    data->m_tickbase = tickbase;
    data->m_punch = g_ctx.local()->m_aimPunchAngle();
    data->m_punch_vel = g_ctx.local()->m_aimPunchAngleVel();
    data->m_view_offset = g_ctx.local()->m_vecViewOffset();
    data->m_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
}

void engineprediction::reset_data()
{
    m_data.fill(StoredData_t());
}

void engineprediction::update_incoming_sequences()
{
    if (!m_clientstate()->pNetChannel)
        return;

    if (m_sequence.empty() || m_clientstate()->pNetChannel->m_nInSequenceNr > m_sequence.front().m_seq) {
        m_sequence.emplace_front(m_globals()->m_realtime, m_clientstate()->pNetChannel->m_nInReliableState, m_clientstate()->pNetChannel->m_nInSequenceNr);
    }

    while (m_sequence.size() > 2048)
        m_sequence.pop_back();
}

void engineprediction::update_vel()
{
    static int m_iLastCmdAck = 0;
    static float m_flNextCmdTime = 0.f;

    if (m_clientstate() && (m_iLastCmdAck != m_clientstate()->nLastCommandAck || m_flNextCmdTime != m_clientstate()->flNextCmdTime))
    {
        if (g_ctx.globals.last_velocity_modifier != g_ctx.local()->m_flVelocityModifier())
        {
            *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(m_prediction() + 0x24)) = 1;
            g_ctx.globals.last_velocity_modifier = g_ctx.local()->m_flVelocityModifier();
        }

        m_iLastCmdAck = m_clientstate()->nLastCommandAck;
        m_flNextCmdTime = m_clientstate()->flNextCmdTime;
    }
}

void engineprediction::setup()
{
    if (prediction_data.prediction_stage != SETUP)
        return;

    backup_data.flags = g_ctx.local()->m_fFlags(); //-V807
    backup_data.velocity = g_ctx.local()->m_vecVelocity();

    prediction_data.old_curtime = m_globals()->m_curtime; //-V807
    prediction_data.old_frametime = m_globals()->m_frametime;

    m_prediction()->InPrediction = true;
    m_prediction()->IsFirstTimePredicted = false;

    m_globals()->m_curtime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
    m_globals()->m_frametime = m_prediction()->EnginePaused ? 0.0f : m_globals()->m_intervalpertick;

    prediction_data.prediction_stage = PREDICT;
}

void engineprediction::predict(CUserCmd* userCommand) {
    // Calculate the unpred_vel variable
    static auto oldOrigin = g_ctx.local()->m_vecOrigin();
    auto unpredVel = (g_ctx.local()->m_vecOrigin() - oldOrigin) * (1.0 / m_globals()->m_intervalpertick);
    oldOrigin = g_ctx.local()->m_vecOrigin();

    // Check if the prediction_random_seed and prediction_player variables are null
    if (!prediction_data.prediction_random_seed)
        prediction_data.prediction_random_seed = *reinterpret_cast <int**> (util::FindSignature(crypt_str("client.dll"), crypt_str("A3 ? ? ? ? 66 0F 6E 86")) + 0x1);

    *prediction_data.prediction_random_seed = MD5_PseudoRandom(userCommand->m_command_number) & INT_MAX;

    if (!prediction_data.prediction_player)
        prediction_data.prediction_player = *reinterpret_cast <int**> (util::FindSignature(crypt_str("client.dll"), crypt_str("89 35 ? ? ? ? F3 0F 10 48")) + 0x2);

    *prediction_data.prediction_player = reinterpret_cast <int> (g_ctx.local());

    // Backup the values of the curtime and frametime variables
    prediction_data.old_curtime = m_globals()->m_curtime;
    prediction_data.old_frametime = m_globals()->m_frametime;

    // Set the values of the curtime and frametime variables
    m_globals()->m_curtime = TICKS_TO_TIME(g_ctx.globals.fixed_tickbase);
    m_globals()->m_frametime = m_prediction()->EnginePaused ? 0.0f : m_globals()->m_intervalpertick;

    // Perform prediction on the movement of the local player
    CMoveData moveData;
    memset(&moveData, 0, sizeof(CMoveData));
    m_gamemovement()->StartTrackPredictionErrors(g_ctx.local());
    m_movehelper()->set_host(g_ctx.local());
    m_prediction()->SetupMove(g_ctx.local(), userCommand, m_movehelper(), &moveData);
    m_prediction()->FinishMove(g_ctx.local(), userCommand, &moveData);

    // Calculate the pred_vel variable
    static auto predOldOrigin = g_ctx.local()->m_vecOrigin();
    auto predVel = (g_ctx.local()->m_vecOrigin() - predOldOrigin) * (1.0 / m_globals()->m_intervalpertick);
    predOldOrigin = g_ctx.local()->m_vecOrigin();

    // Set the prediction_stage variable to FINISH
    prediction_data.prediction_stage = FINISH;
}



void engineprediction::finish()
{
    if (prediction_data.prediction_stage != FINISH)
        return;

    m_gamemovement()->StartTrackPredictionErrors(g_ctx.local());
    m_movehelper()->set_host(g_ctx.local());

    *prediction_data.prediction_random_seed = -1;
    *prediction_data.prediction_player = 0;

    m_globals()->m_curtime = prediction_data.old_curtime;
    m_globals()->m_frametime = prediction_data.old_frametime;
}