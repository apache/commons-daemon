import java.io.*;
import java.lang.*;

public class Test implements Runnable {
    public static void main (String args[]) {
        Thread t = new Thread( new Test());
        t.run();
    } 
    public void run(){     
        try {
        System.out.println("Simple Stdout message");
        System.err.println("Simple Stderr message");
        Thread.sleep(5000);
        } catch( Throwable t ) {
            t.printStackTrace(System.err);
        }         
    }
} 