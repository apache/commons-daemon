/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *          Copyright (c) 1999-2003 The Apache Software Foundation.          *
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

/* @version $Id: DaemonLoader.java,v 1.1 2003/09/04 23:28:20 yoavs Exp $ */

package org.apache.commons.daemon.support;

import org.apache.commons.daemon.Daemon;
import org.apache.commons.daemon.DaemonContext;
import org.apache.commons.daemon.DaemonController;

import java.lang.reflect.Method;

public final class DaemonLoader {

    private static Controller controller = null;
    private static Context context = null;
    private static Object daemon = null;
    /* Methods to call */
    private static Method init = null;
    private static Method start = null;
    private static Method stop = null;
    private static Method destroy = null;

    public static void version() {
        System.err.println("java version \""+
                           System.getProperty("java.version")+
                           "\"");
        System.err.println(System.getProperty("java.runtime.name")+
                           " (build "+
                           System.getProperty("java.runtime.version")+
                           ")");
        System.err.println(System.getProperty("java.vm.name")+
                           " (build "+
                           System.getProperty("java.vm.version")+
                           ", "+
                           System.getProperty("java.vm.info")+
                           ")");
    }

    public static boolean check(String cn) {
        try {
            /* Check the class name */
            if (cn==null)
                throw new NullPointerException("Null class name specified");

            /* Get the ClassLoader loading this class */
            ClassLoader cl=DaemonLoader.class.getClassLoader();
            if (cl==null) {
                System.err.println("Cannot retrieve ClassLoader instance");
                return(false);
            }

            /* Find the required class */
            Class c=cl.loadClass(cn);

            /* This should _never_ happen, but doublechecking doesn't harm */
            if (c==null) throw new ClassNotFoundException(cn);

            /* Create a new instance of the daemon */
            Object s=c.newInstance();

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
               return false (load, start and stop won't be called). */
            t.printStackTrace(System.err);
            return(false);
        }
        /* The class was loaded and instantiated correctly, we can return */
        return(true);
    }

    public static boolean load(String cn, String ar[]) {
        try {
            /* Make sure any previous instance is garbage collected */
            System.gc();

            /* Check if the underlying libray supplied a valid list of
               arguments */
            if (ar==null) ar=new String[0];

            /* Check the class name */
            if (cn==null)
                throw new NullPointerException("Null class name specified");

            /* Get the ClassLoader loading this class */
            ClassLoader cl=DaemonLoader.class.getClassLoader();
            if (cl==null) {
                System.err.println("Cannot retrieve ClassLoader instance");
                return(false);
            }

            /* Find the required class */
            Class c=cl.loadClass(cn);

            /* This should _never_ happen, but doublechecking doesn't harm */
            if (c==null) throw new ClassNotFoundException(cn);

            /* Check interface */
            Class[] interf = c.getInterfaces();
            boolean isdaemon = false;
            if ( interf != null ) {
              for(int i=0;i<interf.length;i++) {
                if (interf[i].getName().equals("org.apache.commons.daemon.Daemon"))
                  isdaemon = true;
              }
            }
            /* Check methods */
            Class[] myclass = new Class[1];
            myclass[0] = ar.getClass();
            init = c.getMethod("init",myclass);

            myclass = null;
            start = c.getMethod("start",myclass);

            stop = c.getMethod("stop",myclass);

            destroy = c.getMethod("destroy",myclass);

            /* Create a new instance of the daemon */
            daemon=c.newInstance();

            if (isdaemon) {
              /* Create a new controller instance */
              controller=new Controller();

              /* Set the availability flag in the controller */
              controller.setAvailable(false);

              /* Create context */
              context = new Context();
              context.setArguments(ar);
              context.setController(controller);

              /* Now we want to call the init method in the class */
              Object arg[] = new Object[1];
              arg[0] = context;
              init.invoke(daemon,arg);
            } else {
              Object arg[] = new Object[1];
              arg[0] = ar;
              init.invoke(daemon,arg);
            }

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
               return false (load, start and stop won't be called). */
            t.printStackTrace(System.err);
            return(false);
        }
        /* The class was loaded and instantiated correctly, we can return */
        return(true);
    }

    public static boolean start() {
        try {
            /* Attempt to start the daemon */
            Object arg[] = null;
            start.invoke(daemon,arg);

            /* Set the availability flag in the controller */
            if (controller != null)
              controller.setAvailable(true);

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
               return false (load, start and stop won't be called). */
            t.printStackTrace(System.err);
            return(false);
        }
        return(true);
    }

    public static boolean stop() {
        try {
            /* Set the availability flag in the controller */
            if (controller != null)
              controller.setAvailable(false);

            /* Attempt to stop the daemon */
            Object arg[] = null;
            stop.invoke(daemon,arg);

            /* Run garbage collector */
            daemon=null;
            controller=null;
            System.gc();

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
               return false (load, start and stop won't be called). */
            t.printStackTrace(System.err);
            return(false);
        }
        return(true);
    }

    private static native void shutdown(boolean reload);

    public static class Controller implements DaemonController {

        boolean available=false;

        private Controller() {
            super();
            this.setAvailable(false);
        }

        private boolean isAvailable() {
            synchronized (this) {
                return(this.available);
            }
        }

        private void setAvailable(boolean available) {
            synchronized (this) {
                this.available=available;
            }
        }

        public void shutdown() throws IllegalStateException {
            synchronized (this) {
                if (!this.isAvailable()) {
                    throw new IllegalStateException();
                } else {
                    this.setAvailable(false);
                    DaemonLoader.shutdown(false);
                }
            }
        }

        public void reload() throws IllegalStateException {
            synchronized (this) {
                if (!this.isAvailable()) {
                    throw new IllegalStateException();
                } else {
                    this.setAvailable(false);
                    DaemonLoader.shutdown(true);
                }
            }
        }

        public void fail()
            throws IllegalStateException {
        }

        public void fail(String message)
            throws IllegalStateException {
        }

        public void fail(Exception exception)
            throws IllegalStateException {
        }

        public void fail(String message, Exception exception)
            throws IllegalStateException {
        }

    }

    public static class Context implements DaemonContext {

        DaemonController controller = null;

        String[] args = null;

        public DaemonController getController() {
            return controller;
        }

        public void setController(DaemonController controller) {
            this.controller = controller;
        }

        public String[] getArguments() {
            return args;
        }

        public void setArguments(String[] args) {
            this.args = args;
        }

    }

}
