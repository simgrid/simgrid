SUBDIRS= doc src 
DISTCLEANFILES =  *~  

ACLOCAL = aclocal -I tools/acmacro -I /usr/share/aclocal/gnome2-macros
AUTOMAKE_OPTIONS = gnu

EXTRA_DIST = bootstrap COPYING INSTALL NEWS README TODO AUTHORS                        
#	Cflags.Makefile.am                      \
#	doc/FAQ.txt                             \
#	doc/footer.html                         \
#	doc/html                                \
#	doc/index.doc                           \
#	doc/Makefile.Example.am                 \
#	doc/Makefile.Example.in                 \
#	tools/colorize.pl                       \
#	tools/index_create.pl                   \
#	tools/acmacro


TAGS: 
	etags `find . -name "*.[ch]"`

tags: TAGS

publish: dist
	 @echo "----[ Put the tarball @VERSION@ on gcl ]----"
	 scp @PACKAGE@-@VERSION@.tar.gz gcl.ucsd.edu:/home/www/simgrid/dl/
	 scp -r doc/html/* gcl.ucsd.edu:/home/www/simgrid/
	 scp ChangeLog gcl.ucsd.edu:/home/www/simgrid/dl/
	 @echo "----[ Put the tarball @VERSION@ on the local filesystem ]----"
	 cp @PACKAGE@-@VERSION@.tar.gz ~/public_html/simgrid2
	 cp -r doc/html ~/public_html/simgrid2/doc/
	 cp ChangeLog ~/public_html/simgrid2/

mail:
	 @ver=`dpkg-parsechangelog -lChangeLog| egrep '^Version: ' | sed 's/Version: //'`;\
	  (echo "Hello," ; \
	   echo; \
           echo "A new version of GRAS is available. Here is the changelog:";\
	   echo;echo;\
           dpkg-parsechangelog -lChangeLog ;\
           echo;echo; \
	   echo "It is available from the official website:";\
	   echo "  http://grail.sdsc.edu/simgrid/";\
	   echo;echo "Cheers, Mt.") | \
          mail -e \
	       -a "From: Martin.Quinson@ens-lyon.fr" \
	       -s "New version of GRAS ($$ver)" \
	       martin.quinson@ens-lyon.fr,arnaud.legrand@ens-lyon.fr


splint:
	splint `find src examples -name '*.c'` +matchanyintegral -warnposix +boolint -Inws_portability/Include/ -Isrc/
