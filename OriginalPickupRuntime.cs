namespace Nitemare3D
{
    /// <summary>
    /// Executable-backed subset of FUN_1010_CF60 for object classes that are
    /// placed directly in MAP.N. Dynamic reward classes 0x31/0x32/0x34/0x35/
    /// 0x37/0x38 and scroll UI class 0x3D remain separate closures.
    /// </summary>
    public sealed class OriginalPickupRuntime
    {
        // DAT_1048_4C28 / 4C29.
        public byte KeyMask { get; private set; }
        public byte IdCardMask { get; private set; }

        // DAT_1048_4C42 / 4C43.
        public byte EnemyLocatorEnergy { get; private set; }
        public byte MapClarityEnergy { get; private set; }

        // DAT_1048_4C45.
        public byte PentagramMask { get; private set; }

        public void ResetNewGame()
        {
            KeyMask = 0;
            IdCardMask = 0;
            EnemyLocatorEnergy = 0;
            MapClarityEnergy = 0;
            PentagramMask = 0;
        }

        public static int PickupSoundEventId(byte objectClass)
        {
            switch (objectClass)
            {
                case 0x2F: // keys
                case 0x30: // ID cards
                case 0x3C: // pentagrams
                case 0x3D: // scrolls
                    return 0x31;

                case 0x33: // health potion
                    return 0x2F;

                case 0x36: // weapon
                    return 0x32;

                case 0x39: // ammo
                    return 0x34;

                case 0x3A: // crystal ball / enemy locator
                    return 0x2E;

                case 0x3B: // magic eye / map clarity
                    return 0x33;

                default:
                    return -1;
            }
        }

        public bool TryCollectDirectMapPickup(
            byte objectClass,
            byte variant,
            Player player,
            OriginalWeaponRuntime weapons)
        {
            switch (objectClass)
            {
                case 0x2F:
                    KeyMask = (byte)(KeyMask | (1 << (variant & 7)));
                    return true;

                case 0x30:
                    IdCardMask = (byte)(IdCardMask | (1 << (variant & 7)));
                    return true;

                case 0x33:
                {
                    if (player == null || player.health >= OriginalRuntime.PlayerMaxHealth)
                        return false;

                    int amount = 0x14 >> (variant & 0x0F);
                    if (amount <= 0)
                        amount = 1;

                    player.health += amount;
                    if (player.health > OriginalRuntime.PlayerMaxHealth)
                        player.health = OriginalRuntime.PlayerMaxHealth;
                    return true;
                }

                case 0x36:
                {
                    if (player == null || variant >= 4)
                        return false;

                    return player.AcquireWeapon(
                        (OriginalWeaponSelector)variant);
                }

                case 0x39:
                {
                    if (weapons == null)
                        return false;

                    OriginalWeaponSelector selector;
                    switch (variant)
                    {
                        case 0:
                            selector = OriginalWeaponSelector.SilverPistol;
                            break;
                        case 1:
                            selector = OriginalWeaponSelector.SingleShotLaser;
                            break;
                        case 2:
                            selector = OriginalWeaponSelector.MagicWand;
                            break;
                        default:
                            return false;
                    }

                    if (!weapons.AddAmmoPickup(selector))
                        return false;

                    // FUN_A9E0 adds 20 first; FUN_A3B6(7/8/9) then clamps to 100.
                    weapons.NormalizeAmmoCaps();
                    return true;
                }

                case 0x3A:
                    if (EnemyLocatorEnergy >= 100)
                        return false;
                    EnemyLocatorEnergy = (byte)System.Math.Min(
                        100, EnemyLocatorEnergy + 20);
                    return true;

                case 0x3B:
                    if (MapClarityEnergy >= 100)
                        return false;
                    MapClarityEnergy = (byte)System.Math.Min(
                        100, MapClarityEnergy + 20);
                    return true;

                case 0x3C:
                    PentagramMask =
                        (byte)(PentagramMask | (1 << (variant & 7)));
                    return true;

                default:
                    return false;
            }
        }
    }
}
