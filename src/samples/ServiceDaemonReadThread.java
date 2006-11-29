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
import java.io.InputStream;
import java.io.IOException;
import java.lang.Thread;
import java.io.BufferedReader;
import java.io.InputStreamReader;

public class ServiceDaemonReadThread extends Thread {
    private BufferedReader in;
    ServiceDaemonReadThread(InputStream in) {
            this.in = new BufferedReader(new InputStreamReader(in));
        }
    public void run() {
        String buff;
        for (;;) {
            try {
                buff = in.readLine();
                if (buff == null) break;
                System.err.print(in.readLine());
            } catch (IOException ex) {
                break; // Exit thread.
            }
        }
    }
}
