using System.IO;
using System;
using System.Collections.Generic;
namespace Nitemare3D
{
	public class Game : Scene
	{
		Pcx hud = new Pcx(Dat.Uif[ImageConsts.UI_HUD]);
		int[] music = new int[]
		{
			MidiConsts.MIDI_E1M1,
			MidiConsts.MIDI_INTERMISSION
		};
		public const string gameTitle = "Nitemare 3D 0.46";
		


		public static Player player;
		public static int episode = 1;
		public override void Load()
		{
			songid = music[1];
			pcx = hud;

			// IMG directories are episode-specific; reload the matching archive before
			// MAP wall IDs are resolved to their exact first-frame indices.
			new Img("data/IMG." + episode);

			player = Entity.Create<Player>();
			OriginalRandom.Reset(1);
			RemoteDoorRuntime.Reset();
			Automap.Reset();

			Level.LoadMap(level, episode);
		}

		public override void UnLoad()
		{
		}

		public static int level = 0;

		public override void Update()
		{
			songid = music[level];
			//Entity.UpdateEntites();
			//player.RenderRaycaster();
		}
	}
}