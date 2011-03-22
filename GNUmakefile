OUTDIR = Debug
CFLAGS += -I ../clib/include -I ../cpplib/include -I /usr/include/squirrel -I ../squirrel3/include -I /usr/include/bullet

depends = $(patsubst %:,,$(subst \ ,,$(shell $(CC) $(CFLAGS) $(CPPFLAGS) -I include -MM $(1))))

${OUTDIR}/gltestplus: ${OUTDIR}\
 ${OUTDIR}/serial.o\
 ${OUTDIR}/serial_util.o\
 ${OUTDIR}/sqadapt.o\
 ${OUTDIR}/Entity.o

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/serial.o: $(call depends,serial.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/serial_util.o: $(call depends,serial_util.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/sqadapt.o: $(call depends,sqadapt.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Entity.o: $(call depends,Entity.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

