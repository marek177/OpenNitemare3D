namespace Nitemare3D
{
    // Port of PlayerHealthRuntime.hpp; enemy receiver 3:8C09, not hazard receiver.
    // Value semantics only: callers still own death sounds/animation and item removal.
    public enum EnemyDamageResult { Omnipotent, AlreadyDead, NonLethal, Lethal }
    public sealed class RecoveredPlayerHealth
    {
        public byte Value = 100;
        public ushort GameState;
        public bool Omnipotent;

        public EnemyDamageResult ApplyEnemyDamage(byte damage)
        {
            if (Omnipotent) return EnemyDamageResult.Omnipotent;
            if (GameState == 2) return EnemyDamageResult.AlreadyDead;
            if (damage >= Value)
            {
                Value = 0;
                GameState = 2;
                return EnemyDamageResult.Lethal;
            }
            Value -= damage;
            return EnemyDamageResult.NonLethal;
        }

        public byte ClampForHud()
        {
            if (Value > 100) Value = 100;
            return Value;
        }

        // Item names are still unbound in the evidence; do not map potion colors here.
        public bool ApplyFixedPickup(byte amount)
        {
            if ((amount != 20 && amount != 30) || Value >= 100) return false;
            Value += amount;
            return true;
        }
        public void RestoreTo100() => Value = 100;
    }
}
