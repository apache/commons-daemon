import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

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
 */
public class ProcrunService implements Runnable {

    private static final int DEFAULT_PAUSE = 60; // Wait 1 minute
    private static final long MS_PER_SEC = 1000L; // Milliseconds in a second

    private static volatile Thread thrd; // start and stop are called from different threads
    
    private final long pause; // How long to pause in service loop

    private ProcrunService(long wait){
        pause=wait;
    }

    private static void usage(){
        System.err.println("Must supply the argument 'start' or 'stop'");        
    }

    /**
     * Common entry point for start and stop service functions.
     * 
     * @param args [start [pause time] | stop]
     */
    public static void main(String[] args) {
        final int argc = args.length;
        log("ProcrunService called with "+argc+" arguments from thread: "+Thread.currentThread());
        for(int i=0; i < argc; i++) {
            System.out.println("["+i+"] "+args[i]);
        }
        String mode=null;
        if (argc > 0) {
            mode = args[0];
            if ("start".equals(mode)){
                long wait = DEFAULT_PAUSE;
                if (argc > 1) {
                    wait = Integer.valueOf(args[1]).intValue();
                }
                log("Starting the thread, wait(seconds): "+wait);
                thrd = new Thread(new ProcrunService(wait*MS_PER_SEC));
                thrd.start();
            } else 
            if ("stop".equals(mode)) {
                if (thrd != null) {
                    log("Interrupting the thread");
                    thrd.interrupt();
                }
            } else {
                usage();
            }
        } else {
            usage();
        }
    }
    
    /**
     * This method performs the work of the service.
     * In this case, it just logs a message every so often.
     */
    public void run() {
        log("Started thread in "+System.getProperty("user.dir"));
        while(true){
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
}
