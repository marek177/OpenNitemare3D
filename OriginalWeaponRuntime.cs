namespace Nitemare3D
{
    /// <summary>
    /// Recovered player weapon state shared by Win16/DOS gameplay:
    /// four selectors, three ammunition pools and the slow-tick fire-attempt gate.
    /// Shot acceptance (jam/ammo/projectile-slot) stays separate from cadence.
    /// </summary>
    public sealed class OriginalWeaponRuntime
    {
        static readonly byte[] FireThresholds = { 2, 1, 3, 1 };

        public byte PistolAmmo { get; private set; }
        public byte PlasmaAmmo { get; private set; }
        public byte WandAmmo { get; private set; }
        public byte OwnedWeaponsMask { get; private set; }
        public OriginalWeaponSelector ActiveSelector { get; private set; } =
            OriginalWeaponSelector.None;
        public byte CadenceCounter { get; private set; }

        // DAT_4BE5 / DOS 4151. Ordinary ammo consumption is bypassed while set.
        public bool Omnipotent { get; set; }

        // DAT_4C2E is a separately recovered scripted weapon lock/jam latch.
        // Its producer is not yet wired here, but the shot-acceptance layer can
        // already preserve the original separation from the cadence gate.
        public bool Jammed { get; set; }

        public void ResetNewGame()
        {
            PistolAmmo = 0;
            PlasmaAmmo = 0;
            WandAmmo = 0;
            OwnedWeaponsMask = 0;
            ActiveSelector = OriginalWeaponSelector.None;
            CadenceCounter = 0;
            Omnipotent = false;
            Jammed = false;
        }

        public void AdvanceSlowTick()
        {
            if (CadenceCounter < byte.MaxValue)
                CadenceCounter++;
        }

        public static byte FireThreshold(OriginalWeaponSelector selector)
        {
            int index = (int)selector;
            return index >= 0 && index < FireThresholds.Length
                ? FireThresholds[index]
                : byte.MaxValue;
        }

        public bool TryAcceptFireAttempt(
            OriginalWeaponSelector selector,
            bool fireEdge,
            bool fireHeld)
        {
            int index = (int)selector;
            if (index < 0 || index >= FireThresholds.Length)
                return false;

            // Selector 3 is the only recovered automatic/held-FIRE path.
            bool triggerAccepted =
                selector == OriginalWeaponSelector.ContinuousLaser
                    ? fireHeld
                    : fireEdge;

            if (!triggerAccepted ||
                CadenceCounter < FireThresholds[index])
            {
                return false;
            }

            // AA90 resets the attempt counter before later shot-acceptance
            // checks. A jam, zero ammo or full projectile pool therefore still
            // consumes the ready cadence interval.
            CadenceCounter = 0;
            return true;
        }

        public bool HasWeapon(OriginalWeaponSelector selector)
        {
            int index = (int)selector;
            return index >= 0 && index < 4 &&
                   (OwnedWeaponsMask & (1 << index)) != 0;
        }

        public bool TrySelect(OriginalWeaponSelector selector)
        {
            if (!HasWeapon(selector))
                return false;

            ActiveSelector = selector;
            return true;
        }

        public void GrantWeapon(OriginalWeaponSelector selector)
        {
            int index = (int)selector;
            if (index < 0 || index >= 4)
                return;

            OwnedWeaponsMask =
                (byte)(OwnedWeaponsMask | (1 << index));
            ActiveSelector = selector;

            // A9E0 initialize mode writes 50, it does not add 50.
            SetAmmo(selector, 50);
        }

        public int GetAmmo(OriginalWeaponSelector selector)
        {
            switch (selector)
            {
                case OriginalWeaponSelector.SilverPistol:
                    return PistolAmmo;

                case OriginalWeaponSelector.MagicWand:
                    return WandAmmo;

                case OriginalWeaponSelector.SingleShotLaser:
                case OriginalWeaponSelector.ContinuousLaser:
                    return PlasmaAmmo;

                default:
                    return 0;
            }
        }

        public void SetAmmo(
            OriginalWeaponSelector selector,
            int value)
        {
            byte stored = unchecked((byte)value);

            switch (selector)
            {
                case OriginalWeaponSelector.SilverPistol:
                    PistolAmmo = stored;
                    break;

                case OriginalWeaponSelector.MagicWand:
                    WandAmmo = stored;
                    break;

                case OriginalWeaponSelector.SingleShotLaser:
                case OriginalWeaponSelector.ContinuousLaser:
                    PlasmaAmmo = stored;
                    break;
            }
        }

        public bool CanAcceptShot(OriginalWeaponSelector selector)
        {
            return !Jammed &&
                   (Omnipotent || GetAmmo(selector) != 0);
        }

        public bool ConsumeAmmo(OriginalWeaponSelector selector)
        {
            if (Jammed)
                return false;

            if (Omnipotent)
                return true;

            switch (selector)
            {
                case OriginalWeaponSelector.SilverPistol:
                    if (PistolAmmo == 0) return false;
                    PistolAmmo--;
                    return true;

                case OriginalWeaponSelector.MagicWand:
                    if (WandAmmo == 0) return false;
                    WandAmmo--;
                    return true;

                case OriginalWeaponSelector.SingleShotLaser:
                case OriginalWeaponSelector.ContinuousLaser:
                    if (PlasmaAmmo == 0) return false;
                    PlasmaAmmo--;
                    return true;

                default:
                    return false;
            }
        }

        public bool AddAmmoPickup(OriginalWeaponSelector selector)
        {
            int current = GetAmmo(selector);
            if (current >= OriginalRuntime.NormalAmmoCap)
                return false;

            // The original adds first and lets the later HUD/state normalization
            // clamp values such as 99+20. Preserve that transient ordering.
            SetAmmo(selector, current + 20);
            return true;
        }

        public void NormalizeAmmoCaps()
        {
            if (PistolAmmo > OriginalRuntime.NormalAmmoCap)
                PistolAmmo = OriginalRuntime.NormalAmmoCap;
            if (PlasmaAmmo > OriginalRuntime.NormalAmmoCap)
                PlasmaAmmo = OriginalRuntime.NormalAmmoCap;
            if (WandAmmo > OriginalRuntime.NormalAmmoCap)
                WandAmmo = OriginalRuntime.NormalAmmoCap;
        }
    }
}
