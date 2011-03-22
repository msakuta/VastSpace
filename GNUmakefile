OUTDIR = Debug
CFLAGS += -I ../clib/include -I ../cpplib/include -I /usr/include/squirrel -I ../squirrel3/include -I /usr/include/bullet

${OUTDIR}/gltestplus: ${OUTDIR}\
 ${OUTDIR}/serial.o\
 ${OUTDIR}/serial_util.o\
 ${OUTDIR}/sqadapt.o

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/serial.o: serial.cpp
${OUTDIR}/serial_util.o: serial_util.cpp
${OUTDIR}/sqadapt.o: sqadapt.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

