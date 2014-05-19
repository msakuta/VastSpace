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
		BULLET_INCLUDE=../../bullet/src
	else
		BULLET_INCLUDE=/usr/include/bullet
	endif
endif

ifndef BULLET_LIB
	ifeq "$(USE_LOCAL_BULLET)" "y"
		BULLET_LIB=-L../../bullet/src/LinearMath \
			-L../../bullet/src/BulletCollision \
			-L../../bullet/src/BulletDynamics
	else
		BULLET_LIB=
	endif
endif

CFLAGS += -I ../../include -I ../../clib/include -I ../../cpplib/include -I ../../squirrel3/include -I ${BULLET_INCLUDE}
CFLAGS += -D DEDICATED
CFLAGS += -fPIC

# Ignore DLL export definition in gcc build. 
CFLAGS += '-DVASTSPACE_EXPORT='

depends = $(patsubst %:,,$(subst \ ,,$(shell $(CC) $(CFLAGS) $(CPPFLAGS) -I ../include -MM $(1))))

objects = ${OUTDIR}/Soldier.o\
 ${OUTDIR}/TorusStation.o\
 ${OUTDIR}/GimbalTurret.o\
 ../../clib/Release/clib.a\
 ../../cpplib/Release/cpplib.a

all: ../../${OUTDIR}/vastspace.so

../../${OUTDIR}/vastspace.so: ${OUTDIR} ${objects}
	${CC} ${CFLAGS} $(CPPFLAGS) -shared ${objects} -o $@ ${BULLET_LIB} \
	-lstdc++ -lm -lBulletCollision -lBulletDynamics -lLinearMath -ldl -lrt -lpthread

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/Soldier.o: $(call depends,Soldier.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/TorusStation.o: $(call depends,TorusStation.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@
${OUTDIR}/GimbalTurret.o: $(call depends,GimbalTurret.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

clean:
	rm ${OUTDIR} -r


