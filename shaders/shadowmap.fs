uniform sampler2DShadow shadowmap;
uniform sampler2DShadow shadowmap2;
uniform sampler2DShadow shadowmap3;

// Returns the first element of the vector returned by shadow2DProj(), because
// using the last element (alpha channel) won't work for Radeon HD.
float shadowMapIntensity(){
	if(	   0.001 < gl_TexCoord[4].x && gl_TexCoord[4].x < 0.999
		&& 0.001 < gl_TexCoord[4].y && gl_TexCoord[4].y < 0.999
		&& 0.001 < gl_TexCoord[4].z && gl_TexCoord[4].z < 0.999)
		return shadow2DProj(shadowmap3, gl_TexCoord[4])[0];
	else if(0.001 < gl_TexCoord[3].x && gl_TexCoord[3].x < 0.999
		&& 0.001 < gl_TexCoord[3].y && gl_TexCoord[3].y < 0.999
		&& 0.001 < gl_TexCoord[3].z && gl_TexCoord[3].z < 0.999)
		return shadow2DProj(shadowmap2, gl_TexCoord[3])[0];
	else{
		// Smoothly disappear on the edge
		float shadowedge = min(gl_TexCoord[2].x, 1. - gl_TexCoord[2].x);
		shadowedge = min(shadowedge, min(gl_TexCoord[2].y, 1. - gl_TexCoord[2].y));
		shadowedge = min(shadowedge, min(gl_TexCoord[2].z, 1. - gl_TexCoord[2].z));
		shadowedge = 1. - min(1., 10. * shadowedge);
		return (shadow2DProj(shadowmap, gl_TexCoord[2]) * (1. - shadowedge) + shadowedge)[0];
	}
}

