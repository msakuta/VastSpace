OUTDIR = Debug
CFLAGS += -I ../clib/include -I ../cpplib/include -I /usr/include/squirrel

${OUTDIR}/gltestplus: ${OUTDIR}\
 ${OUTDIR}/serial.o

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/serial.o: serial.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

