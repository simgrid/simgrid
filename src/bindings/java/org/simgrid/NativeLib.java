/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid;

import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;

public final class NativeLib {

	public static String getPath() {
		String prefix = "NATIVE";
		String os = System.getProperty("os.name");
		String arch = System.getProperty("os.arch");

		if (arch.matches("^i[3-6]86$"))
			arch = "x86";
		else if (arch.equalsIgnoreCase("amd64"))
			arch = "x86_64";

		if (os.toLowerCase().startsWith("win")){
			os = "Windows";
			arch = "x86";
		}else if (os.contains("OS X"))
			os = "Darwin";

		os = os.replace(' ', '_');
		arch = arch.replace(' ', '_');

		return prefix + "/" + os + "/" + arch + "/";
	}
	public static void nativeInit(String name) {
		try {
			/* Prefer the version of the library bundled into the jar file and use it */
			loadLib(name);
		} catch (SimGridLibNotFoundException e) {
			/* If not found, try to see if we can find a version on disk */
			try {
				System.loadLibrary(name);
			} catch (UnsatisfiedLinkError e2) {
				System.err.println("Cannot load the bindings to the "+name+" library in path "+getPath());
				e.printStackTrace();
				System.err.println("This jar file does not seem to fit your system, and I cannot find an installation of SimGrid.");
				System.exit(1);
			}
		}
	}

	private static void loadLib (String name) throws SimGridLibNotFoundException {
		String Path = NativeLib.getPath();

		String filename=name;
		InputStream in = NativeLib.class.getClassLoader().getResourceAsStream(Path+filename);

		if (in == null) {
			filename = "lib"+name+".so";
			in = NativeLib.class.getClassLoader().getResourceAsStream(Path+filename);
		}
		if (in == null) {
			filename = name+".dll";
			in =  NativeLib.class.getClassLoader().getResourceAsStream(Path+filename);
		}
		if (in == null) {
			filename = "lib"+name+".dll";
			in =  NativeLib.class.getClassLoader().getResourceAsStream(Path+filename);
		}
		if (in == null) {
			filename = "lib"+name+".dylib";
			in =  NativeLib.class.getClassLoader().getResourceAsStream(Path+filename);
		}
		if (in == null) {
			throw new SimGridLibNotFoundException("Cannot find library "+name+" in path "+Path+". Sorry, but this jar does not seem to be usable on your machine.");
		}
		try {
			// We must write the lib onto the disk before loading it -- stupid operating systems
			File fileOut = new File(filename);
			fileOut = File.createTempFile(name+"-", ".tmp");
			// don't leak the file on disk, but remove it on JVM shutdown
			Runtime.getRuntime().addShutdownHook(new Thread(new FileCleaner(fileOut.getAbsolutePath())));
			OutputStream out = new FileOutputStream(fileOut);

			/* copy the library in position */  
			byte[] buffer = new byte[4096]; 
			int bytes_read; 
			while ((bytes_read = in.read(buffer)) != -1)     // Read until EOF
				out.write(buffer, 0, bytes_read); 

			/* close all file descriptors, and load that shit */
			in.close();
			out.close();
			System.load(fileOut.getAbsolutePath());
		} catch (Exception e) {
			System.err.println("Error while extracting the native library from the jar: ");
			e.printStackTrace();
			throw new SimGridLibNotFoundException("Cannot load the bindings to the "+name+" library in path "+getPath(),   e);
		}
	}

	/* A hackish mechanism used to remove the file containing our library when the JVM shuts down */
	private static class FileCleaner implements Runnable {
		private String target;
		public FileCleaner(String name) {
			target = name;
		}
		public void run() {
			try {
				new File(target).delete();
			} catch(Exception e) {
				System.out.println("Unable to clean temporary file "+target+" during shutdown.");
				e.printStackTrace();
			}
		}    
	}


	public static void main(String[] args) {
		System.out.println("This jarfile searches the native code under: " +getPath());
	}
}

class SimGridLibNotFoundException extends Exception {
	private static final long serialVersionUID = 1L;
	public SimGridLibNotFoundException(String msg) {
		super(msg);
	}

	public SimGridLibNotFoundException(String msg, Exception e) {
		super(msg,e);
	}
}