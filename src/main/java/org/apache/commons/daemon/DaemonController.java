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

package org.apache.commons.daemon;


/**
 * Defines methods needed by the DaemonLoader.
 * 
 * @author Pier Fumagalli
 * @version $Id$
 */
public interface DaemonController
{

    /**
     * Shuts down the daemon.     
     */
    public void shutdown()
        throws IllegalStateException;

    /**
     * Reloads daemon
     */
    public void reload()
        throws IllegalStateException;

    /**
     * Shuts down daemon and logs failed message.
     */
    public void fail()
        throws IllegalStateException;

    /**
     * Shuts down daemon and logs failed message.
     */
    public void fail(String message)
        throws IllegalStateException;

    /**
     * Shuts down daemon and logs failed message.
     */
    public void fail(Exception exception)
        throws IllegalStateException;

    /**
     * Shuts down daemon and logs failed message.
     */
    public void fail(String message, Exception exception)
        throws IllegalStateException;

}

