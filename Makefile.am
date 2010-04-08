## Process this file with automake to produce Makefile.in

SUBDIRS = src data po

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/sounds; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/sounds/$(PACKAGE); \
	  for sound in $(srcdir)/sounds/*; do \
	    if test -f $$sound; then \
	      $(INSTALL_DATA) $$sound $(DESTDIR)$(datadir)/sounds/$(PACKAGE); \
	    fi \
	  done \
	fi
	-$(mkinstalldirs) $(DESTDIR)$(scoredir)
	touch $(DESTDIR)$(scoredir)/gweled.easy.scores
	-chown $(scores_user):$(scores_group) $(DESTDIR)$(scoredir)/gweled.easy.scores
	-chmod 664 $(DESTDIR)$(scoredir)/gweled.easy.scores
	-if test "x$(setgid)" = "xtrue"; then chgrp $(scores_group) $(DESTDIR)$(bindir)/gweled && chmod 2555 $(DESTDIR)$(bindir)/gweled ; fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d sounds; then \
	  mkdir $(distdir)/sounds; \
	  for sound in sounds/*; do \
	    if test -f $$sound; then \
	      cp -p $$sound $(distdir)/sounds; \
	    fi \
	  done \
	fi

