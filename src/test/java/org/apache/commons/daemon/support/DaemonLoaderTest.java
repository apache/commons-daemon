/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.commons.daemon.support;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.Test;

/**
 * Tests {@link DaemonLoader}.
 */
class DaemonLoaderTest {

    /**
     * A plain application, as run by jsvc through DaemonWrapper with {@code @className}.
     */
    public static class WrappedApplication {

        public static void main(final String[] args) {
            // empty
        }
    }

    @Test
    void testCheckDaemonClass() {
        assertTrue(DaemonLoader.check(WrappedApplication.class.getName()));
        assertFalse(DaemonLoader.check("org.apache.commons.daemon.DoesNotExist"));
    }

    @Test
    void testCheckWrappedClass() {
        // jsvc -check @className must accept what DaemonLoader.load() accepts.
        assertTrue(DaemonLoader.check("@" + WrappedApplication.class.getName()));
        assertFalse(DaemonLoader.check("@org.apache.commons.daemon.DoesNotExist"));
    }
}
