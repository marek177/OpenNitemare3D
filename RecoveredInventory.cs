namespace Nitemare3D
{
    // Port of src/game/InventoryRuntime.hpp, NITE3W inventory byte masks.
    public static class RecoveredInventory
    {
        public static bool HasBit(byte mask, int bit) =>
            bit >= 0 && bit < 8 && (mask & (1 << bit)) != 0;

        public static byte GrantBit(byte mask, int bit) =>
            bit >= 0 && bit < 8 ? (byte)(mask | (1 << bit)) : mask;

        public static bool HasSecretPanelCredential(byte cards) => HasBit(cards, 0);
        public static bool HasAllPentagrams(byte mask) => (mask & 0x0F) == 0x0F;
        public static bool CanUseKeyWarp(byte wallClass, byte keys) =>
            wallClass >= 0x19 && wallClass <= 0x1C && HasBit(keys, wallClass - 0x19);
    }
}
