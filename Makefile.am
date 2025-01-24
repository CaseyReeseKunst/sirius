desktopdir = $(datadir)/applications
desktop_in_files = sirius.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

SUBDIRS = po src pixmaps data

EXTRA_DIST = 	autogen.sh		\
	sirius.desktop			\
	sirius.desktop.in		\
	intltool-extract.in		\
	intltool-merge.in		\
	intltool-update.in	

