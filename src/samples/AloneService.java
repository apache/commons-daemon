/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *          Copyright (c) 1999-2001 The Apache Software Foundation.          *
 *                           All rights reserved.                            *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * Redistribution and use in source and binary forms,  with or without modi- *
 * fication, are permitted provided that the following conditions are met:   *
 *                                                                           *
 * 1. Redistributions of source code  must retain the above copyright notice *
 *    notice, this list of conditions and the following disclaimer.          *
 *                                                                           *
 * 2. Redistributions  in binary  form  must  reproduce the  above copyright *
 *    notice,  this list of conditions  and the following  disclaimer in the *
 *    documentation and/or other materials provided with the distribution.   *
 *                                                                           *
 * 3. The end-user documentation  included with the redistribution,  if any, *
 *    must include the following acknowlegement:                             *
 *                                                                           *
 *       "This product includes  software developed  by the Apache  Software *
 *        Foundation <http://www.apache.org/>."                              *
 *                                                                           *
 *    Alternately, this acknowlegement may appear in the software itself, if *
 *    and wherever such third-party acknowlegements normally appear.         *
 *                                                                           *
 * 4. The names  "The  Jakarta  Project",  "WebApp",  and  "Apache  Software *
 *    Foundation"  must not be used  to endorse or promote  products derived *
 *    from this  software without  prior  written  permission.  For  written *
 *    permission, please contact <apache@apache.org>.                        *
 *                                                                           *
 * 5. Products derived from this software may not be called "Apache" nor may *
 *    "Apache" appear in their names without prior written permission of the *
 *    Apache Software Foundation.                                            *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES *
 * INCLUDING, BUT NOT LIMITED TO,  THE IMPLIED WARRANTIES OF MERCHANTABILITY *
 * AND FITNESS FOR  A PARTICULAR PURPOSE  ARE DISCLAIMED.  IN NO EVENT SHALL *
 * THE APACHE  SOFTWARE  FOUNDATION OR  ITS CONTRIBUTORS  BE LIABLE  FOR ANY *
 * DIRECT,  INDIRECT,   INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL *
 * DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE GOODS *
 * OR SERVICES;  LOSS OF USE,  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION) *
 * HOWEVER CAUSED AND  ON ANY  THEORY  OF  LIABILITY,  WHETHER IN  CONTRACT, *
 * STRICT LIABILITY, OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN *
 * ANY  WAY  OUT OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF  ADVISED  OF THE *
 * POSSIBILITY OF SUCH DAMAGE.                                               *
 *                                                                           *
 * ========================================================================= *
 *                                                                           *
 * This software  consists of voluntary  contributions made  by many indivi- *
 * duals on behalf of the  Apache Software Foundation.  For more information *
 * on the Apache Software Foundation, please see <http://www.apache.org/>.   *
 *                                                                           *
 * ========================================================================= */

/* @version $Id: AloneService.java,v 1.1 2003/09/27 15:45:02 jfclere Exp $ */

import java.io.*;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Enumeration;

import org.apache.commons.collections.ExtendedProperties;
import java.io.IOException;
import java.util.Iterator;

/*
 * That is like the ServiceDaemon but it does used the interface.
 */
public class AloneService {

    private ExtendedProperties prop = null;
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
        System.setErr(new PrintStream(new FileOutputStream(new File("/ServiceDaemon.err"),true)));
        System.err.println("ServiceDaemon: instance "+this.hashCode()+
                           " init");

        /* read the properties file */
        prop = new ExtendedProperties("startfile");

        /* create an array to store the processes */
	int i=0;
        for (Iterator e = prop.getKeys(); e.hasNext() ;) {
            e.next();
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
        for (Iterator e = prop.getKeys(); e.hasNext() ;) {
           String name = (String) e.next();
           System.err.println("ServiceDaemon: starting: " + name + " : " + prop.getString(name));
           try {
               proc[i] = Runtime.getRuntime().exec(prop.getString(name));
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

    public void stop()
    throws IOException, InterruptedException {
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
