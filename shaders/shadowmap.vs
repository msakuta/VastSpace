uniform mat4 shadowMatrices[3];
varying vec4 shadowTexCoord1;
varying vec4 shadowTexCoord2;
varying vec4 shadowTexCoord3;

void shadowMapVertex(){
	shadowTexCoord1 = shadowMatrices[0] * gl_ModelViewMatrix * gl_Vertex;
	shadowTexCoord2 = shadowMatrices[1] * gl_ModelViewMatrix * gl_Vertex;
	shadowTexCoord3 = shadowMatrices[2] * gl_ModelViewMatrix * gl_Vertex;
}
