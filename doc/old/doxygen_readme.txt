This directory contains documentation on the C++ code

In order to generate documentation, do this:
	cd ..
	make doxygen

Make sure you have the following pakcages installed:
	doxygen...rpm (http://ww.doxygen.org/)
	graphviz...rpm (http://www.research.att.com/sw/tools/graphviz/)

You can can also get them at http://rpmfind.net/

Change the EXTRACT_ALL setting in the file freecnc-doxygen.conf to NO for generating documentation for only ...AnimEvent classes or to YES for all classes.
