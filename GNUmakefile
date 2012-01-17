OUTDIR = Debug
CFLAGS += -I ../clib/include -I ../cpplib/include -I ../SQUIRREL3/include -I /usr/include/bullet

depends = $(patsubst %:,,$(subst \ ,,$(shell $(CC) $(CFLAGS) $(CPPFLAGS) -I include -MM $(1))))

objects = ${OUTDIR}/serial.o\
 ${OUTDIR}/serial_util.o\
 ${OUTDIR}/sqadapt.o\
 ${OUTDIR}/dstring.o\
 ${OUTDIR}/cmd.o\
 ${OUTDIR}/Entity.o\
 ${OUTDIR}/CoordSys.o\
 ${OUTDIR}/Universe.o\
 ${OUTDIR}/Player.o\
 ${OUTDIR}/Game.o\
 ${OUTDIR}/astro.o\
 ${OUTDIR}/judge.o\
 ${OUTDIR}/war.o\
 ${OUTDIR}/stellar_file.o\
 ${OUTDIR}/dedsvr.o\
 ${OUTDIR}/calc/calc3.o\
 ${OUTDIR}/calc/mathvars.o\
 ../clib/Release/clib.a\
 ../cpplib/Release/cpplib.a\
 ../SQUIRREL3/lib/libsquirrel.a\
 ../SQUIRREL3/lib/libsqstdlib.a\
 
 all: ${OUTDIR}/gltestplus

${OUTDIR}/gltestplus: ${OUTDIR} ${objects}
	${CC} ${CFLAGS} $(CPPFLAGS) ${objects} -o $@ -lstdc++ -lm -lBulletCollision -lBulletDynamics -lLinearMath -ldl

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/serial.o: $(call depends,serial.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/serial_util.o: $(call depends,serial_util.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/sqadapt.o: $(call depends,sqadapt.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/dstring.o: $(call depends,dstring.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/cmd.o: CFLAGS += -I /usr/lib/gcc-lib/i386-
${OUTDIR}/cmd.o: $(call depends,cmd.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Entity.o: $(call depends,Entity.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/CoordSys.o: $(call depends,CoordSys.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Universe.o: $(call depends,Universe.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Player.o: $(call depends,Player.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Game.o: $(call depends,Game.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/astro.o: $(call depends,astro.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/judge.o: $(call depends,judge.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/war.o: $(call depends,war.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/stellar_file.o: $(call depends,stellar_file.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/dedsvr.o: $(call depends,dedsvr.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/calc/calc3.o: $(call depends,calc/calc3.c)
	mkdir -p ${OUTDIR}/calc
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/calc/mathvars.o: $(call depends,calc/mathvars.c)
	mkdir -p ${OUTDIR}/calc
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

clean:
	rm Debug -r


