ifeq "$d" "y"
OUTDIR = Debug
CFLAGS += -g
else
OUTDIR = Release
CFLAGS += -O3 -g
endif

ifndef BULLET_INCLUDE
BULLET_INCLUDE=bullet/src
endif

ifndef BULLET_LIB
BULLET_LIB=
endif

CFLAGS += -I clib/include -I cpplib/include -I squirrel3/include \
 -I lpng -I zlib -I ${BULLET_INCLUDE}
CFLAGS += -D DEDICATED
CPPFLAGS += -std=c++11
CC = gcc
#gltestdll_OUTDIR = gltestdll/${OUTDIR}

ifdef MINGW
RDYNAMIC =
else
RDYNAMIC = -rdynamic
endif

depends = $(patsubst %:,,$(subst \ ,,$(shell $(CC) $(CFLAGS) $(CPPFLAGS) -I include -MM src/$(1))))
#gltestdll_depends = $(patsubst %:,,$(subst \ ,,$(shell $(CC) $(CFLAGS) $(CPPFLAGS) -I include -MM gltestdll/$(1))))

objects = ${OUTDIR}/serial.o\
 ${OUTDIR}/serial_util.o\
 ${OUTDIR}/sqadapt.o\
 ${OUTDIR}/dstring.o\
 ${OUTDIR}/cmd.o\
 ${OUTDIR}/BinDiff.o\
 ${OUTDIR}/Observable.o\
 ${OUTDIR}/Entity.o\
 ${OUTDIR}/CoordSys.o\
 ${OUTDIR}/Universe.o\
 ${OUTDIR}/Player.o\
 ${OUTDIR}/Game.o\
 ${OUTDIR}/astro.o\
 ${OUTDIR}/TexSphere.o\
 ${OUTDIR}/judge.o\
 ${OUTDIR}/war.o\
 ${OUTDIR}/stellar_file.o\
 ${OUTDIR}/Application.o\
 ${OUTDIR}/Server.o\
 ${OUTDIR}/dedsvr.o\
 ${OUTDIR}/Bullet.o\
 ${OUTDIR}/BeamProjectile.o\
 ${OUTDIR}/Missile.o\
 ${OUTDIR}/Sceptor.o\
 ${OUTDIR}/Worker.o\
 ${OUTDIR}/Defender.o\
 ${OUTDIR}/Docker.o\
 ${OUTDIR}/Builder.o\
 ${OUTDIR}/SqInitProcess.o\
 ${OUTDIR}/Autonomous.o\
 ${OUTDIR}/Warpable.o\
 ${OUTDIR}/Frigate.o\
 ${OUTDIR}/Beamer.o\
 ${OUTDIR}/arms.o\
 ${OUTDIR}/LTurret.o\
 ${OUTDIR}/Assault.o\
 ${OUTDIR}/Attacker.o\
 ${OUTDIR}/Destroyer.o\
 ${OUTDIR}/Shipyard.o\
 ${OUTDIR}/RStation.o\
 ${OUTDIR}/png.o\
 ${OUTDIR}/calc/calc3.o\
 ${OUTDIR}/calc/mathvars.o\
 ${OUTDIR}/calc/calc0.o\
 ./clib/Release/clib.a\
 ./cpplib/Release/cpplib.a\
 ./zlib/libz.a\
 ./squirrel3/lib/libsquirrel.a\
 ./squirrel3/lib/libsqstdlib.a\
 ./lpng/libpng.a

gltestdll_objects = gltestdll/${OUTDIR}/Soldier.o\
 ./clib/Release/clib.a\
 ./cpplib/Release/cpplib.a

all: ${OUTDIR}/gltestplus #${OUTDIR}/gltestdll.so

${OUTDIR}/gltestplus: ${OUTDIR} ${objects}
	${CC} ${CFLAGS} $(CPPFLAGS) $(RDYNAMIC) ${objects} -o $@ $(BULLET_LIB) \
	-lstdc++ -lm -lBulletDynamics -lBulletCollision -lLinearMath -ldl -lrt -lpthread

