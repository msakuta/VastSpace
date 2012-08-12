uniform sampler2DShadow shadowmap;
uniform sampler2DShadow shadowmap2;
uniform sampler2DShadow shadowmap3;
uniform float shadowSlopeScaledBias;
const float margin = 0.01;

// Returns the first element of the vector returned by shadow2DProj(), because
// using the last element (alpha channel) won't work for Radeon HD.
float shadowMapIntensity(float offset){
	vec4 voff = vec4(0, 0, -offset * shadowSlopeScaledBias, 0);
	if(	   margin < gl_TexCoord[4].x && gl_TexCoord[4].x < 1. - margin
		&& margin < gl_TexCoord[4].y && gl_TexCoord[4].y < 1. - margin
		&& margin < gl_TexCoord[4].z && gl_TexCoord[4].z < 1. - margin)
		return shadow2DProj(shadowmap3, gl_TexCoord[4] + voff)[0];
	else if(margin < gl_TexCoord[3].x && gl_TexCoord[3].x < 1. - margin
		&& margin < gl_TexCoord[3].y && gl_TexCoord[3].y < 1. - margin
		&& margin < gl_TexCoord[3].z && gl_TexCoord[3].z < 1. - margin)
		return shadow2DProj(shadowmap2, gl_TexCoord[3] + voff)[0];
	else{
		// Smoothly disappear on the edge
		float shadowedge = min(gl_TexCoord[2].x, 1. - gl_TexCoord[2].x);
		shadowedge = min(shadowedge, min(gl_TexCoord[2].y, 1. - gl_TexCoord[2].y));
		shadowedge = min(shadowedge, min(gl_TexCoord[2].z, 1. - gl_TexCoord[2].z));
		shadowedge = 1. - min(1., 10. * shadowedge);
		return (shadow2DProj(shadowmap, gl_TexCoord[2] + voff) * (1. - shadowedge) + shadowedge)[0];
	}
}

