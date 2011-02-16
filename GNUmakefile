OUTDIR = Debug
CFLAGS += -I ../clib/include

${OUTDIR}/cpplib.a: ${OUTDIR}\
 ${OUTDIR}/cpplib.a(dstring.o)\
 ${OUTDIR}/cpplib.a(vec3.o)\
 ${OUTDIR}/cpplib.a(vec4.o)\
 ${OUTDIR}/cpplib.a(quat.o)\
 ${OUTDIR}/cpplib.a(mat4.o)\
 ${OUTDIR}/cpplib.a(glcull.o)

${OUTDIR}:
	mkdir ${OUTDIR}

${OUTDIR}/cpplib.a(dstring.o): src/dstring.cpp
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

