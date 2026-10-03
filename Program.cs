using System;
using System.IO;
using System.Runtime.InteropServices;
using SFML.System;
namespace Nitemare3D
{
	class Program
	{

		[DllImport("X11")]
		extern public static int XInitThreads();
		static void Main(string[] args)
		{
			if (args.Length > 0 && args[0] == "--self-test")
			{
				RuntimeSelfTest.Run();
				return;
			}

			foreach (string arg in args)
			{
				// Recovered GUARD runtime is the production default now.
				// Keep an explicit legacy switch for A/B behavior comparison.
				if (arg == "--legacy-guards")
					OriginalRuntimeState.AutonomousGuardRuntimeEnabled = false;
				else if (arg == "--n3d-original-guards")
					OriginalRuntimeState.AutonomousGuardRuntimeEnabled = true;
			}


			XInitThreads();

			Dat.Load();


			GameWindow.Init();
			Input.Init();

			Img.LoadEpisode(1);

			Clock dt = new Clock();

			SoundEffect.LoadSounds();

			Scene.AddScene(new StartMenu());
			Scene.AddScene(new MainMenu());
			Scene.AddScene(new LevelSelect());
			Scene.AddScene(new SoundEditor());
			Scene.AddScene(new Game());

			Scene.Start();

			while (GameWindow.IsOpen())
			{
				MusicPlayer.Play(Scene.currentScene.songid);
				Time.dt = dt.Restart().AsSeconds();
				GameWindow.Clear();

				
				Level.Update();
				
				
				Entity.UpdateEntites();
				
				GameWindow.DrawPcx();
				GameWindow.DrawFrameBuffer();
				
				if (!Scene.currentScene.fading)
				{
					Scene.currentScene.Update();
				}
				

				Input.Update();

				Scene.currentScene.UpdateFading();
				GameWindow.Display();
			}




		}
	}
}
