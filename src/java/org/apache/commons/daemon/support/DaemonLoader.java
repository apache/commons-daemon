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

package org.apache.commons.daemon.support;

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
            boolean isdaemon = false;
            try {
              Class dclass = cl.loadClass("org.apache.commons.daemon.Daemon");
              isdaemon = dclass.isAssignableFrom(c);
            } catch(Exception cnfex) {
              // Swallow if Daemon not found.
            }

            /* Check methods */
            Class[] myclass = new Class[1];
            if (isdaemon) {
              myclass[0] = DaemonContext.class;
            } else {
              myclass[0] = ar.getClass();
            }

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
            System.gc();

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
               return false (load, start and stop won't be called). */
            t.printStackTrace(System.err);
            return(false);
        }
        return(true);
    }

    public static boolean destroy() {
        try {
            /* Attempt to stop the daemon */
            Object arg[] = null;
            destroy.invoke(daemon,arg);

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
