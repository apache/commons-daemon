/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/* @version $Id$ */

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Properties;

/*
 * That is like the ServiceDaemon but it does not use the Daemon interface.
 */
public class AloneService {

    private Properties prop = null;
    private Process proc[] = null;
    private ServiceDaemonReadThread readout[] = null;
    private ServiceDaemonReadThread readerr[] = null;

    protected void finalize() {
        System.err.println("ServiceDaemon: instance "+this.hashCode()+
                           " garbage collected");
    }

    /**
     * init and destroy were added in jakarta-tomcat-daemon.
     */
    public void init(String[] arguments)
    throws Exception {
        /* Set the err */
        System.setErr(new PrintStream(new FileOutputStream("/ServiceDaemon.err",true)));
        System.err.println("ServiceDaemon: instance "+this.hashCode()+
                           " init");

        /* read the properties file */
        prop = new Properties();
        try {
            prop.load(new FileInputStream("startfile"));
        }
        catch (Exception e) {
            // Cannot find startfile.properties.
            // XXX: Should we print something?
        }
        /* create an array to store the processes */
        int i=0;
        for (Enumeration e = prop.keys(); e.hasMoreElements() ;) {
            e.nextElement();
            i++;
        }
        System.err.println("ServiceDaemon: init for " + i + " processes");
        proc = new Process[i];
        readout = new ServiceDaemonReadThread[i];
        readerr = new ServiceDaemonReadThread[i];
        for (i=0;i<proc.length;i++) {
            proc[i] = null;
            readout[i] = null;
            readerr[i] = null;
        }

        System.err.println("ServiceDaemon: init done ");

    }

    public void start() {
        /* Dump a message */
        System.err.println("ServiceDaemon: starting");

        /* Start */
        int i=0;
        for (Enumeration e = prop.keys(); e.hasMoreElements() ;) {
           String name = (String) e.nextElement();
           System.err.println("ServiceDaemon: starting: " + name + " : " + prop.getProperty(name));
           try {
               proc[i] = Runtime.getRuntime().exec(prop.getProperty(name));
           } catch(Exception ex) {
               System.err.println("Exception: " + ex);
           }
           /* Start threads to read from Error and Out streams */
           readerr[i] =
               new ServiceDaemonReadThread(proc[i].getErrorStream());
           readout[i] =
               new ServiceDaemonReadThread(proc[i].getInputStream());
           readerr[i].start();
           readout[i].start();
           i++;
        }
    }

    public void stop() {
        /* Dump a message */
        System.err.println("ServiceDaemon: stopping");

        for (int i=0;i<proc.length;i++) {
            if ( proc[i]==null)
               continue;
            proc[i].destroy();
            try {
                proc[i].waitFor();
            } catch(InterruptedException ex) {
                System.err.println("ServiceDaemon: exception while stopping:" +
                                    ex);
            }
        }

        System.err.println("ServiceDaemon: stopped");
    }

    public void destroy() {
        System.err.println("ServiceDaemon: instance "+this.hashCode()+
                           " destroy");
    }

}
