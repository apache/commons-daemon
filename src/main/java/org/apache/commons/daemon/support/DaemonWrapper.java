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

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.Arrays;
import org.apache.commons.daemon.Daemon;
import org.apache.commons.daemon.DaemonContext;

/**
 * Implementation of the Daemon that allows running
 * standard applications as daemons.
 * The applications must have the mechanism to manage
 * the application lifecycle.
 *
 * @version $Id$
 * @author Mladen Turk
 */
public class DaemonWrapper implements Daemon
{

    private final static String ARGS            = "args";
    private final static String START_CLASS     = "start";
    private final static String START_METHOD    = "start.method";
    private final static String STOP_CLASS      = "stop";
    private final static String STOP_METHOD     = "stop.method";
    private final static String STOP_ARGS       = "stop.args";
    private String              configFileName  = null;
    private final DaemonConfiguration config;

    private final Invoker             startup;
    private final Invoker             shutdown;

    public DaemonWrapper()
    {
        super();
        config   = new DaemonConfiguration();
        startup  = new Invoker();
        shutdown = new Invoker();
    }

    /**
     * Called from DaemonLoader on init stage.
     * <p>
     * Accepts the following configuration arguments:
     * <ul>
     * <li>-daemon-properties: - load DaemonConfiguration properties from the specified file to act as defaults</li>
     * <li>-start: set start class name</li>
     * <li>-start-method: set start method name</li>
     * <li>-stop: set stop class name</li>
     * <li>-stop-method: set stop method name</li>
     * <li>-stop-argument: set optional argument to stop method</li>
     * <li>Anything else is treated as a startup argument</li>
     * </ul>
     * <p>
     * The following "-daemon-properties" are recognized:
     * <ul>
     * <li>args (startup argument)</li>
     * <li>start</li>
     * <li>start.method</li>
     * <li>stop</li>
     * <li>stop.method</li>
     * <li>stop.args</li>
     * </ul>
     * These are used to set the corresponding item if it has not already been
     * set by the command arguments. <b>However, note that args and stop.args are
     * appended to any existing values.</b>
     */
    public void init(DaemonContext context)
        throws Exception
    {
        String[] args = context.getArguments();

        if (args != null) {
            int i;
            // Parse our arguments and remove them
            // from the final argument array we are
            // passing to our child.
            for (i = 0; i < args.length; i++) {
                if (args[i].equals("--")) {
                    // Done with argument processing
                    break;
                }
                else if (args[i].equals("-daemon-properties")) {
                    if (++i == args.length) {
                        throw new IllegalArgumentException(args[i - 1]);
                    }
                    configFileName = args[i];
                }
                else if (args[i].equals("-start")) {
                    if (++i == args.length) {
                        throw new IllegalArgumentException(args[i - 1]);
                    }
                    startup.setClassName(args[i]);
                }
                else if (args[i].equals("-start-method")) {
                    if (++i == args.length) {
                        throw new IllegalArgumentException(args[i - 1]);
                    }
                    startup.setMethodName(args[i]);
                }
                else if (args[i].equals("-stop")) {
                    if (++i == args.length) {
                        throw new IllegalArgumentException(args[i - 1]);
                    }
                    shutdown.setClassName(args[i]);
                }
                else if (args[i].equals("-stop-method")) {
                    if (++i == args.length) {
                        throw new IllegalArgumentException(args[i - 1]);
                    }
                    shutdown.setMethodName(args[i]);
                }
                else if (args[i].equals("-stop-argument")) {
                    if (++i == args.length) {
                        throw new IllegalArgumentException(args[i - 1]);
                    }
                    String[] aa = new String[1];
                    aa[0] = args[i];
                    shutdown.addArguments(aa);
                }
                else {
                    // This is not our option.
                    // Everything else will be forwarded to the main
                    break;
                }
            }
            if (args.length > i) {
                String[] copy = new String[args.length - i];
                System.arraycopy(args, i, copy, 0, copy.length);
                startup.addArguments(copy);
            }
        }
        if (config.load(configFileName)) {
            // Setup params if not set via cmdline.
            startup.setClassName(config.getProperty(START_CLASS));
            startup.setMethodName(config.getProperty(START_METHOD));
            // Merge the config with command line arguments
            startup.addArguments(config.getPropertyArray(ARGS));

            shutdown.setClassName(config.getProperty(STOP_CLASS));
            shutdown.setMethodName(config.getProperty(STOP_METHOD));
            shutdown.addArguments(config.getPropertyArray(STOP_ARGS));
        }
        startup.validate();
        shutdown.validate();
    }

    /**
     */
    public void start()
        throws Exception
    {
        startup.invoke();
    }

    /**
     */
    public void stop()
        throws Exception
    {
        shutdown.invoke();
    }

    /**
     */
    public void destroy()
    {
        // Nothing for the moment
        System.err.println("DaemonWrapper: instance " + this.hashCode() + " destroy");
    }

    // Internal class for wrapping the start/stop methods
    class Invoker
    {
        private String      name = null;
        private String      call = null;
        private String[]    args = null;
        private Method      inst = null;
        private Class<?>    main = null;

        protected Invoker()
        {
        }

        protected void setClassName(String name)
        {
            if (this.name == null) {
                this.name = name;
            }
        }
        protected void setMethodName(String name)
        {
            if (this.call == null) {
                this.call = name;
            }
        }
        protected void addArguments(String[] args)
        {
            if (args != null) {
                ArrayList<String> aa = new ArrayList<String>();
                if (this.args != null) {
                    aa.addAll(Arrays.asList(this.args));
                }
                aa.addAll(Arrays.asList(args));
                this.args = aa.toArray(new String[aa.size()]);
            }
        }

        protected void invoke()
            throws Exception
        {
            if (name.equals("System") && call.equals("exit")) {
                // Just call a System.exit()
                // The start method was probably installed
                // a shutdown hook.
                System.exit(0);
            }
            else {
                Object obj   = null;
                if ((inst.getModifiers() & Modifier.STATIC) == 0) {
                    // We only need object instance for non-static methods.
                    obj = main.newInstance();
                }
                Object arg[] = new Object[1];

                arg[0] = args;
                inst.invoke(obj, arg);
            }
        }
        // Load the class using reflection
        protected void validate()
            throws Exception
        {
            /* Check the class name */
            if (name == null) {
                name = "System";
                call = "exit";
                return;
            }
            if (args == null) {
                args = new String[0];
            }
            if (call == null) {
                call = "main";
            }

            // Get the ClassLoader loading this class
            ClassLoader cl = DaemonWrapper.class.getClassLoader();
            if (cl == null) {
                throw new NullPointerException("Cannot retrieve ClassLoader instance");
            }
            Class<?>[] ca = new Class[1];
            ca[0]      = args.getClass();
            // Find the required class
            main = cl.loadClass(name);
            if (main == null) {
                throw new ClassNotFoundException(name);
            }
            // Find the required method.
            // NoSuchMethodException will be thrown if matching method
            // is not found.
            inst = main.getMethod(call, ca);
        }
    }
}
