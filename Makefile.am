AUTOMAKE_OPTIONS=foreign
SUBDIRS = ferro

ACLOCAL_AMFLAGS = -I m4
AM_CXXFLAGS=$(BOOST_CPPFLAGS)
AM_LDFLAGS=$(BOOST_LDFLAGS)

EASYBMP_SRCS=EasyBMP.cpp EasyBMP.h EasyBMP_BMP.h EasyBMP_DataStructures.h EasyBMP_VariousBMPutilities.h

RESOURCE_SRCS=CLUTResource.h CLUTResource.cpp PICTResource.h PICTResource.cpp SndResource.h SndResource.cpp $(EASYBMP_SRCS)

EXTRA_DIST=atque.wxg atque.icns Atque-Info.plist EasyBMP_License.txt COPYING.txt atque.xcodeproj/project.pbxproj atque.rc atque.ico README.txt atque.png

INCLUDES=-I$(top_srcdir)/ferro

bin_PROGRAMS=DTB2

DTB2_SOURCES=termview.cpp termview.h atque.h atque.cpp split.cpp split.h merge.cpp merge.h filesystem.h $(RESOURCE_SRCS)
if MAKE_WINDOWS
atque-resources.o:
	@WX_RESCOMP@ -o atque-resources.o -I$(srcdir) $(srcdir)/atque.rc
DTB2_LDADD=atque-resources.o ferro/libferro.a
else
DTB2_LDADD=ferro/libferro.a
endif


