#include "g_think.h"

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

        switch(guard->state)
        {
            case N3D_GUARD_STATE_01:
                N3D_RE_TickState01(guard);
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
