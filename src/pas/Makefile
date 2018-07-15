     PC = pc
     AR = ar
 RANLIB = ranlib
DESTDIR = /usr/local/gli
  CHMOD = chmod
     RM = rm

.SUFFIXES: .a .o .pen .p

.p.o:
	$(PC) -c $<
.p.pen:
	$(PC) -c -env $@ $<

pascal: pasrtl.a pasrtl.o rtl.pen gusdefs.pen gksdefs.pen

pasrtl.a: gus.o gks.o pasrtl.o
	$(AR) crv $@ $?
	$(RANLIB) $@

install: pascal
	@$(CHMOD) 644 *.a
	$(CP) pasrtl.a $(DESTDIR)
	@$(CHMOD) 644 *.pen
	$(CP) *.pen $(DESTDIR)
	@$(CHMOD) 644 g*.p
	$(CP) g*.p $(DESTDIR)

clean:
	@$(RM) -f *.a *.o *.pen
	@$(RM) -f *.bak

