package com.tbd.SlashEM;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;

import java.io.File;

/**
 * Starts the game again after it has closed. The game closes its process when
 * a game ends; when the player picked "Play again", this activity runs in a
 * process of its own, waits for the old one to go, then opens the game.
 */
public class RestartActivity extends Activity
{
	static final String EXTRA_PID = "pid";

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		final int pid = getIntent().getIntExtra(EXTRA_PID, 0);
		final Handler handler = new Handler(Looper.getMainLooper());
		handler.post(new Runnable()
		{
			int tries = 0;

			@Override
			public void run()
			{
				// Wait (up to about three seconds) for the old game to close.
				if(pid != 0 && new File("/proc/" + pid).exists() && tries++ < 60)
				{
					handler.postDelayed(this, 50);
					return;
				}
				Intent i = new Intent(RestartActivity.this, SlashEM.class);
				i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
				startActivity(i);
				finish();
				android.os.Process.killProcess(android.os.Process.myPid());
			}
		});
	}
}
