package com.tbd.SlashEM;

import android.content.Context;
import android.content.Intent;
import android.graphics.Insets;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;

import java.io.File;

import com.tbd.forkfront.ForkFront;

/**
 * The game activity. Adapts ForkFront to current Android versions without
 * changing the upstream ForkFront sources.
 */
public class SlashEM extends ForkFront
{
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		addRestartHook();

		// Apps targeting Android 15+ are drawn edge to edge, behind the status
		// bar, navigation bar and keyboard. Pad the game's root view so nothing
		// is hidden behind them.
		if(Build.VERSION.SDK_INT >= 35)
		{
			View root = findViewById(com.tbd.forkfront.R.id.base_frame);
			if(root != null)
			{
				root.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener()
				{
					@Override
					public WindowInsets onApplyWindowInsets(View v, WindowInsets insets)
					{
						Insets i = insets.getInsets(WindowInsets.Type.systemBars()
								| WindowInsets.Type.displayCutout()
								| WindowInsets.Type.ime());
						v.setPadding(i.left, i.top, i.right, i.bottom);
						return WindowInsets.CONSUMED;
					}
				});
				root.requestApplyInsets();
			}
		}
	}

	// Android 10+ stores the game files in app-specific storage, which needs no
	// permission. Skip ForkFront's storage permission prompt there; on Android
	// 13+ the request is always denied and ForkFront would close the app.
	@Override
	public void ensureReadWritePermissions(RequestExternalStorageResult result)
	{
		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
			result.onGranted();
		else
			super.ensureReadWritePermissions(result);
	}

	private static boolean sRestartHook;

	// The game closes the app when a game ends. If the player picked "Play
	// again" (or "New character", or "Hall"), the game leaves a marker file
	// behind; when it is there, open the game again as the app closes.
	private void addRestartHook()
	{
		if(sRestartHook)
			return;
		sRestartHook = true;
		final Context app = getApplicationContext();
		Runtime.getRuntime().addShutdownHook(new Thread()
		{
			@Override
			public void run()
			{
				if(!restartPending(app))
					return;
				try
				{
					Intent i = new Intent(app, RestartActivity.class);
					i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
					i.putExtra(RestartActivity.EXTRA_PID, android.os.Process.myPid());
					app.startActivity(i);
				}
				catch(Exception e)
				{
				}
			}
		});
	}

	private static boolean restartPending(Context app)
	{
		File ext = app.getExternalFilesDir(null);
		if(ext != null && new File(ext, "legacy.restart").exists())
			return true;
		return new File(app.getFilesDir(), "legacy.restart").exists();
	}
}
