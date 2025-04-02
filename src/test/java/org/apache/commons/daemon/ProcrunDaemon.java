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

import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;

class ProcrunDaemon {

    /* The server does nothing more than the following:
     - 1 Just stop and do nothing more.
     - 2 wait 60 second and stop.
     */
    public static void server() {
        System.out.println("TestServ");
        ServerSocket serverSocket = null;
        try {
            serverSocket = new ServerSocket(4444, 2);
        } catch (IOException e) {
            System.out.println("Could not listen on port: 4444");
            System.exit(-1);
        }

        /* Wait fo clients */
        for (;;) {
            Socket clientSocket = null;
            try {
                clientSocket = serverSocket.accept();
                ProcessClient(clientSocket);
            } catch (IOException e) {
                System.out.println("Accept or Client failed: 4444");
                System.exit(-1);
            }
        }
    }

    public static void ProcessClient(Socket clientSocket) {
        byte[] buf = new byte[1024];
        int num;
        InputStream clientInput = null;
        try {
            clientInput = clientSocket.getInputStream();
            while ((num = clientInput.read(buf)) != -1) {
                System.out.println(".");
                if (buf[0] == '0') {
                    /* We just use 0 to check that the service is started */
                    System.out.println("0");
                    return;
                } else if (buf[0] == '1') {
                    System.out.println("1");
                    System.exit(0);
                } else if (buf[0] == '2') {
                    System.out.println("2");
                    try {
                        Thread.currentThread().sleep(60000);
                    } catch (Exception ex) {
                        System.out.println(ex);
                    }
                    System.exit(0);
                }
            }
            System.out.println("num " + num);
        } catch (IOException e) {
            System.out.println("Could not read from port: 4444");
            System.exit(-1);
        }

        try {
            while ((num = clientInput.read(buf)) != -1) {
                System.out.println(".");
            }
            System.out.println("num " + num);
        } catch (IOException e) {
            System.out.println("Could not read from port: 4444");
            System.exit(-1);
        }

    }

    /* client piece */
 /* The client processes the command and sends it to server */
    public static void client(String string) {
        System.out.println(string);
        if (string.charAt(0) == '3' || string.charAt(0) == '4') {
            /* Wait 60 seconds in the client, then sends the command to the server */
            Runtime runtime = Runtime.getRuntime();
            try {
                Thread.currentThread().sleep(60000);
            } catch (Exception ex) {
                System.out.println(ex);
            }
            if (string.charAt(0) == '3') {
                string = "1";
            } else if (string.charAt(0) == '4') {
                string = "2"; /* The server will wait 60 seconds after the stop command */
            }
        }
        try {
            Socket connection = new Socket("127.0.0.1", 4444);
            OutputStream os = connection.getOutputStream();
            byte[] value = string.getBytes();
            os.write(value);
            os.flush();
            connection.close();
        } catch (Exception ex) {
            System.out.println(ex);
            System.exit(1);
        }
        System.exit(0);
    }

    /* client server test for procrun */
    public static void main(String[] argv) {
        if (argv.length != 0) {
            /* Just send the command to the server */
            try {
                System.setOut(new PrintStream(new BufferedOutputStream(new FileOutputStream("client.txt")), true));
            } catch (Exception ex) {
                System.out.println(ex);
            }
            client(argv[0]);
        } else {
            /* start server */
            try {
                System.setOut(new PrintStream(new BufferedOutputStream(new FileOutputStream("server.txt")), true));
            } catch (Exception ex) {
                System.out.println(ex);
            }
            server();
        }
    }

    /* For the jvm mode */
    private static volatile Thread thrd; // start and stop are called from different threads

    public static void start(String[] argv) {
        try {
            System.setOut(new PrintStream(new BufferedOutputStream(new FileOutputStream("jvm.txt")), true));
        } catch (Exception ex) {
            System.out.println(ex);
            System.exit(1);
        }
        if (argv.length != 0) {
            System.out.println("start: " + argv[0]);
        } else {
            System.out.println("start no argv");
        }
        thrd = new Thread() {
            public void run() {
                server();
            }
        };
        thrd.start();
        while (thrd.isAlive()) {
            try {
                thrd.join();
            } catch (InterruptedException ie) {
                System.out.println(ie);
            }
        }
        System.out.println("start Thread finished");
    }

    public static void stop(String[] argv) {
        if (argv.length != 0) {
            System.out.println("stop: " + argv[0]);
            String string = argv[0];
            if (string.charAt(0) == '3' || string.charAt(0) == '4') {
                /* Wait 60 seconds in the stop */
                Runtime runtime = Runtime.getRuntime();
                try {
                    Thread.currentThread().sleep(60000);
                } catch (Exception ex) {
                    System.out.println(ex);
                }
            }
        } else {
            System.out.println("stop no argv");
        }
        /* just stop the thread! */
        if (thrd != null) {
            System.out.println("stop: interrupt Thread");
            thrd.interrupt();
        } else {
            System.out.println("stop: Oops no Thread");
        }
    }
}
