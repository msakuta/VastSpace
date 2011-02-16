

# switch it back and forth by giving an argument "d=y" to make.
ifeq "$d" "y"
DEBUG = -ggdb -D _DEBUG
OUTDIR = Debug
else
DEBUG = -s -O3 -D NDEBUG # essentially release
OUTDIR = Release
endif

CFLAGS = ${DEBUG} -I include
OUT = ${OUTDIR}/clib.a
default: ${OUT}

${OUT}: ${OUTDIR}\
 ${OUT}(UnZip.o)\
 ${OUT}(amat3.o)\
 ${OUT}(amat4.o)\
 ${OUT}(aquat.o)\
 ${OUT}(aquatrot.o)\
 ${OUT}(avec3.o)\
 ${OUT}(cfloat.o)\
 ${OUT}(dstr.o)\
 ${OUT}(rseq.o)\
 ${OUT}(timemeas.o)\
 ${OUT}(cs.o)\
 ${OUT}(cs2.o)\
 ${OUT}(cs2x.o)\
 ${OUT}(cs2xcut.o)\
 ${OUT}(cs2xedit.o)\
 ${OUT}(lzc.o)\
 ${OUT}(lzuc.o)\
 ${OUT}(suf.o)\
 ${OUT}(sufreloc.o)
# ${OUT}(sufdraw.o)
# ${OUT}(wavsound.o)
# ${OUT}(UnZip.o)\

${OUTDIR}:
	mkdir $@

${OUT}(UnZip.o): src/UnZip.c
${OUT}(amat3.o): src/amat3.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(amat4.o): src/amat4.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(aquat.o): src/aquat.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(aquatrot.o): src/aquatrot.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(avec3.o): src/avec3.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(cfloat.o): src/cfloat.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(dstr.o): src/dstr.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(rseq.o): src/rseq.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(timemeas.o): src/timemeas.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
#${OUT}(UnZip.o): src/UnZip.c
#	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(wavsound.o): src/wavsound.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(cs.o): src/colseq/cs.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(cs2.o): src/colseq/cs2.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(cs2x.o): src/colseq/cs2x.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(cs2xcut.o): src/colseq/cs2xcut.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(cs2xedit.o): src/colseq/cs2xedit.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(lzc.o): src/lzw/lzc.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(lzuc.o): src/lzw/lzuc.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUT}(suf.o): src/suf/suf.c
${OUT}(sufdraw.o): src/suf/sufdraw.c
${OUT}(sufreloc.o): src/suf/sufreloc.c
	${CC} $(CFLAGS) $(CPPFLAGS) -c $< -o $% && $(AR) r $@ $% && $(RM) $%


