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

import java.security.Permission;
import java.util.StringTokenizer;

/**
 * This class represents the permissions to control and query the status of
 * a <code>Daemon</code>. A <code>DaemonPermission</code> consists of a
 * target name and a list of actions associated with it.
 * <p>
 * In this specification version the only available target name for this
 * permission is &quot;control&quot;, but further releases may add more target
 * names to fine-tune the access that needs to be granted to the caller.
 * </p>
 * <p>
 * Actions are defined by a string of comma-separated values, as shown in the
 * table below. The empty string implies no permission at all, while the
 * special &quot;*&quot; value implies all permissions for the given
 * name:
 * </p>
 * <p>
 * <table width="100%" border="1">
 *  <tr>
 *   <th>Target&quot;Name</th>
 *   <th>Action</th>
 *   <th>Description</th>
 *  </tr>
 *  <tr>
 *   <td rowspan="5">&quot;control&quot;</td>
 *   <td>&quot;start&quot;</td>
 *   <td>
 *    The permission to call the <code>start()</code> method in an instance
 *    of a <code>DaemonController</code> interface.
 *   </td>
 *  </tr>
 *  <tr>
 *   <td>&quot;stop&quot;</td>
 *   <td>
 *    The permission to call the <code>stop()</code> method in an instance
 *    of a <code>DaemonController</code> interface.
 *   </td>
 *  </tr>
 *  <tr>
 *   <td>&quot;shutdown&quot;</td>
 *   <td>
 *    The permission to call the <code>shutdown()</code> method in an instance
 *    of a <code>DaemonController</code> interface.
 *   </td>
 *  </tr>
 *  <tr>
 *   <td>&quot;reload&quot;</td>
 *   <td>
 *    The permission to call the <code>reload()</code> method in an instance
 *    of a <code>DaemonController</code> interface.
 *   </td>
 *  </tr>
 *  <tr>
 *   <td>&quot;*&quot;</td>
 *   <td>
 *    The special wildcard action implies all above-mentioned action. This is
 *    equal to construct a permission with the &quot;start, stop, shutdown,
 *    reload&quot; list of actions.
 *   </td>
 *  </tr>
 * </table>
 * </p>
 *
 * @author <a href="mailto:pier.fumagalli@sun.com">Pier Fumagalli</a>
 * @author Copyright &copy; 2000-2001 <a href="http://www.apache.org/">The
 *         Apache Software Foundation</a>. All rights reserved.
 * @version 1.0 <i>(CVS $Revision: 1.1 $)</i>
 */
public final class DaemonPermission extends Permission {

    /* ==================================================================== */
    /* Constants. */

    /**
     * The target name when associated with control actions
     * (&quot;control&quot;).
     */
    protected static final String CONTROL = "control";

    /**
     * The target type when associated with control actions.
     */
    protected static final int TYPE_CONTROL = 1;

    /**
     * The action name associated with the permission to call the
     * <code>DaemonController.start()</code> method.
     */
    protected static final String CONTROL_START = "start";

    /**
     * The action name associated with the permission to call the
     * <code>DaemonController.stop()</code> method.
     */
    protected static final String CONTROL_STOP = "stop";

    /**
     * The action name associated with the permission to call the
     * <code>DaemonController.shutdown()</code> method.
     */
    protected static final String CONTROL_SHUTDOWN = "shutdown";

    /**
     * The action name associated with the permission to call the
     * <code>DaemonController.reload()</code> method.
     */
    protected static final String CONTROL_RELOAD = "reload";

    /**
     * The action mask associated with the permission to call the
     * <code>DaemonController.start()</code> method.
     */
    protected static final int MASK_CONTROL_START = 0x01;

    /**
     * The action mask associated with the permission to call the
     * <code>DaemonController.stop()</code> method.
     */
    protected static final int MASK_CONTROL_STOP = 0x02;

    /**
     * The action mask associated with the permission to call the
     * <code>DaemonController.shutdown()</code> method.
     */
    protected static final int MASK_CONTROL_SHUTDOWN = 0x04;

    /**
     * The action mask associated with the permission to call the
     * <code>DaemonController.reload()</code> method.
     */
    protected static final int MASK_CONTROL_RELOAD = 0x08;

    /**
     * The &quot;wildcard&quot; action implying all actions for the given
     * target name.
     */
    protected static final String WILDCARD = "*";

    /* ==================================================================== */
    /* Instance variables */

    /** The type of this permission object. */
    private transient int type = 0;
    /** The permission mask associated with this permission object. */
    private transient int mask = 0;
    /** The String representation of this permission object. */
    private transient String desc = null;

    /* ==================================================================== */
    /* Constructors */

    /**
     * Create a new <code>DaemonPermission</code> instance with a specified
     * permission name.
     * <p>
     * This constructor will create a new <code>DaemonPermission</code>
     * instance that <b>will not</b> grant any permission to the caller.
     *
     * @param target The target name of this permission.
     * @exception IllegalArgumentException If the specified target name is not
     *                supported.
     */
    public DaemonPermission (String target)
    throws IllegalArgumentException {
        // Setup the target name of this permission object.
        super(target);

        // Check if the permission target name was specified
        if (target==null)
            throw new IllegalArgumentException("Null permission name");

        // Check if this is a "control" permission and set up accordingly.
        if (CONTROL.equalsIgnoreCase(target)) {
            type=TYPE_CONTROL;
            return;
        }

        // If we got here, we have an invalid permission name.
        throw new IllegalArgumentException("Invalid permission name \""+
                                           target+"\" specified");
    }

