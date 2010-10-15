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

/* @version $Id: Main.java 937350 2010-04-23 16:03:39Z mturk $ */

package org.apache.commons.daemon;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.apache.commons.daemon.support.DaemonConfiguration;

/**
 * Implementation of the Daemon that allows running
 * standard applications as daemons.
 * The applications must have the mechanism to manage
 * the application lifecycle.
 *
 * @version 1.0 <i>(SVN $Revision: 925053 $)</i>
 * @author Mladen Turk
 */
public class Main implements Daemon
{

    private final static String ARGS            = "args";
    private String              configFileName  = null;
    private DaemonConfiguration config;

    private Invoker             startup         = null;
    private Invoker             shutdown        = null;

    public Main()
    {
        super();
        config   = new DaemonConfiguration();
        startup  = new Invoker();
        shutdown = new Invoker();
    }

    /**
     * Called from DaemonLoader on init stage.
     */
    public void init(DaemonContext context)
        throws Exception
    {
        ArrayList  sa = new ArrayList();
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
                    if (++i == args.length)
                        throw new IllegalArgumentException(args[i - 1]);
                    configFileName = args[i];
                }
                else if (args[i].equals("-start")) {
                    if (++i == args.length)
                        throw new IllegalArgumentException(args[i - 1]);
                    startup.setClassName(args[i]);
                }
                else if (args[i].equals("-start-mehod")) {
                    if (++i == args.length)
                        throw new IllegalArgumentException(args[i - 1]);
                    startup.setMethodName(args[i]);
                }
                else if (args[i].equals("-stop")) {
                    if (++i == args.length)
                        throw new IllegalArgumentException(args[i - 1]);
                    shutdown.setClassName(args[i]);
                }
                else if (args[i].equals("-stop-method")) {
                    if (++i == args.length)
                        throw new IllegalArgumentException(args[i - 1]);
                    shutdown.setMethodName(args[i]);
                }
                else if (args[i].equals("-stop-argument")) {
                    if (++i == args.length)
                        throw new IllegalArgumentException(args[i - 1]);
                    ArrayList aa = new ArrayList();
                    aa.add(args[i]);
                    shutdown.setArguments(aa);
                }
                else {
                    // This is not our option.
                    // Everything else will be forwarded to the main
                    break;
                }
            }
            String[] copy = new String[args.length - i];
            System.arraycopy(args, i, copy, 0, copy.length);
            sa.addAll(Arrays.asList(copy));
        }
        if (config.load(configFileName)) {
            // Merge the config with command line arguments
            //
            args = config.getPropertyArray(ARGS);
            if (args != null) {
                // Add daemon.args[0] ... daemon.args[n]
                // To the end of command line arguments
                sa.addAll(Arrays.asList(args));
            }
        }
        startup.setArguments(sa);
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
        System.err.println("Main: instance " + this.hashCode() + " destroy");
    }

    // Internal class for wrapping the start/stop methods
    class Invoker
    {
        private String      name = null;
        private String      main = null;
        private String[]    args = null;
        private Method      inst = null;
        private Class       claz = null;

        protected Invoker()
        {
        }

        protected void setClassName(String name)
        {
            this.name = name;
        }
        protected void setMethodName(String name)
        {
            this.main = name;
        }

        protected void setArguments(ArrayList args)
        {
            if (args != null) {
                this.args = Arrays.copyOf(args.toArray(), args.size(), String[].class);
            }
        }

        protected void invoke()
            throws Exception
        {
            if (name.equals("System.exit")) {
                // Just call a System.exit()
                // The start method was probably installed
                // a shutdown hook.
                System.exit(0);
            }
            else {
                Object obj   = claz.newInstance();
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
            if (name == null)
                throw new NullPointerException("Null class name specified");
            else if (name.equals("System.exit"))
                return;
            if (args == null)
                args = new String[0];
            if (main == null)
                main = "main";

            // Get the ClassLoader loading this class
            ClassLoader cl = Main.class.getClassLoader();
            if (cl == null)
                throw new NullPointerException("Cannot retrieve ClassLoader instance");
            Class[] ca = new Class[1];
            ca[0]      = args.getClass();
            // Find the required class
            claz = cl.loadClass(name);
            if (claz == null)
                throw new ClassNotFoundException(name);
            // Find the required method.
            // NoSuchMethodException will be thrown if matching method
            // is not found.
            inst = claz.getMethod(main, ca);
        }
    }
}
