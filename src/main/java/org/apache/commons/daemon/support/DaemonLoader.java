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

package org.apache.commons.daemon.support;

import org.apache.commons.daemon.DaemonContext;
import org.apache.commons.daemon.DaemonController;
import org.apache.commons.daemon.DaemonInitException;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/**
 * Used by jsvc for Daemon management.
 *
 * @version $Id$
 */
public final class DaemonLoader
{

    // N.B. These static mutable variables need to be accessed using synch.
    private static Controller controller = null; //@GuardedBy("this")
    private static Object daemon    = null; //@GuardedBy("this")
    /* Methods to call */
    private static Method init      = null; //@GuardedBy("this")
    private static Method start     = null; //@GuardedBy("this")
    private static Method stop      = null; //@GuardedBy("this")
    private static Method destroy   = null; //@GuardedBy("this")
    private static Method signal    = null; //@GuardedBy("this")

    public static void version()
    {
        System.err.println("java version \"" +
                           System.getProperty("java.version") + "\"");
        System.err.println(System.getProperty("java.runtime.name") +
                           " (build " +
                           System.getProperty("java.runtime.version") + ")");
        System.err.println(System.getProperty("java.vm.name") +
                           " (build " +
                           System.getProperty("java.vm.version") +
                           ", " + System.getProperty("java.vm.info") + ")");
        System.err.println("commons daemon version \"" +
                System.getProperty("commons.daemon.version") + "\"");
        System.err.println("commons daemon process (id: " +
                           System.getProperty("commons.daemon.process.id") +
                           ", parent: " +
                           System.getProperty("commons.daemon.process.parent") + ")");
    }