#${OUTDIR}/gltestdll.so: gltestdll/${OUTDIR} ${gltestdll_objects}
#	${CC} ${CFLAGS} $(CPPFLAGS) -shared ${gltestdll_objects} -o $@ -lstdc++ -lm -lBulletCollision -lBulletDynamics -lLinearMath -ldl -lrt -lpthread

${OUTDIR}/gltestdll.so:
	cd gltestdll && ${MAKE}

./clib/${OUTDIR}/clib.a:
	cd clib && ${MAKE}

./cpplib/${OUTDIR}/cpplib.a:
	cd cpplib && ${MAKE}

./zlib/libz.a:
	cd zlib && ${MAKE}
	
./lpng/libpng.a:
	cd lpng && ${MAKE}

./squirrel3/lib/libsquirrel.a ./squirrel3/lib/libsqstdlib.a:
	-mkdir ./squirrel3/lib ./squirrel3/bin # Squirrel's makefile does not try to mkdir
	cd squirrel3 && ${MAKE}

${OUTDIR}:
	mkdir ${OUTDIR}

gltestdll/${OUTDIR}:
	mkdir gltestdll/${OUTDIR}

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
${OUTDIR}/BinDiff.o: $(call depends,BinDiff.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Observable.o: $(call depends,Observable.cpp)
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
${OUTDIR}/TexSphere.o: $(call depends,TexSphere.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/judge.o: $(call depends,judge.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/war.o: $(call depends,war.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/stellar_file.o: $(call depends,stellar_file.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Application.o: $(call depends,Application.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Server.o: $(call depends,Server.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/dedsvr.o: $(call depends,dedsvr.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Bullet.o: $(call depends,Bullet.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/BeamProjectile.o: $(call depends,BeamProjectile.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Missile.o: $(call depends,Missile.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Sceptor.o: $(call depends,Sceptor.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Worker.o: $(call depends,Worker.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Defender.o: $(call depends,Defender.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Docker.o: $(call depends,Docker.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Builder.o: $(call depends,Builder.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/SqInitProcess.o: $(call depends,SqInitProcess.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Autonomous.o: $(call depends,Autonomous.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Warpable.o: $(call depends,Warpable.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Frigate.o: $(call depends,Frigate.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Beamer.o: $(call depends,Beamer.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/arms.o: $(call depends,arms.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/LTurret.o: $(call depends,LTurret.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Assault.o: $(call depends,Assault.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Attacker.o: $(call depends,Attacker.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Destroyer.o: $(call depends,Destroyer.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Shipyard.o: $(call depends,Shipyard.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/RStation.o: $(call depends,RStation.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/png.o: $(call depends,png.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/calc/calc3.o: $(call depends,calc/calc3.c)
	mkdir -p ${OUTDIR}/calc
	${CC} $(CFLAGS) -I include -c $< -o $@
${OUTDIR}/calc/mathvars.o: $(call depends,calc/mathvars.c)
	mkdir -p ${OUTDIR}/calc
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/calc/calc0.o: $(call depends,calc/calc0.c)
	mkdir -p ${OUTDIR}/calc
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

#gltestdll/${OUTDIR}/Soldier.o: $(call gltestdll_depends,Soldier.cpp)
#	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

rc.zip: shaders/*.fs shaders/*.vs models/*.mqo models/*.nut models/*.bmp models/*.jpg \
models/*.mot \
models/*.png \
textures/*.png \
textures/*.jpg \
textures/*.bmp \
gltestdll/models/*.mqo \
gltestdll/models/*.mot \
gltestdll/models/*.nut \
gltestdll/models/*.png \
gltestdll/models/*.jpg \
surface/models/*.mqo \
surface/models/*.mot \
surface/models/*.nut
	7z u $@ $^

clean:
	rm ${OUTDIR} -r

.PHONY: ${OUTDIR}/gltestdll.so ./clib/${OUTDIR}/clib.a

