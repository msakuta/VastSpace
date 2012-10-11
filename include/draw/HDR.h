/** \file
 * \brief The header for defining HDR effect related global variables.
 *
 * They're not necessarily bound to ShaderBind object, but they often do.
 * Specifically, all world objects should bind to exposure value, or it would look strange.
 */
#ifndef HDR_H
#define HDR_H

extern double r_exposure; ///< The exposure of the camera.
extern int r_auto_exposure; ///< Flag to enable automatic exposure adjustment.
extern int r_tonemap; ///< Flag to enable tone mapping in the shader.

#endif
