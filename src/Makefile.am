
INCLUDES =						\
	-DDATADIR=\""$(datadir)"\"			\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"		\
	-DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"	\
	$(PACKAGE_CFLAGS)

bin_PROGRAMS = sirius

sirius_SOURCES =					\
	sirius.c sirius.h				\
	board.c board.h					\
	book.c board.h					\
 	evaluation.c evaluation.h			\
	gnome-canvas-pimage.c gnome-canvas-pimage.h	\
	hashtable.c hashtable.h				\
	search.c search.h				\
	timer.c timer.h

sirius_LDADD = $(PACKAGE_LIBS)

