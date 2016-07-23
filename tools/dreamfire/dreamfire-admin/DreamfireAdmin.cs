using System;
using System.Windows.Forms;

namespace dreamfireadmin
{
	public class DreamfireAdmin : Form
	{
		Label title = new Label();

		public DreamfireAdmin ()
		{
			this.SuspendLayout ();
			this.title.AutoSize = true;
			this.title.Text = "Start a New Dreamfire Project";

			this.Controls.Add (title);
			this.ResumeLayout ();

			this.Name = "Dreamfire Admin";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Dreamfire Admin";
		}
	}

	public class Program {
		public static void Main(String [] args) {
			Application.EnableVisualStyles ();
			Application.SetCompatibleTextRenderingDefault (false);
			Application.Run (new DreamfireAdmin ());
		}
	}
}

