function vrmain(env) {
  var Geometry        = env.core.Geometry;
  var Program         = env.core.Program;
  var Model           = env.core.Model;
  var Vector3f        = env.core.Vector3f;
  var Vector4f        = env.core.Vector4f;
  var Matrix4f        = env.core.Matrix4f;
  var VERTEX_POSITION = env.core.VERTEX_POSITION;
  var VERTEX_COLOR    = env.core.VERTEX_COLOR;
  ////////////////////////////////////////////

  env.scene.setClearColor(Vector4f(1, 1, 1, 1));

  var program = Program((
    '#version 300 es\n'+
    'in vec3 Position;\n'+
    'in vec4 VertexColor;\n'+
    'uniform mat4 Modelm;\n'+
    'uniform mat4 Viewm;\n'+
    'uniform mat4 Projectionm;\n'+
    'out vec4 fragmentColor;\n'+
    'void main()\n'+
    '{\n'+
    ' gl_Position = Projectionm * (Viewm * (Modelm * vec4(Position, 1.0)));\n'+
    ' fragmentColor = VertexColor;\n'+
    '}'
  ), (
    '#version 300 es\n'+
    'in lowp vec4 fragmentColor;\n'+
    'out lowp vec4 outColor;\n'+
    'void main()\n'+
    '{\n'+
    ' outColor = fragmentColor;\n'+
    '}'
  ));
  var cubeVertices = [
    VERTEX_POSITION,       VERTEX_COLOR,
    Vector3f(-1,  1, -1),  Vector4f(1, 0, 1, 1), // top
    Vector3f( 1,  1, -1),  Vector4f(0, 1, 0, 1),
    Vector3f( 1,  1,  1),  Vector4f(0, 0, 1, 1),
    Vector3f(-1,  1,  1),  Vector4f(1, 0, 0, 1),
    Vector3f(-1, -1, -1),  Vector4f(0, 0, 1, 1), // bottom
    Vector3f(-1, -1,  1),  Vector4f(0, 1, 0, 1),
    Vector3f( 1, -1,  1),  Vector4f(1, 0, 1, 1),
    Vector3f( 1, -1, -1),  Vector4f(1, 0, 0, 1)
  ];
  var cubeIndices = [
    0, 1, 2, 2, 3, 0, // top
    4, 5, 6, 6, 7, 4, // bottom
    2, 6, 7, 7, 1, 2, // right
    0, 4, 5, 5, 3, 0, // left
    3, 5, 6, 6, 2, 3, // front
    0, 1, 7, 7, 4, 0  // back
  ];
  var cubeGeometry = Geometry({
    vertices: cubeVertices,
    indices: cubeIndices
  });
  var cube = null;
  for (var i = 0; i < 15; ++i) {
    if (cube) {
      var nextCube = Model({
        geometry: cubeGeometry,
        program: program,
        position: Vector3f(1, 2, -0.5),
        onFrame: function(ev) {
          if (!this._start) {
            this._start = ev.now;
          }
          var secondsElapsed = (ev.now - this._start); // Seconds
          this.rotation.x = secondsElapsed;
          this.rotation.y = secondsElapsed * 0.8;
        },
      });
      cube.add(nextCube);
      cube = nextCube;
    } else {
      cube = Model({
        geometry: cubeGeometry,
        program: program,
        position: Vector3f(0, 0, -18),
        onFrame: function(ev) {
          if (!this._start) {
            this._start = ev.now;
          }
          var secondsElapsed = (ev.now - this._start); // Seconds
          this.rotation.x = secondsElapsed;
          this.rotation.y = secondsElapsed * 0.8;
        },
      });
      env.scene.add(cube);
    }
  }
}