    /**
     * Create a new <code>DaemonPermission</code> instance with a specified
     * permission name and a specified list of actions.
     * <p>
     * </p>
     *
     * @param target The target name of this permission.
     * @param actions The list of actions permitted by this permission.
     * @exception IllegalArgumentException If the specified target name is not
     *                supported, or the specified list of actions includes an
     *                invalid value.
     */
    public DaemonPermission(String target, String actions)
    throws IllegalArgumentException {
        // Setup this instance's target name.
        this(target);

        // Create the appropriate mask if this is a control permission.
        if (this.type==TYPE_CONTROL) {
            this.mask=this.createControlMask(actions);
            return;
        }
    }

    /* ==================================================================== */
    /* Public methods */

    /**
     * Return the list of actions permitted by this instance of
     * <code>DaemonPermission</code> in its canonical form.
     *
     * @return The canonicalized list of actions.
     */
    public String getActions() {
        if (this.type==TYPE_CONTROL) {
            return(this.createControlActions(this.mask));
        }
        return("");
    }

    /**
     * Return the hash code for this <code>DaemonPermission</code> instance.
     *
     * @return An hash code value.
     */
    public int hashCode() {
        this.setupDescription();
        return(this.desc.hashCode());
    }

    /**
     * Check if a specified object equals <code>DaemonPermission</code>.
     *
     * @return <b>true</b> or <b>false</b> wether the specified object equals
     *         this <code>DaemonPermission</code> instance or not.
     */
    public boolean equals(Object object) {
        if (object == this) return(true);

        if (!(object instanceof DaemonPermission)) return false;

        DaemonPermission that = (DaemonPermission)object;

        if (this.type!=that.type) return(false);
        return(this.mask==that.mask);
    }

    /**
     * Check if this <code>DaemonPermission</code> implies another
     * <code>Permission</code>.
     *
     * @return <b>true</b> or <b>false</b> wether the specified permission
     *         is implied by this <code>DaemonPermission</code> instance or
     *         not.
     */
    public boolean implies(Permission permission) {
        if (permission == this) return(true);

        if (!(permission instanceof DaemonPermission)) return false;

        DaemonPermission that = (DaemonPermission)permission;

        if (this.type!=that.type) return(false);
        return((this.mask&that.mask)==that.mask);
    }

    /**
     * Return a <code>String</code> representation of this instance.
     *
     * @return A <code>String</code> representing this
     *         <code>DaemonPermission</code> instance.
     */
    public String toString() {
        this.setupDescription();
        return(new String(this.desc));
    }

    /* ==================================================================== */
    /* Private methods */

    /** Create a String description for this permission instance. */
    private void setupDescription() {
        if (this.desc!=null) return;

        StringBuffer buf=new StringBuffer();
        buf.append(this.getClass().getName());
        buf.append('[');
        switch (this.type) {
            case (TYPE_CONTROL): {
                buf.append(CONTROL);
                break;
            }
            default: {
                buf.append("UNKNOWN");
                break;
            }
        }
        buf.append(':');
        buf.append(this.getActions());
        buf.append(']');

        this.desc=buf.toString();
    }

    /** Create a permission mask for a given control actions string. */
    private int createControlMask(String actions)
    throws IllegalArgumentException {
        if (actions==null) return(0);

        int mask=0;
        StringTokenizer tok=new StringTokenizer(actions,",",false);
        while (tok.hasMoreTokens()) {
            String val=tok.nextToken().trim();

            if (WILDCARD.equals(val)) {
                return(MASK_CONTROL_START|MASK_CONTROL_STOP|
                       MASK_CONTROL_SHUTDOWN|MASK_CONTROL_RELOAD);
            } else if (CONTROL_START.equalsIgnoreCase(val)) {
                mask=mask|MASK_CONTROL_START;
            } else if (CONTROL_STOP.equalsIgnoreCase(val)) {
                mask=mask|MASK_CONTROL_STOP;
            } else if (CONTROL_SHUTDOWN.equalsIgnoreCase(val)) {
                mask=mask|MASK_CONTROL_SHUTDOWN;
            } else if (CONTROL_RELOAD.equalsIgnoreCase(val)) {
                mask=mask|MASK_CONTROL_RELOAD;
            } else {
                throw new IllegalArgumentException("Invalid action name \""+
                                                   val+"\" specified");
            }
        }
        return(mask);
    }

    /** Create a actions list for a given control permission mask. */
    private String createControlActions(int mask) {
        StringBuffer buf=new StringBuffer();
        boolean sep=false;

        if ((mask&MASK_CONTROL_START)==MASK_CONTROL_START) {
            sep=true;
            buf.append(CONTROL_START);
        }

        if ((mask&MASK_CONTROL_STOP)==MASK_CONTROL_STOP) {
            if (sep) buf.append(",");
            else sep=true;
            buf.append(CONTROL_STOP);
        }

        if ((mask&MASK_CONTROL_SHUTDOWN)==MASK_CONTROL_SHUTDOWN) {
            if (sep) buf.append(",");
            else sep=true;
            buf.append(CONTROL_SHUTDOWN);
        }

        if ((mask&MASK_CONTROL_RELOAD)==MASK_CONTROL_RELOAD) {
            if (sep) buf.append(",");
            else sep=true;
            buf.append(CONTROL_RELOAD);
        }

        return buf.toString();
    }
}