    public static boolean check(String cn)
    {
        try {
            /* Check the class name */
            if (cn == null) {
                throw new NullPointerException("Null class name specified");
            }

            /* Get the ClassLoader loading this class */
            ClassLoader cl = DaemonLoader.class.getClassLoader();
            if (cl == null) {
                System.err.println("Cannot retrieve ClassLoader instance");
                return false;
            }

            /* Find the required class */
            Class c = cl.loadClass(cn);

            /* This should _never_ happen, but doublechecking doesn't harm */
            if (c == null) {
                throw new ClassNotFoundException(cn);
            }

            /* Create a new instance of the daemon */
            c.newInstance();

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
             * return false (load, start and stop won't be called).
             */
            t.printStackTrace(System.err);
            return false;
        }
        /* The class was loaded and instantiated correctly, we can return
         */
        return true;
    }

    public static boolean signal()
    {
        try {
            if (signal != null) {
                signal.invoke(daemon, new Object[0]);
                return true;
            }
            System.out.println("Daemon doesn't support signaling");
        } catch (Throwable ex) {
            System.err.println("Cannot send signal: " + ex);
            ex.printStackTrace(System.err);
        }
        return false;
    }

    public static boolean load(String className, String args[])
    {
        try {
            /* Make sure any previous instance is garbage collected */
            System.gc();

            /* Check if the underlying libray supplied a valid list of
               arguments */
            if (args == null) {
                args = new String[0];
            }

            /* Check the class name */
            if (className == null) {
                throw new NullPointerException("Null class name specified");
            }

            /* Get the ClassLoader loading this class */
            ClassLoader cl = DaemonLoader.class.getClassLoader();
            if (cl == null) {
                System.err.println("Cannot retrieve ClassLoader instance");
                return false;
            }
            Class c;
            if (className.charAt(0) == '@') {
                /* Wrapp the class with DaemonWrapper
                 * and modify arguments to include the real class name.
                 */
                c = DaemonWrapper.class;
                String[] a = new String[args.length + 2];
                a[0] = "-start";
                a[1] = className.substring(1);
                System.arraycopy(args, 0, a, 2, args.length);
                args = a;
            }
            else {
                c = cl.loadClass(className);
            }
            /* This should _never_ happen, but doublechecking doesn't harm */
            if (c == null) {
                throw new ClassNotFoundException(className);
            }
            /* Check interfaces */
            boolean isdaemon = false;

            try {
                Class dclass =
                    cl.loadClass("org.apache.commons.daemon.Daemon");
                isdaemon = dclass.isAssignableFrom(c);
            }
            catch (Exception cnfex) {
                // Swallow if Daemon not found.
            }

            /* Check methods */
            Class[] myclass = new Class[1];
            if (isdaemon) {
                myclass[0] = DaemonContext.class;
            }
            else {
                myclass[0] = args.getClass();
            }

            init    = c.getMethod("init", myclass);

            myclass = null;
            start   = c.getMethod("start", myclass);
            stop    = c.getMethod("stop", myclass);
            destroy = c.getMethod("destroy", myclass);

            try {
                signal = c.getMethod("signal", myclass);
            } catch (NoSuchMethodException e) {
                // Signaling will be disabled.
            }

            /* Create a new instance of the daemon */
            daemon = c.newInstance();

            if (isdaemon) {
                /* Create a new controller instance */
                controller = new Controller();

                /* Set the availability flag in the controller */
                controller.setAvailable(false);

                /* Create context */
                Context context = new Context();
                context.setArguments(args);
                context.setController(controller);

                /* Now we want to call the init method in the class */
                Object arg[] = new Object[1];
                arg[0] = context;
                init.invoke(daemon, arg);
            }
            else {
                Object arg[] = new Object[1];
                arg[0] = args;
                init.invoke(daemon, arg);
            }

        }
        catch (InvocationTargetException e) {
            Throwable thrown = e.getTargetException();
            /* DaemonInitExceptions can fail with a nicer message */
            if (thrown instanceof DaemonInitException) {
                failed(((DaemonInitException) thrown).getMessageWithCause());
            }
            else {
                thrown.printStackTrace(System.err);
            }
            return false;
        }
        catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
             * return false (load, start and stop won't be called).
             */
            t.printStackTrace(System.err);
            return false;
        }
        /* The class was loaded and instantiated correctly, we can return */
        return true;
    }

    public static boolean start()
    {
        try {
            /* Attempt to start the daemon */
            Object arg[] = null;
            start.invoke(daemon, arg);

            /* Set the availability flag in the controller */
            if (controller != null) {
                controller.setAvailable(true);
            }

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
             * return false (load, start and stop won't be called).
             */
            t.printStackTrace(System.err);
            return false;
        }
        return true;
    }

    public static boolean stop()
    {
        try {
            /* Set the availability flag in the controller */
            if (controller != null) {
                controller.setAvailable(false);
            }

            /* Attempt to stop the daemon */
            Object arg[] = null;
            stop.invoke(daemon, arg);

            /* Run garbage collector */
            System.gc();

        }
        catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
             * return false (load, start and stop won't be called).
             */
            t.printStackTrace(System.err);
            return false;
        }
        return true;
    }

    public static boolean destroy()
    {
        try {
            /* Attempt to stop the daemon */
            Object arg[] = null;
            destroy.invoke(daemon, arg);

            /* Run garbage collector */
            daemon = null;
            controller = null;
            System.gc();

        } catch (Throwable t) {
            /* In case we encounter ANY error, we dump the stack trace and
             * return false (load, start and stop won't be called).
             */
            t.printStackTrace(System.err);
            return false;
        }
        return true;
    }

    private static native void shutdown(boolean reload);
    private static native void failed(String message);

    public static class Controller
        implements DaemonController
    {

        private boolean available = false;

        private Controller()
        {
            super();
            this.setAvailable(false);
        }

        private boolean isAvailable()
        {
            synchronized (this) {
                return this.available;
            }
        }

        private void setAvailable(boolean available)
        {
            synchronized (this) {
                this.available = available;
            }
        }

        public void shutdown()
            throws IllegalStateException
        {
            synchronized (this) {
                if (!this.isAvailable()) {
                    throw new IllegalStateException();
                }
                this.setAvailable(false);
                DaemonLoader.shutdown(false);
            }
        }

        public void reload()
            throws IllegalStateException
        {
            synchronized (this) {
                if (!this.isAvailable()) {
                    throw new IllegalStateException();
                }
                this.setAvailable(false);
                DaemonLoader.shutdown(true);
            }
        }

        public void fail()
        {
            fail(null, null);
        }

        public void fail(String message)
        {
            fail(message, null);
        }

        public void fail(Exception exception)
        {
            fail(null, exception);
        }

        public void fail(String message, Exception exception)
        {
            synchronized (this) {
                this.setAvailable(false);
                String msg = message;
                if (exception != null) {
                    if (msg != null) {
                        msg = msg + ": " + exception.toString();
                    }
                    else {
                        msg = exception.toString();
                    }
                }
                DaemonLoader.failed(msg);
            }
        }

    }

    public static class Context
        implements DaemonContext
    {

        private DaemonController daemonController = null;

        private String[] args = null;

        public DaemonController getController()
        {
            return daemonController;
        }

        public void setController(DaemonController controller)
        {
            this.daemonController = controller;
        }

        public String[] getArguments()
        {
            return args;
        }

        public void setArguments(String[]args)
        {
            this.args = args;
        }

    }
}
