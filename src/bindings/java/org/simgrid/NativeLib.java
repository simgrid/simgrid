/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

package org.simgrid;

import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;

public final class NativeLib {
	/* Statically load the library which contains all native functions used in here */
	static private boolean isNativeInited = false;
	public static void nativeInit() {
		if (isNativeInited)
			return;

		if (System.getProperty("os.name").toLowerCase().startsWith("win"))
			NativeLib.nativeInit("winpthread-1");

		NativeLib.nativeInit("simgrid");
		NativeLib.nativeInit("simgrid-java");      
		isNativeInited = true;
	}

	public static void nativeInit(String name) {
		try {
			/* Prefer the version of the library bundled into the jar file and use it */
			loadLib(name);
		} catch (LinkageException embeededException) {
			/* If not found, try to see if we can find a version on disk */
			try {
				System.loadLibrary(name);
			} catch (UnsatisfiedLinkError systemException) {
				if (! name.equals("boost_context")) { // Ignore when we cannot load boost_context
					
					System.err.println("\nCannot load the bindings to the "+name+" library in path "+getPath());
					Throwable cause = embeededException.getCause();
					if (cause instanceof java.lang.UnsatisfiedLinkError) {
						if (cause.getMessage().matches(".*libcgraph.so.*"))
							System.err.println("HINT: Try to install the libcgraph package (sudo apt-get install libcgraph).");
						else if (cause.getMessage().matches(".*libboost_context.so.*"))
							System.err.println("HINT: Try to install the boost-context package (sudo apt-get install libboost-context-dev).");
						else
							System.err.println("Try to install the missing dependencies, which name should appear above.");							
					} else {
						System.err.println("This jar file does not seem to fit your system, and no usable SimGrid installation found on disk.");
					}
					System.err.println();
					cause.printStackTrace();
					System.exit(1);
				}
			}
		}
	}

	public static String getPath() {
		// Inspiration: https://github.com/xerial/snappy-java/blob/develop/src/main/java/org/xerial/snappy/OSInfo.java
		String prefix = "NATIVE";
		String os = System.getProperty("os.name");
		String arch = System.getProperty("os.arch");

		if (arch.matches("^i[3-6]86$"))
			arch = "x86";
		else if (arch.equalsIgnoreCase("x86_64"))
			arch = "amd64";
		else if (arch.equalsIgnoreCase("AMD64"))
			arch = "amd64";

		if (os.toLowerCase().startsWith("win")){
			os = "Windows";
		} else if (os.contains("OS X"))
			os = "Darwin";

		os = os.replace(' ', '_');
		arch = arch.replace(' ', '_');

		return prefix + "/" + os + "/" + arch + "/";
	}
	static Path tempDir = null;
	private static void loadLib (String name) throws LinkageException {
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
			throw new LinkageException("Cannot find library "+name+" in path "+Path+". Sorry, but this jar does not seem to be usable on your machine.");
		}
		try {
			// We must write the lib onto the disk before loading it -- stupid operating systems
			if (tempDir == null) {
				tempDir = Files.createTempDirectory("simgrid-java-");
				// don't leak the files on disk, but remove it on JVM shutdown
				Runtime.getRuntime().addShutdownHook(new Thread(new FileCleaner(tempDir.toFile())));
			}
			File fileOut = new File(tempDir.toFile().getAbsolutePath() + File.separator + filename);

			/* copy the library in position */  
			OutputStream out = new FileOutputStream(fileOut);
			byte[] buffer = new byte[4096]; 
			int bytes_read; 
			while ((bytes_read = in.read(buffer)) != -1)     // Read until EOF
				out.write(buffer, 0, bytes_read); 

			/* close all file descriptors, and load that shit */
			in.close();
			out.close();
			System.load(fileOut.getAbsolutePath());
		} catch (SecurityException|UnsatisfiedLinkError|IOException e) {
			throw new LinkageException("Cannot load the bindings to the "+name+" library in path "+getPath(), e);
		}
	}

	/* A hackish mechanism used to remove the file containing our library when the JVM shuts down */
	private static class FileCleaner implements Runnable {
		private File dir;
		public FileCleaner(File dir) {
			this.dir = dir;
		}
		@Override
		public void run() {
			try {
				for (File f : dir.listFiles())
					if (! f.delete() )
						System.err.println("Unable to clean temporary file "+f.getAbsolutePath()+" during shutdown.");
			    if (! dir.delete() )
					System.err.println("Unable to clean temporary file "+dir.getAbsolutePath()+" during shutdown.");			    	
			} catch(Exception e) {
				System.err.println("Unable to clean temporary file "+dir.getAbsolutePath()+" during shutdown: "+e.getCause());
				e.printStackTrace();
			}
		}    
	}


	public static void main(String[] args) {
		System.out.println("This jarfile searches the native code under: " +getPath());
	}
}

class LinkageException extends Exception {
	private static final long serialVersionUID = 1L;
	public LinkageException(String msg) {
		super(msg);
	}

	public LinkageException(String msg, Throwable e) {
		super(msg,e);
	}
}
