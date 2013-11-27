package org.simgrid.msg;

public final class NativeLib {
    public static String getPath() {
        String prefix = "NATIVE";
        String os = System.getProperty("os.name");
        String arch = System.getProperty("os.arch");

        if (os.toLowerCase().startsWith("^win"))
            os = "Windows";
        else if (os.contains("OS X"))
            os = "Darwin";

        if (arch.matches("^i[3-6]86$"))
            arch = "x86";
        else if (arch.equalsIgnoreCase("amd64"))
            arch = "x86_64";

        os = os.replace(' ', '_');
        arch = arch.replace(' ', '_');

        return prefix + "/" + os + "/" + arch + "/";
    }

    public static void main(String[] args) {
        System.out.println(getPath());
    }
}
