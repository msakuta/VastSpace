ifeq "$d" "y"
OUTDIR = Debug
CFLAGS += -I ../clib/include -I include -D_DEBUG -g
else
OUTDIR = Release
CFLAGS += -I ../clib/include -I include -DNDEBUG -O3
endif

CFLAGS += -fPIC

${OUTDIR}/cpplib.a: ${OUTDIR}\
 ${OUTDIR}/cpplib.a(dstring.o)\
 ${OUTDIR}/cpplib.a(vec3.o)\
 ${OUTDIR}/cpplib.a(vec4.o)\
 ${OUTDIR}/cpplib.a(quat.o)\
 ${OUTDIR}/cpplib.a(mat4.o)
# ${OUTDIR}/cpplib.a(glcull.o)

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/cpplib.a(dstring.o): src/dstring.cpp include/cpplib/dstring.h
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUTDIR}/cpplib.a(vec3.o): src/vec3.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUTDIR}/cpplib.a(vec4.o): src/vec4.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUTDIR}/cpplib.a(quat.o): src/quat.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUTDIR}/cpplib.a(mat4.o): src/mat4.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $% && $(AR) r $@ $% && $(RM) $%
${OUTDIR}/cpplib.a(glcull.o): src/gl/cull.cpp
	${CC} $(CFLAGS) $(CPPFLAGS) -I include -c $< -o $% && $(AR) r $@ $% && $(RM) $%

# Implicit rule for making test programs.
tests/dstrtest: tests/dstrtest.cpp ${OUTDIR}/cpplib.a
	${CC} ${CFLAGS} ${CPPFLAGS} $^ -o $@ -lstdc++

.PHONY: clean

clean:
	rm ${OUTDIR}/cpplib.a
	rm tests/dstrtest

