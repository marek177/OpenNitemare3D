#include "g_think.h"
#include "n3d_re_object_defs.h"
#include "n3d_re_guard_sounds.h"
#include "n3d_re_guard_perception.h"

void G_HandleThinking()
{
    for(int i = 0; i < m_objcount; i++)
    {
        switch ((*m_objects[i]).type)
        {
        case M_PLAYER:
            P_PlayerThink();
            break;
        
        default:
            break;
        }
    }
}

void G_HandleRecoveredGuardThinking()
{
    /*
     * Only execute autonomous state behavior whose control flow is currently
     * closed by executable evidence. Other states deliberately remain untouched.
     */
    for(uint16_t i = 0; i < n3d_guard_count; ++i)
    {
        n3d_guard_record* guard = &n3d_guards[i];

        N3D_RE_UpdateGuardAreaForSlot(i);

        switch(guard->state)
        {
            case N3D_GUARD_STATE_00:
                if(guard->object_slot < n3d_object_count)
                {
                    N3D_RE_TickAnimationTimer(
                        guard,
                        &n3d_objects[guard->object_slot]);
                }
                break;

            case N3D_GUARD_STATE_01:
                N3D_RE_TickState01(guard);
                break;

            case N3D_GUARD_STATE_02:
                if(guard->object_slot < n3d_object_count)
                {
                    n3d_object_record* object =
                        &n3d_objects[guard->object_slot];

                    const n3d_object_definition_slot* definition =
                        N3D_RE_ObjectDefinition(
                            object->sequence_id);

                    if(definition)
                    {
                        uint16_t sequence = 0;
                        if(N3D_RE_ObjectDefinitionGuardStateSequence(
                               &definition->header,
                               N3D_GUARD_STATE_02,
                               &sequence))
                        {
                            const uint16_t random_value =
                                N3D_RE_GuardAlertUsesRandom(
                                    object->object_class)
                                    ? N3D_RE_RngNextGlobal()
                                    : 0;

                            /*
                             * Preserve exact RNG/SND selector semantics now;
                             * the old C shell audio bridge still needs a
                             * separate original-SND-index integration.
                             */
                            (void)N3D_RE_GuardAlertSoundId(
                                object->object_class,
                                random_value);

                            N3D_RE_BeginState02AlertSequence(
                                guard,
                                object,
                                sequence);
                        }
                    }
                }
                break;

            case N3D_GUARD_STATE_03:
                if(guard->object_slot < n3d_object_count)
                {
                    n3d_object_record* object =
                        &n3d_objects[guard->object_slot];

                    const n3d_object_definition_slot* definition =
                        N3D_RE_ObjectDefinition(
                            object->sequence_id);

                    if(definition)
                    {
                        /*
                         * FUN_7594 calls FUN_7494/D50A with secondary object
                         * checks enabled and facing/FOV bypassed.
                         */
                        const int perception_succeeded =
                            N3D_RE_EvaluateGuardPerceptionMap(
                                guard,
                                object,
                                n3d_player.world_x,
                                n3d_player.world_y,
                                1,
                                1);

                        int attack_eligible = 0;
                        if(N3D_RE_TryEvaluateGuardAttackGate(
                               guard,
                               object,
                               n3d_player.world_x,
                               n3d_player.world_y,
                               perception_succeeded,
                               &attack_eligible) &&
                           attack_eligible)
                        {
                            uint16_t sequence = 0;
                            if(N3D_RE_ObjectDefinitionGuardStateSequence(
                                   &definition->header,
                                   N3D_GUARD_STATE_03,
                                   &sequence))
                            {
                                N3D_RE_BeginState03AttackSequence(
                                    guard,
                                    object,
                                    sequence);
                            }
                        }

                        /*
                         * attack_eligible == 0 enters FUN_76FC movement planning
                         * in the original. Leave state 03 unchanged until that
                         * movement planner/collision fallback is ported exactly.
                         */
                    }
                }
                break;

            case N3D_GUARD_STATE_0A:
            case N3D_GUARD_STATE_LETHAL_CONTACT:
                /* no local dispatcher action */
                break;

            default:
                break;
        }
    }
}
