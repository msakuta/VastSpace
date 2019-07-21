ifeq "$d" "y"
OUTDIR = Debug
CFLAGS += -g
else
OUTDIR = Release
CFLAGS += -O3 -g
endif

USE_LOCAL_BULLET=y

ifndef BULLET_INCLUDE
	ifeq "$(USE_LOCAL_BULLET)" "y"
		BULLET_INCLUDE=bullet/src
	else
		BULLET_INCLUDE=/usr/include/bullet
	endif
endif

ifndef BULLET_LIB
	ifeq "$(USE_LOCAL_BULLET)" "y"
		BULLET_LIB=-Lbullet/bin
	else
		BULLET_LIB=
	endif
endif

CFLAGS += -I clib/include -I cpplib/include -I squirrel3/include \
 -I lpng -I zlib -I ${BULLET_INCLUDE} -DBT_USE_DOUBLE_PRECISION
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
 ${OUTDIR}/RoundAstrobj.o\
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
 ${OUTDIR}/mqo.o\
 ${OUTDIR}/mqoadapt.o\
 ${OUTDIR}/Mesh.o\
 ${OUTDIR}/png.o\
 ${OUTDIR}/sdnoise1234.o\
 ${OUTDIR}/simplexnoise1234.o\
 ${OUTDIR}/simplexnoise1234d.o\
 ./clib/Release/clib.a\
 ./cpplib/Release/cpplib.a\
 ./zlib/libz.a\
 ./squirrel3/lib/libsquirrel.a\
 ./squirrel3/lib/libsqstdlib.a\
 ./lpng/libpng.a

gltestdll_objects = gltestdll/${OUTDIR}/Soldier.o\
 ./clib/Release/clib.a\
 ./cpplib/Release/cpplib.a

LIB_LINEAR_MATH = LinearMath_gmake_x64_release
LIB_BULLET_COLLISION = BulletCollision_gmake_x64_release
LIB_BULLET_DYNAMICS = BulletDynamics_gmake_x64_release

all: ${OUTDIR}/gltestplus ${OUTDIR}/vastspace.so

${OUTDIR}/gltestplus: ${OUTDIR} ${objects} libLinearMath libBulletCollision libBulletDynamics
	${CC} ${CFLAGS} $(CPPFLAGS) $(RDYNAMIC) ${objects} -o $@ $(BULLET_LIB) \
	-lstdc++ -lm -l${LIB_BULLET_DYNAMICS} -l${LIB_BULLET_COLLISION} -l${LIB_LINEAR_MATH} -ldl -lrt -lpthread

#${OUTDIR}/gltestdll.so: gltestdll/${OUTDIR} ${gltestdll_objects}
#	${CC} ${CFLAGS} $(CPPFLAGS) -shared ${gltestdll_objects} -o $@ -lstdc++ -lm -lBulletCollision -lBulletDynamics -lLinearMath -ldl -lrt -lpthread

${OUTDIR}/vastspace.so:
	cd mods/vastspace && ${MAKE}

./clib/${OUTDIR}/clib.a:
	cd clib && CFLAGS="-I ../zlib" ${MAKE} 

./cpplib/${OUTDIR}/cpplib.a:
	cd cpplib && ${MAKE}

./zlib/libz.a:
	cd zlib && ${MAKE}
	
./lpng/libpng.a:
	cd lpng && ${MAKE} -f scripts/makefile.linux

./squirrel3/lib/libsquirrel.a ./squirrel3/lib/libsqstdlib.a:
	-mkdir ./squirrel3/lib ./squirrel3/bin # Squirrel's makefile does not try to mkdir
	cd squirrel3 && ${MAKE}

ifeq "$(USE_LOCAL_BULLET)" "y"
libLinearMath: bullet/bin/lib${LIB_LINEAR_MATH}.a
libBulletCollision: bullet/bin/lib${LIB_BULLET_COLLISION}.a
libBulletDynamics: bullet/bin/lib${LIB_BULLET_DYNAMICS}.a
endif

bullet/bin/lib${LIB_LINEAR_MATH}.a bullet/bin/lib${LIB_BULLET_COLLISION}.a bullet/bin/lib${LIB_BULLET_DYNAMICS}.a:
	cd bullet/build3 && ./premake4_linux64 --double gmake && cd gmake && make LinearMath BulletCollision BulletDynamics

${OUTDIR}:
	mkdir ${OUTDIR}

mods/vastspace/${OUTDIR}:
	mkdir mods/vastspace/${OUTDIR}

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
${OUTDIR}/RoundAstrobj.o: $(call depends,RoundAstrobj.cpp)
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
${OUTDIR}/mqo.o: $(call depends,mqo.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/mqoadapt.o: $(call depends,mqoadapt.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/Mesh.o: $(call depends,Mesh.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/png.o: $(call depends,png.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/sdnoise1234.o: $(call depends,noises/sdnoise1234.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/simplexnoise1234.o: $(call depends,noises/simplexnoise1234.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/simplexnoise1234d.o: $(call depends,noises/simplexnoise1234d.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

#gltestdll/${OUTDIR}/Soldier.o: $(call gltestdll_depends,Soldier.cpp)
#	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

rc.zip: shaders/*.fs shaders/*.vs models/*.mqo models/*.nut models/*.bmp models/*.jpg \
models/*.mot \
models/*.png \
textures/*.png \
textures/*.jpg \
textures/*.bmp \
surface/models/*.mqo \
surface/models/*.mot \
surface/models/*.nut
	7z u $@ $^

clean:
	rm ${OUTDIR} -r

.PHONY: ${OUTDIR}/gltestdll.so ./clib/${OUTDIR}/clib.a libLinearMath libBulletCollision libBulletDynamics

