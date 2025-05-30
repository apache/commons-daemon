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

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Properties;
import java.text.ParseException;

/**
 * Used by jsvc for Daemon configuration.
 * <p>
 * Configuration is read from properties file.
 * If no properties file is given the {@code daemon.properties}
 * is used from the current directory.
 * </p>
 * <p>
 * The properties file can have property values expanded at runtime
 * by using System properties or execution environment. The part
 * of the property value between {@code ${} and {@code }}
 * will be used as System property or environment key. If found then
 * the entire {@code ${foo}} will be replaced by the value of
 * either system property or environment variable named {@code foo}.
 * </p>
 * <p>
 * If no variable is found the {@code ${foo}}  will be passed as is.
 * In case of {@code $${foo}} this will be unescaped and resulting
 * value will be {@code ${foo}}.
 * </p>
 */
public final class DaemonConfiguration
{
    /**
     * Default configuration file name.
     */
    protected static final String DEFAULT_CONFIG        = "daemon.properties";
    /**
     * Property prefix
     */
    protected static final String PREFIX                = "daemon.";
    private   static final String BTOKEN                = "${";
    private   static final String ETOKEN                = "}";

    private final Properties configurationProperties;
    private final Properties systemProperties;

    /**
     * An empty immutable {@code String} array.
     */
    static final String[] EMPTY_STRING_ARRAY = {};

    /**
     * Default constructor
     */
    public DaemonConfiguration()
    {
        configurationProperties = new Properties();
        systemProperties        = System.getProperties();
    }

    /**
     * Loads the configuration properties file.
     *
     * @param fileName The properties file to load.
     * @return {@code true} if the file was loaded.
     */
    public boolean load(String fileName)
    {
        if (fileName == null) {
            fileName = DEFAULT_CONFIG;
        }

        try (InputStream inputStream = new FileInputStream(fileName)) {
            configurationProperties.clear();
            configurationProperties.load(inputStream);
            return true;
        } catch (final IOException ex) {
            // Error reading properties file
            return false;
        }
    }

    private String expandProperty(final String propValue)
        throws ParseException
    {
        final StringBuilder expanded;
        int btoken;
        int ctoken = 0;

        if (propValue == null) {
            return null;
        }
        expanded = new StringBuilder();
        btoken   = propValue.indexOf(BTOKEN);
        while (btoken != -1) {
            if (btoken > 0 && propValue.charAt(btoken - 1) == BTOKEN.charAt(0)) {
                // Skip and unquote.
                expanded.append(propValue.substring(ctoken, btoken));
                ctoken = btoken + 1;
                btoken = propValue.indexOf(BTOKEN, btoken + BTOKEN.length());
                continue;
            }
            final int etoken = propValue.indexOf(ETOKEN, btoken);
            if (etoken == -1) {
                // We have "${" without "}"
                throw new ParseException("Error while looking for teminating '" +
                                         ETOKEN + "'", btoken);
            }
            final String variable = propValue.substring(btoken + BTOKEN.length(), etoken);
            String sysvalue = systemProperties.getProperty(variable);
            if (sysvalue == null) {
                // Try with the environment if there was no
                // property by that name.
                sysvalue = System.getenv(variable);
            }
            if (sysvalue != null) {
                final String strtoken = propValue.substring(ctoken, btoken);
                expanded.append(strtoken);
                expanded.append(sysvalue);
                ctoken = etoken + ETOKEN.length();
            }
            btoken = propValue.indexOf(BTOKEN, etoken + ETOKEN.length());
        }
        // Add what's left.
        expanded.append(propValue.substring(ctoken));
        return expanded.toString();
    }

    /**
     * Gets the configuration property.
     *
     * @param name The name of the property to get.
     * @throws ParseException if the property is wrongly formatted.
     * @return  Configuration property including any expansion/replacement
     */
    public String getProperty(final String name)
        throws ParseException
    {
        if (name == null) {
            return null;
        }
        return expandProperty(configurationProperties.getProperty(PREFIX + name));
    }

    /**
     * Gets the configuration property array.
     * <p>
     * Property array is constructed form the list of properties
     * which end with {@code [index]}
     * </p>
     * <pre>
     * daemon.arg[0] = argument 1
     * daemon.arg[1] = argument 2
     * daemon.arg[2] = argument 3
     * </pre>
     * @param name The name of the property array to get.
     * @throws ParseException if the property is wrongly formatted.
     * @return  Configuration property array including any expansion/replacement
     */
    public String[] getPropertyArray(final String name)
        throws ParseException
    {
        final ArrayList<String> list = new ArrayList<>();
        String    args;

        // Load daemon.arg[0] ... daemon.arg[n] into the String array.
        //
        while ((args = getProperty(name + "[" + list.size() + "]")) != null) {
            list.add(args);
        }
        return list.toArray(EMPTY_STRING_ARRAY);
    }
}

