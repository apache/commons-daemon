Procrun

Features:

1. Redirect console applications
2. Running redirected applications as services
3. Console or Windows GUI application mode to avoid any
   temporarily flashing console windows in case the procrun
   is invoked by a GUI program.
4. Running Java applications using default or specified JVM
5. Service management Install/Start/Stop/Delete/Update


Invocation:

procrun [action] [options] [parameters]

Actions:
Actions are two letter codes encapsulated inside double slashes followed
by service name. The service name has to be without spaces.

//IS//<serviceName> Install the service 
//RS//<serviceName> Run service
                    called only when running as service. Do not call that
                    directly.
//DS//<serviceName> Delete the service
//TS//<serviceName> Test the service
                    Run service as ordinary console application.
//US//<serviceName> Update service
                    Change services startup parameters
//GT//<serviceName> Test (Run) the service using windows try
                    The service will run and put the icon in the
                    system try.
//GD//<serviceName> Test (Run) the service using windows try
                    The service will run and and show console window.


Options:
Options are used for install and update actions and are written to the
serviceName registry parameters.
Options are followed with option parameter. If the option parameter
needs spaces it has to be quoted.

--DisplayName   <service display name>
                This is the name shown in the Windows Services
                manager.
--Description   <service description>
                This is the service description shown in the windows
                Services manager.

--ImagePath     <full path to process executable>
                Full path to the executable to be run as service,
                or name of the Java Class Path. In cases when the
                service runs JVM or Java binary this is the
                parameter passed to -Djava.class.path option.

--Arguments     <arguments passed to the service>
                Arguments are enclosed inside double quotation
                marks and passed to the service image

--WorkingPath   <services working path>
                Sets the working path to the desired value.

--Java          [auto|java|path to the jvm.dll]
                auto:
                    Auto will cause to load the default JVM read
                    read from registry.
                java[w]:
                    The default java.exe or javaw.exe will be
                    located and executed with
                    -Djava.class.path=ImagePath
                If neither java or auto are specified then this
                parameter is treated as full path to the jvm.dll

--JavaOptions   <list of java options>
                This is the list of options to be passed to the JVM
                The options are separated using hash (#) simbol.
                For Example:
                -Xmx=100M#-Djava.compiler=NONE
                                
--StartupClass  <class name>
                Class name that will be called if started from JVM
                when the applications starts.
                The method name is separated by semicolon after the
                class name.
                The parameters passed to the class method are semicolon
                separated values after the method name.
                For example:
                org/apache/jk/apr/TomcatStarter;Main;start
                
--ShutdownClass <class name>
                Class name that will be called if started from JVM
                when the applications stops. The class has to have
                the method Main.
                The method name is separated by semicolon after the
                class name.
                The parameters passed to the class method are semicolon
                separated values after the method name.
                For example:
                org/apache/jk/apr/TomcatStarter;Main;stop;some;dummy;params
                
--StdInputFile  <path to the file>
                The file that will be read an passed as standard
                input stream to the redirected application
--StdOutputFile <path to the file>
                Path to the redirected stdout.
--StdErrorFile  <path to the file>
                Path to the redirected stderr.

--Startup       <auto|manual>
                The services startup mode Automatic or Manual.
                Default value is auto.

--User          <username>
                The User account used for launching redirected process.

--Password      <password>
                The password of User account used for launching
                redirected process.

--Install       <procrun.exe>
                Used as Service manager ImagePath when installing
                service from installation program using procrunw.
                                
Examples:

Installing Tomcat as service:
    procrun //IS//Tomcat4 --DisplayName "Tomcat 4.1.19" \
            --Description "Tomcat 4.1.19 LE JDK 1.4 http://jakarta.apache.org" \
            --ImagePath "c:\devtools\tomcat\41\bin\bootstrap.jar" \
            --StartupClass org.apache.catalina.startup.Bootstrap;main;start \
            --ShutdownClass org.apache.catalina.startup.Bootstrap;main;stop \
            --Java auto

Installing Tomcat as service without popup console window:
    procrunw //IS//Tomcat4 --DisplayName "Tomcat 4.1.19" \
            --Description "Tomcat 4.1.19 LE JDK 1.4 http://jakarta.apache.org" \
            --Install "c:\devtools\tomcat\41\bin\procrun.exe"

After installing you may call update service to add or change service
parameters:    
    procrunw //US//Tomcat4 \
            --ImagePath "c:\devtools\tomcat\41\bin\bootstrap.jar" \            
            --StartupClass org.apache.catalina.startup.Bootstrap;main;start \
            --ShutdownClass org.apache.catalina.startup.Bootstrap;main;stop \
            --Java auto

Notice:
    You may change all parameters except service name and Install image.
