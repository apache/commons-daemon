/* ========================================================================= *
 *                                                                           *
 *                 The Apache Software License,  Version 1.1                 *
 *                                                                           *
 *             Copyright (c) 2001 The Apache Software Foundation.            *
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
 * 4. The names  "The Jakarta  Project",  and  "Apache  Software Foundation" *
 *    must not  be used  to endorse  or promote  products derived  from this *
 *    software without  prior written  permission.  For written  permission, *
 *    please contact <apache@apache.org>.                                    *
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

package org.apache.commons.daemon;

/**
 * This interface provides support for native daemon invocation. Using
 * a platform dependant helper program classes that implement the
 * <code>Daemon</code> interface can be initialized, started and
 * stopped according to the convensions of the underlying operating
 * system.
 * <p>
 * Implementors of this interface must also provide a public constructor
 * with no arguments so that instances can be created in an automated
 * fashion.
 * </p>
 * @author Pier Fumagalli
 * @author Copyright &copy; 2000-2001 <a href="http://www.apache.org/">The
 *         Apache Software Foundation</a>. All rights reserved.
 * @version 1.0 <i>(CVS $Revision: 1.3 $)</i>
 */
public interface Daemon {

    /**
     * Initialize this <code>Daemon</code> instance.
     * <p>
     *   This method gets called once the JVM process is created and the
     *   <code>Daemon</code> instance is created thru its empty public
     *   constructor.
     * </p>
     * <p>
     *   Under certain operating systems (typically Unix based operating
     *   systems) and if the native invocation framework is configured to do
     *   so, this method might be called with <i>super-user</i> privileges.
     * </p>
     * <p>
     *   For example, it might be wise to create <code>ServerSocket</code>
     *   instances within the scope of this method, and perform all operations
     *   requiring <i>super-user</i> privileges in the underlying operating
     *   system.
     * </p>
     * <p>
     *   Apart from set up and allocation of native resources, this method
     *   must not start the actual operation of the <code>Daemon</code> (such
     *   as starting threads calling the <code>ServerSocket.accept()</code>
     *   method) as this would impose some serious security hazards. The
     *   start of operation must be performed in the <code>start()</code>
     *   method.
     * </p>
     *
     * @param context A <code>DaemonContext</code> object used to
     * communicate with the container.
     * 
     * @exception Exception Any exception preventing a successful
     *                      initialization.
     */
    public void init(DaemonContext context)
    throws Exception;

    /**
     * Start the operation of this <code>Daemon</code> instance. This
     * method is to be invoked by the environment after the init()
     * method has been successfully invoked and possibly the security
     * level of the JVM has been dropped.  <code>Implementors of this
     * method are free to start any number of threads, but need to
     * return control avfter having done that to enable invocation of
     * the stop()-method.
     */
    public void start()
    throws Exception;

    /**
     * Stop the operation of this <code>Daemon</code> instance. Note
     * that the proper place to free any allocated resources such as
     * sockets or file descriptors is in the destroy method, as the
     * container may restart the Daemon by calling start() after
     * stop().
     */
    public void stop()
    throws Exception;

    /**
     * Free any resources allocated by this daemon such as file
     * descriptors or sockets. This method gets called by the container
     * after stop() has been called, before the JVM exits. The Daemon
     * can not be restarted after this method has been called without a
     * new call to the init() method.
     */
    public void destroy();
}
