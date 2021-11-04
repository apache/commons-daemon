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
 */
public final class DaemonLoader
{

    // N.B. These static mutable variables need to be accessed using synch.
    private static Controller controller; //@GuardedBy("this")
    private static Object daemon; //@GuardedBy("this")
    /* Methods to call */
    private static Method init; //@GuardedBy("this")
    private static Method start; //@GuardedBy("this")
    private static Method stop; //@GuardedBy("this")
    private static Method destroy; //@GuardedBy("this")
    private static Method signal; //@GuardedBy("this")

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

    public static boolean check(final String cn)
    {
        try {
            /* Check the class name */
            if (cn == null) {
                throw new NullPointerException("Null class name specified");
            }

            /* Get the ClassLoader loading this class */
            final ClassLoader cl = DaemonLoader.class.getClassLoader();
            if (cl == null) {
                System.err.println("Cannot retrieve ClassLoader instance");
                return false;
            }

            /* Find the required class */
            final Class<?> c = cl.loadClass(cn);

            /* This should _never_ happen, but doublechecking doesn't harm */
            if (c == null) {
                throw new ClassNotFoundException(cn);
            }

            /* Create a new instance of the daemon */
            c.getConstructor().newInstance();

        } catch (final Throwable t) {
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
                signal.invoke(daemon);
                return true;
            }
            System.out.println("Daemon doesn't support signaling");
        } catch (final Throwable ex) {
            System.err.println("Cannot send signal: " + ex);
            ex.printStackTrace(System.err);
        }
        return false;
    }

    public static boolean load(final String className, String[] args)
    {
        try {
            /* Check if the underlying library supplied a valid list of
               arguments */
            if (args == null) {
                args = new String[0];
            }

            /* Check the class name */
            if (className == null) {
                throw new NullPointerException("Null class name specified");
            }

            /* Get the ClassLoader loading this class */
            final ClassLoader cl = DaemonLoader.class.getClassLoader();
            if (cl == null) {
                System.err.println("Cannot retrieve ClassLoader instance");
                return false;
            }
            final Class<?> c;
            if (className.charAt(0) == '@') {
                /* Wrap the class with DaemonWrapper
                 * and modify arguments to include the real class name.
                 */
                c = DaemonWrapper.class;
                final String[] a = new String[args.length + 2];
                a[0] = "-start";
                a[1] = className.substring(1);
                System.arraycopy(args, 0, a, 2, args.length);
                args = a;
            }
            else {
                c = cl.loadClass(className);
            }
            /* This should _never_ happen, but double-checking doesn't harm */
            if (c == null) {
                throw new ClassNotFoundException(className);
            }
            /* Check interfaces */
            boolean isdaemon = false;

            try {
                final Class<?> dclass = cl.loadClass("org.apache.commons.daemon.Daemon");
                isdaemon = dclass.isAssignableFrom(c);
            }
            catch (final Exception cnfex) {
                // Swallow if Daemon not found.
            }

            /* Check methods */
            Class<?>[] myclass = new Class[1];
            if (isdaemon) {
                myclass[0] = DaemonContext.class;
            }
            else {
                myclass[0] = args.getClass();
            }

            init    = c.getMethod("init", myclass);

            start   = c.getMethod("start", null);
            stop    = c.getMethod("stop", null);
            destroy = c.getMethod("destroy", null);

            try {
                signal = c.getMethod("signal", null);
            } catch (final NoSuchMethodException e) {
                // Signalling will be disabled.
            }

            /* Create a new instance of the daemon */
            daemon = c.getConstructor().newInstance();

            if (isdaemon) {
                /* Create a new controller instance */
                controller = new Controller();

                /* Set the availability flag in the controller */
                controller.setAvailable(false);

                /* Create context */
                final Context context = new Context();
                context.setArguments(args);
                context.setController(controller);

                /* Now we want to call the init method in the class */
                final Object[] arg = new Object[1];
                arg[0] = context;
                init.invoke(daemon, arg);
            }
            else {
                final Object[] arg = new Object[1];
                arg[0] = args;
                init.invoke(daemon, arg);
            }

        }
        catch (final InvocationTargetException e) {
            final Throwable thrown = e.getTargetException();
            /* DaemonInitExceptions can fail with a nicer message */
            if (thrown instanceof DaemonInitException) {
                failed(((DaemonInitException) thrown).getMessageWithCause());
            }
            else {
                thrown.printStackTrace(System.err);
            }
            return false;
        }
        catch (final Throwable t) {
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
            start.invoke(daemon, null);

            /* Set the availability flag in the controller */
            if (controller != null) {
                controller.setAvailable(true);
            }

        } catch (final Throwable t) {
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
            stop.invoke(daemon, null);
        }
        catch (final Throwable t) {
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
            destroy.invoke(daemon, null);

            daemon = null;
            controller = null;
        } catch (final Throwable t) {
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

        private boolean available;

        private Controller()
        {
            this.setAvailable(false);
        }

        private boolean isAvailable()
        {
            synchronized (this) {
                return this.available;
            }
        }

        private void setAvailable(final boolean available)
        {
            synchronized (this) {
                this.available = available;
            }
        }

        @Override
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

        @Override
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

        @Override
        public void fail()
        {
            fail(null, null);
        }

        @Override
        public void fail(final String message)
        {
            fail(message, null);
        }

        @Override
        public void fail(final Exception exception)
        {
            fail(null, exception);
        }

        @Override
        public void fail(final String message, final Exception exception)
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

        private DaemonController daemonController;

        private String[] args;

        @Override
        public DaemonController getController()
        {
            return daemonController;
        }

        public void setController(final DaemonController controller)
        {
            this.daemonController = controller;
        }

        @Override
        public String[] getArguments()
        {
            return args;
        }

        public void setArguments(final String[]args)
        {
            this.args = args;
        }

    }
}
