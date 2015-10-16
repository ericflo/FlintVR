/*
#include "CoreSubmit.h"

#ifndef LOG_COMPONENT
#define LOG_COMPONENT "VrCubeWorld"
#endif

#if 0
	#define GLJ( func )		func; EglCheckErrors();
#else
	#define GLJ( func )		func;
#endif

OVR::Matrix4f CURRENT_EYE_VIEW_MATRIX;
OVR::Matrix4f CURRENT_EYE_PROJECTION_MATRIX;

bool CoreSubmit(JSContext *cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	// Check the arguments length
	if (args.length() != 1) {
		JS_ReportError(cx, "Wrong number of arguments: %d, was expecting: %d", argc, 1);
		return false;
	}
	// Check to make sure the arg is an object
	if (!args[0].isObject()) {
		JS_ReportError(cx, "Invalid argument type, must be an object");
		return false;
	}

	// Get the GlGeometry reference
	JS::RootedValue geomVal(cx, args[0]);
	JS::RootedObject geomObj(cx, &geomVal.toObject());
	// TODO: Validate object type before casting
	GeometryBundle* bundle = (GeometryBundle*)JS_GetPrivate(geomObj);

	// Set up model matrix
	OVR::Matrix4f matrix = (
			OVR::Matrix4f::RotationX(bundle->rotate->x) *
			OVR::Matrix4f::RotationY(bundle->rotate->y) *
			OVR::Matrix4f::RotationZ(bundle->rotate->z)
	);
	matrix.SetTranslation(*(bundle->translate));

	GLJ( glUseProgram( bundle->program.program ) );
	GLJ( glUniformMatrix4fv( bundle->program.uModel, 1, GL_TRUE, matrix.M[0] ) );
	GLJ( glUniformMatrix4fv( bundle->program.uView, 1, GL_TRUE, CURRENT_EYE_VIEW_MATRIX.M[0] ) );
	GLJ( glUniformMatrix4fv( bundle->program.uProjection, 1, GL_TRUE, CURRENT_EYE_PROJECTION_MATRIX.M[0] ) );
	GLJ( glBindVertexArray( bundle->geometry.vertexArrayObject ) );
	GLJ( glDrawElements( GL_TRIANGLES, bundle->geometry.indexCount, GL_UNSIGNED_SHORT, NULL ) );

	args.rval().set(JS::NullValue());
	return true;
}

void SetCurrentEyeViewMatrix(OVR::Matrix4f mat) {
	CURRENT_EYE_VIEW_MATRIX = mat;
}

void SetCurrentEyeProjectionMatrix(OVR::Matrix4f mat) {
	CURRENT_EYE_PROJECTION_MATRIX = mat;
}

void SetupCoreSubmit(JSContext *cx, JS::RootedObject *global, JS::RootedObject *core) {
	JSFunction *func = JS_DefineFunction(cx, *core, "submit", CoreSubmit, 1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT);
	if (!func) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_COMPONENT, "Could not add env.core.submit\n");
		return;
	}
}
*/