To build it use:
docbook2man jsvc.1.xml

If your docbook2man uses xalan and you are behind a firewall
Modify jsvc.1.xml as below:
+++
Index: jsvc.1.xml
===================================================================
--- jsvc.1.xml	(revision 170004)
+++ jsvc.1.xml	(working copy)
@@ -1,6 +1,6 @@
 <?xml version="1.0" encoding="utf-8"?>
 <!DOCTYPE refentry PUBLIC "-//OASIS//DTD DocBook XML V4.1.2//EN"
-                   "http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd">
+                   "docbookx.dtd">
 <refentry id='jsvc1'>
   <refmeta>
     <refentrytitle>JSVC</refentrytitle>
+++
Use fetch.sh to get docbook files.
