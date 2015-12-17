uniform sampler2DShadow shadowmap;
uniform sampler2DShadow shadowmap2;
uniform sampler2DShadow shadowmap3;
uniform float shadowSlopeScaledBias;
const float margin = 0.01;
varying vec4 shadowTexCoord1;
varying vec4 shadowTexCoord2;
varying vec4 shadowTexCoord3;

// Returns the first element of the vector returned by shadow2DProj(), because
// using the last element (alpha channel) won't work for Radeon HD.
float shadowMapIntensity(float offset){
	vec4 voff = vec4(0, 0, -offset * shadowSlopeScaledBias, 0);
	if(	   margin < shadowTexCoord3.x && shadowTexCoord3.x < 1. - margin
		&& margin < shadowTexCoord3.y && shadowTexCoord3.y < 1. - margin
		&& margin < shadowTexCoord3.z && shadowTexCoord3.z < 1. - margin)
		return shadow2DProj(shadowmap3, shadowTexCoord3 + voff)[0];
	else if(margin < shadowTexCoord2.x && shadowTexCoord2.x < 1. - margin
		&& margin < shadowTexCoord2.y && shadowTexCoord2.y < 1. - margin
		&& margin < shadowTexCoord2.z && shadowTexCoord2.z < 1. - margin)
		return shadow2DProj(shadowmap2, shadowTexCoord2 + voff)[0];
	else{
		// Smoothly disappear on the edge
		float shadowedge = min(shadowTexCoord1.x, 1. - shadowTexCoord1.x);
		shadowedge = min(shadowedge, min(shadowTexCoord1.y, 1. - shadowTexCoord1.y));
		shadowedge = min(shadowedge, min(shadowTexCoord1.z, 1. - shadowTexCoord1.z));
		shadowedge = 1. - min(1., 10. * shadowedge);
		return (shadow2DProj(shadowmap, shadowTexCoord1 + voff) * (1. - shadowedge) + shadowedge)[0];
	}
}
