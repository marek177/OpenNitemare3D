using static SFML.Window.Keyboard;
using SFML.Window;
using System;
using System.Collections.Generic;
namespace Nitemare3D
{
    //todo: implement more keys, should be easy to just automate at some point    
    public enum KeyboardKey
    {
        Enter = Key.Enter,
        Up = Key.Up,
        Down = Key.Down,
        Left = Key.Left,
        Right = Key.Right,
        Space = Key.Space,
        Num1 = Key.Num1,
        Num2 = Key.Num2,
        Num3 = Key.Num3,
        Num4 = Key.Num4,

        Backspace = Key.Backspace,

        LControl = Key.LControl,

        F1 = Key.F1,
        F2 = Key.F2,
        F3 = Key.F3,
        F4 = Key.F4,
        F5 = Key.F5
    }


    public static class Input
    {
        static int numberInput = -1;
        static readonly Dictionary<KeyboardKey, bool> previousKeyState = new Dictionary<KeyboardKey, bool>();

        const float textInputTime = .1f;
        static float textInputTimer = 0;

        public static string text = string.Empty;
        static void OnTextEnter(object sender, TextEventArgs e)
        {
            if (textInputTime > textInputTimer) { return; }
            numberInput = -1;
            int num;
            bool isNumber = int.TryParse(e.Unicode, out num);
            if (isNumber)
            {
                numberInput = num;
            }

            if (e.Unicode == "\b" || e.Unicode == "\n")
            {
                return;
            }

            text = e.Unicode;
        }

        //returns -1 if input is not a number
        public static int GetNumberInput()
        {
            return numberInput;
        }

        public static bool LeftClick()
        {
            return Mouse.IsButtonPressed(Mouse.Button.Left);
        }

        public static void Update()
        {
            // Program.cs calls Input.Update() at the end of each frame. Store the
            // final level state here so IsKeyPressed() can reproduce the original
            // rising-edge USE/ACTION behavior on the next frame.
            foreach (KeyboardKey key in Enum.GetValues(typeof(KeyboardKey)))
            {
                previousKeyState[key] = IsKeyDown(key);
            }

            textInputTimer += Time.dt;
            text = string.Empty;
            numberInput = -1;
        }

        public static void Init()
        {
            GameWindow.sfWindow.TextEntered += new EventHandler<TextEventArgs>(OnTextEnter);
        }

        public static bool IsKeyDown(KeyboardKey key)
        {
            // KeyboardKey mirrors SFML.Window.Keyboard.Key numerically, but C#
            // requires an explicit conversion between the two enum types.
            return Keyboard.IsKeyPressed((Key)(int)key) && GameWindow.sfWindow.HasFocus();
        }

        /// <summary>
        /// True only on the transition from up to down. NITE3W.EXE uses this
        /// rising-edge behavior for USE/ACTION (input mask 0x0200).
        /// </summary>
        public static bool IsKeyPressed(KeyboardKey key)
        {
            bool down = IsKeyDown(key);
            bool wasDown;
            previousKeyState.TryGetValue(key, out wasDown);
            return down && !wasDown;
        }
    }
}
