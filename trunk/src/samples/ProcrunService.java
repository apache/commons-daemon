import java.io.File;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
//import java.util.Enumeration;
import java.util.Iterator;
//import java.util.List;
//import java.util.Map;
import java.util.Properties;
import java.util.TreeSet;
//import java.net.InterfaceAddress;
//import java.net.NetworkInterface;
//import java.net.SocketException;


/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/**
 * Sample service implementation for use with Windows Procrun.
 * <p>
 * Use the main() method for running as a Java (external) service.
 * Use the start() and stop() methods for running as a jvm (in-process) service
 */
public class ProcrunService implements Runnable {

    private static final int DEFAULT_PAUSE = 60; // Wait 1 minute
    private static final long MS_PER_SEC = 1000L; // Milliseconds in a second

    private static volatile Thread thrd; // start and stop are called from different threads

    private final long pause; // How long to pause in service loop

    private final File stopFile;

    /**
     *
     * @param wait seconds to wait in loop
     * @param filename optional filename - if non-null, run loop will stop when it disappears
     * @throws IOException
     */
    private ProcrunService(long wait, File file) {
        pause=wait;
        stopFile = file;
    }

    private static File tmpFile(String filename) {
        return new File(System.getProperty("java.io.tmpdir"),
                filename != null ? filename : "ProcrunService.tmp");
    }

    private static void usage(){
        System.err.println("Must supply the argument 'start' or 'stop'");
    }

    /**
     * Helper method for process args with defaults.
     *
     * @param args array of string arguments, may be empty
     * @param argnum which argument to extract
     * @return the argument or null
     */
    private static String getArg(String[] args, int argnum){
        if (args.length > argnum) {
            return args[argnum];
        } else {
            return null;
        }
    }

    private static void logSystemEnvironment()
    {
        ClassLoader cl = Thread.currentThread().getContextClassLoader();
        if (cl == null)
            log("Missing currentThread context ClassLoader");
        else
            log("Using context ClassLoader : " + cl.toString());
        log("Program environment: ");

// Java 1.5+ code
//        Map        em = System.getenv();
//        TreeSet    es = new TreeSet(em.keySet());
//        for (Iterator i = es.iterator(); i.hasNext();) {
//            String n = (String)i.next();
//            log(n + " ->  " + em.get(n));
//        }

        log("System properties: ");
        Properties ps = System.getProperties();
        TreeSet    ts = new TreeSet(ps.keySet());
        for (Iterator i = ts.iterator(); i.hasNext();) {
            String n = (String)i.next();
            log(n + " ->  " + ps.get(n));
        }

// Java 1.6+ code
//        log("Network interfaces: ");
//        log("LVPMU (L)oopback (V)irtual (P)ointToPoint (M)multicastSupport (U)p");
//        try {
//            for (Enumeration e = NetworkInterface.getNetworkInterfaces(); e.hasMoreElements();) {
//                NetworkInterface n = (NetworkInterface)e.nextElement();
//                char [] flags = { '-', '-', '-', '-', '-'};
//                if (n.isLoopback())
//                    flags[0] = 'x';
//                if (n.isVirtual())
//                    flags[1] = 'x';
//                if (n.isPointToPoint())
//                    flags[2] = 'x';
//                if (n.supportsMulticast())
//                    flags[3] = 'x';
//                if (n.isUp())
//                    flags[4] = 'x';
//                String neti = new String(flags) + "   " + n.getName() + "\t";
//                for (Enumeration i = n.getSubInterfaces(); i.hasMoreElements();) {
//                    NetworkInterface s = (NetworkInterface)i.nextElement();
//                    neti += " [" + s.getName() + "]";
//                }
//                log(neti + " -> " + n.getDisplayName());
//                List i = n.getInterfaceAddresses();
//                if (!i.isEmpty()) {
//                    for (int x = 0; x < i.size(); x++) {
//                        InterfaceAddress a = (InterfaceAddress)i.get(x);
//                        log("        " + a.toString());
//                    }
//                }
//            }
//        } catch (SocketException e) {
//            // Ignore
//        }
    }

    /**
     * Common entry point for start and stop service functions.
     * To allow for use with Java mode, a temporary file is created
     * by the start service, and a deleted by the stop service.
     *
     * @param args [start [pause time] | stop]
     * @throws IOException if there are problems creating or deleting the temporary file
     */
    public static void main(String[] args) throws IOException {
        final int argc = args.length;
        log("ProcrunService called with "+argc+" arguments from thread: "+Thread.currentThread());
        for(int i=0; i < argc; i++) {
            System.out.println("["+i+"] "+args[i]);
        }
        String mode=getArg(args, 0);
        if ("start".equals(mode)){
            File f = tmpFile(getArg(args, 2));
            log("Creating file: "+f.getPath());
            f.createNewFile();
            startThread(getArg(args, 1), f);
        } else if ("stop".equals(mode)) {
            final File tmpFile = tmpFile(getArg(args, 1));
            log("Deleting file: "+tmpFile.getPath());
            tmpFile.delete();
        } else {
            usage();
        }
    }

    /**
     * Start the jvm version of the service, and waits for it to complete.
     *
     * @param args optional, arg[0] = timeout (seconds)
     */
    public static void start(String [] args) {
        startThread(getArg(args, 0), null);
        while(thrd.isAlive()){
            try {
                thrd.join();
            } catch (InterruptedException ie){
                // Ignored
            }
        }
    }

    private static void startThread(String waitParam, File file) {
        long wait = DEFAULT_PAUSE;
        if (waitParam != null) {
            wait = Integer.valueOf(waitParam).intValue();
        }
        log("Starting the thread, wait(seconds): "+wait);
        thrd = new Thread(new ProcrunService(wait*MS_PER_SEC,file));
        thrd.start();
    }

    /**
     * Stop the JVM version of the service.
     *
     * @param args ignored
     */
    public static void stop(String [] args){
        if (thrd != null) {
            log("Interrupting the thread");
            thrd.interrupt();
        } else {
            log("No thread to interrupt");
        }
    }

    /**
     * This method performs the work of the service.
     * In this case, it just logs a message every so often.
     */
    public void run() {
        log("Started thread in "+System.getProperty("user.dir"));
        logSystemEnvironment();
        while(stopFile == null || stopFile.exists()){
            try {
                log("pausing...");
                Thread.sleep(pause);
            } catch (InterruptedException e) {
                log("Exitting");
                break;
            }
        }
    }

    private static void log(String msg){
        DateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss ");
        System.out.println(df.format(new Date())+msg);
    }

    protected void finalize(){
        log("Finalize called from thread "+Thread.currentThread());
    }
}
