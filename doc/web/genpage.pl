#!/usr/bin/perl -w
# 
# Copyright (c) 2001-2003 Regents of the University of California. 
# All rights reserved.                                             
#
# See the file LICENSE included in this distribution for details.  
#
# $Id: genpage.pl,v 1.3 2006/07/05 16:59:56 srhea Exp $ 

use strict;
  
my $title = shift;
my $content_file = shift;
open (FILE, "$content_file") or die "Could not open $content_file";

print<<EOF;
<!-- 

  Copyright (c) 2006-2008 Sean C. Rhea (srhea\@srhea.net)
  All rights reserved.                                             

  This file was automatically generated by genpage.pl.  To change it,
  please edit the content file, $content_file.

-->

<html>
<head>
<title>Golden Cheetah: Cycling Performance Software for Linux, Mac OS X, and Windows</title>
 <meta name="keywords" content="powertap srm linux mac cycling performance">
<style type='text/css'>
li {
    margin: .5em 0
}
body {
    color: #000000;
    background: #ffffff;
}
:link {
      color: #5e431b;
}
:visited {
      color: #996e2d;
}
</style>
</head>

<table width="95%" border="0" width="100%" cellspacing="10">
<tr>

<!-- Left Column -->
<td width="175" valign="top">
<center>
<img src="logo.jpg" width="175" height="175" alt="Picture of Cheetah">

<p>  <b><a href="index.html">Introduction</a></b>
<br> <b><a href="download.html">Download</a></b>
<br> <b><a href="screenshots.html">Screenshots</a>
<br> <b><a href="wiki.html">Wiki</a>
<br> <b><a href="users-guide.html">User's Guide</a>
<br> <b><a href="developers-guide.html">Developer's Guide</a>
<br> <b><a href="faq.html">FAQ</a>
<br> <b><a href="wishlist.html">Wish List</a>
<br> <b><a href="license.html">License</a></b>
<br> <b><a href="contrib.html">Contributors</a></b>
<br> <b><a href="search.html">Search</a></b>
<br> <b><a href="mailing-list.html">Mailing List</a></b>
<br> <b><a href="bug-tracker.html">Bug Tracker</a></b>

</center>
</td>
<!-- End of Left Column -->

<!-- Right Column -->
<td align="left" valign="top">
<table width="100%" cellspacing="10">
<tr align="center"><td>

<p>
<p>
<big><big><big><b><font face="arial,helvetica,sanserif">Golden
Cheetah</font></b></big></big></big>
<br>
<big><font face="arial,helvetica,sanserif">
Cycling Performance Software for Linux, Mac OS X, and Windows
</font></big>
<p>
</td></tr>

<tr><td bgcolor="#5e431b">
<font color="#f8d059" face="arial,helvetica,sanserif">
<big><strong>$title</strong></big>
</font>
</td></tr>

<tr><td>
EOF

my $match = "\\\$" . "Id:.* (\\d\\d\\d\\d\\/\\d\\d\\/\\d\\d "
    . "\\d\\d:\\d\\d:\\d\\d) .*\\\$";

my $last_mod;
while (<FILE>) {
    
    if (m/$match/) {
        $last_mod = $1;
    }
    print;
}
close (FILE);

#if (defined $last_mod) {
#    print "<p><hr><em>Last modified $last_mod.</em>\n";
#}

print<<EOF;

</tr></td>
</table>
</td>
<!-- End of Right Column -->

</tr>
</table>

</body>
</html>
EOF

