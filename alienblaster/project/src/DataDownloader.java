// This string is autogenerated by ChangeAppSettings.sh, do not change spaces amount
package de.schwardtnet.alienblaster;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.view.Window;
import android.view.WindowManager;

import android.widget.TextView;
import org.apache.http.client.methods.*;
import org.apache.http.*;
import org.apache.http.impl.*;
import org.apache.http.impl.client.*;
import java.util.zip.*;
import java.io.*;



class DataDownloader extends Thread
{
	class StatusWriter
	{
		private TextView Status;
		private MainActivity Parent;

		public StatusWriter( TextView _Status, MainActivity _Parent )
		{
			Status = _Status;
			Parent = _Parent;
		}
		
		public void setText(final String str)
		{
			class Callback implements Runnable
			{
				public TextView Status;
				public String text;
				public void run()
				{
					Status.setText(text);
				}
			}
			Callback cb = new Callback();
			cb.text = new String(str);
			cb.Status = Status;
			Parent.runOnUiThread(cb);
		}
		
	}
	public DataDownloader( MainActivity _Parent, TextView _Status )
	{
		Parent = _Parent;
		DownloadComplete = false;
		Status = new StatusWriter( _Status, _Parent );
		Status.setText( "Connecting to " + Globals.DataDownloadUrl );
		this.start();
	}

	@Override
	public void run() 
	{
	
		String path = getOutFilePath("DownloadFinished.flag");
		InputStream checkFile = null;
		try {
			checkFile = new FileInputStream( path );
		} catch( FileNotFoundException e ) {
		} catch( SecurityException e ) { };
		if( checkFile != null )
		{
			Status.setText( "No need to download" );
			DownloadComplete = true;
			initParent();
			return;
		}
		checkFile = null;
		
		// Create output directory
		if( Globals.DownloadToSdcard )
		{
			try {
				(new File( "/sdcard/" + Globals.ApplicationName )).mkdirs();
			} catch( SecurityException e ) { };
		}
		else
		{
			try {
				FileOutputStream dummy = Parent.openFileOutput( "dummy", Parent.MODE_WORLD_READABLE );
				dummy.write(0);
				dummy.flush();
			} catch( FileNotFoundException e ) {
			} catch( java.io.IOException e ) {};
		}
		
		HttpGet request = new HttpGet(Globals.DataDownloadUrl);
		request.addHeader("Accept", "*/*");
		HttpResponse response = null;
		try {
			DefaultHttpClient client = new DefaultHttpClient();
			client.getParams().setBooleanParameter("http.protocol.handle-redirects", true);
			response = client.execute(request);
		} catch (IOException e) { } ;
		if( response == null )
		{
			Status.setText( "Error connecting to " + Globals.DataDownloadUrl );
			return;
		}

		Status.setText( "Downloading data from " + Globals.DataDownloadUrl );
		
		ZipInputStream zip = null;
		try {
			zip = new ZipInputStream(response.getEntity().getContent());
		} catch( java.io.IOException e ) {
			Status.setText( "Error downloading data from " + Globals.DataDownloadUrl );
			return;
		}
		
		byte[] buf = new byte[1024];
		
		ZipEntry entry = null;

		while(true)
		{
			entry = null;
			try {
				entry = zip.getNextEntry();
			} catch( java.io.IOException e ) {
				Status.setText( "Error downloading data from " + Globals.DataDownloadUrl );
				return;
			}
			if( entry == null )
				break;
			if( entry.isDirectory() )
			{
				try {
					(new File( getOutFilePath(entry.getName()) )).mkdirs();
				} catch( SecurityException e ) { };
				continue;
			}
			
			OutputStream out = null;
			path = getOutFilePath(entry.getName());
			
			try {
				out = new FileOutputStream( path );
			} catch( FileNotFoundException e ) {
			} catch( SecurityException e ) { };
			if( out == null )
			{
				Status.setText( "Error writing to " + path );
				return;
			}

			Status.setText( "Writing file " + path );

			try {
				int len;
				while ((len = zip.read(buf)) > 0)
				{
					out.write(buf, 0, len);
				}
				out.flush();
			} catch( java.io.IOException e ) {
				Status.setText( "Error writing file " + path );
				return;
			}

		}

		OutputStream out = null;
		path = getOutFilePath("DownloadFinished.flag");
		try {
			out = new FileOutputStream( path );
			out.write(0);
			out.flush();
		} catch( FileNotFoundException e ) {
		} catch( SecurityException e ) {
		} catch( java.io.IOException e ) {
			Status.setText( "Error writing file " + path );
			return;
		};
		
		if( out == null )
		{
			Status.setText( "Error writing to " + path );
			return;
		}
	
		Status.setText( "Finished" );
		DownloadComplete = true;
		
		initParent();
	};
	
	private void initParent()
	{
		class Callback implements Runnable
		{
			public MainActivity Parent;
			public void run()
			{
					Parent.initSDL();
			}
		}
		Callback cb = new Callback();
		cb.Parent = Parent;
		Parent.runOnUiThread(cb);
	}
	
	private String getOutFilePath(final String filename)
	{
		if( Globals.DownloadToSdcard )
			return	"/sdcard/" + Globals.ApplicationName + "/" + filename;
		return Parent.getFilesDir().getAbsolutePath() + "/" + filename;
	};
	
	public boolean DownloadComplete;
	public StatusWriter Status;
	private MainActivity Parent;
}
