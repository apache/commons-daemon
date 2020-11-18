/*
 *  Copyright 2010 Media Service Provider Ltd
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License./*
 *
 */
package org.apache.commons.daemon;

/**
 * Throw this during init if you can't initialise yourself for some expected reason. Using this exception will cause the
 * exception's message to come out on stdout, rather than a dirty great stacktrace.
 */
public class DaemonInitException extends Exception {

    private static final long serialVersionUID = 5665891535067213551L;

    /**
     * Constructs a new exception with the specified message.
     *
     * @param message the detail message accessible with {@link #getMessage()} .
     */
    public DaemonInitException(final String message) {
        super(message);
    }

    public DaemonInitException(final String message, final Throwable cause) {
        super(message, cause);
    }

    public String getMessageWithCause() {
        final String extra = getCause() == null ? "" : ": " + getCause().getMessage();
        return getMessage() + extra;
    }

}
