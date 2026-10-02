namespace Nitemare3D
{
    public static class RemoteDoorRuntime
    {
        public const byte OpenCommand = RecoveredInteractionFacts.RemoteOpenCommand;
        public const byte CloseCommand = RecoveredInteractionFacts.RemoteCloseCommand;

        // Recovered DAT_1048_51A4. Each remote group owns one bit.
        public static byte LatchMask { get; private set; }

        public static void Reset()
        {
            LatchMask = 0;
        }

        public static bool IsGroupLatched(int group)
        {
            return group >= 0 &&
                   group < 8 &&
                   (LatchMask & (1 << group)) != 0;
        }

        public static void Dispatch(byte command, int group)
        {
            if ((command != OpenCommand && command != CloseCommand) ||
                group < 0 ||
                group >= 8)
            {
                return;
            }

            for (int x = 0; x < 64; x++)
            {
                for (int y = 0; y < 64; y++)
                {
                    if (!(Level.tilemap[x, y] is SlidingDoorTile door) ||
                        !door.IsRemoteControlled ||
                        door.CredentialGroup != group)
                    {
                        continue;
                    }

                    // Original command 0x1E selects only state 1/3.
                    if (command == OpenCommand)
                    {
                        if (door.RuntimeState == DoorRuntimeState.Closed ||
                            door.RuntimeState == DoorRuntimeState.Closing)
                        {
                            door.RemoteOpen();
                        }
                    }
                    // Original command 0x1F selects only state 0/2.
                    else if (door.RuntimeState == DoorRuntimeState.Open ||
                             door.RuntimeState == DoorRuntimeState.Opening)
                    {
                        door.RemoteClose();
                    }
                }
            }

            // FUN_1018_2Axx toggles the same DAT_51A4 group bit after dispatch.
            LatchMask ^= (byte)(1 << group);
        }
    }
}
