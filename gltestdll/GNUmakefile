ifeq "$d" "y"
OUTDIR = Debug
CFLAGS += -g
else
OUTDIR = Release
CFLAGS += -O3 -g
endif
ifndef BULLET_INCLUDE
BULLET_INCLUDE=/usr/include/bullet
endif
CFLAGS += -I ../include -I ../../clib/include -I ../../cpplib/include -I ../../SQUIRREL3/include -I ${BULLET_INCLUDE}
CFLAGS += -D DEDICATED

depends = $(patsubst %:,,$(subst \ ,,$(shell $(CC) $(CFLAGS) $(CPPFLAGS) -I ../include -MM $(1))))

objects = ${OUTDIR}/Soldier.o\
 ../../clib/Release/clib.a\
 ../../cpplib/Release/cpplib.a

all: ../${OUTDIR}/gltestdll.so

../${OUTDIR}/gltestdll.so: ${OUTDIR} ${objects}
	${CC} ${CFLAGS} $(CPPFLAGS) -shared ${objects} -o $@ -lstdc++ -lm -lBulletCollision -lBulletDynamics -lLinearMath -ldl -lrt -lpthread

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/Soldier.o: $(call depends,Soldier.cpp)
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $@

clean:
	rm ${OUTDIR} -r


