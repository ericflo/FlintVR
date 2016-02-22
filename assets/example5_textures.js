var Geometry        = Flint.Core.Geometry;
var Program         = Flint.Core.Program;
var Model           = Flint.Core.Model;
var Texture         = Flint.Core.Texture;
var Vector2f        = Flint.Core.Vector2f;
var Vector3f        = Flint.Core.Vector3f;
var Vector4f        = Flint.Core.Vector4f;
var Matrix4f        = Flint.Core.Matrix4f;
var VERTEX_POSITION = Flint.Core.VERTEX_POSITION;
var VERTEX_COLOR    = Flint.Core.VERTEX_COLOR;
var VERTEX_UV0      = Flint.Core.VERTEX_UV0;

function vrmain(env) {
  var program = Program((
    '#version 300 es\n'+
    'in vec3 Position;\n'+
    'in vec4 VertexColor;\n'+
    'in vec2 TexCoord;\n'+
    'uniform mat4 Modelm;\n'+
    'uniform mat4 Viewm;\n'+
    'uniform mat4 Projectionm;\n'+
    'out vec4 fragmentColor;\n'+
    'out highp vec2 oTexCoord;\n'+
    'void main()\n'+
    '{\n'+
    ' gl_Position = Projectionm * (Viewm * (Modelm * vec4(Position, 1.0)));\n'+
    ' fragmentColor = VertexColor;\n'+
    ' oTexCoord = TexCoord;\n'+
    '}'
  ), (
    '#version 300 es\n'+
    'uniform sampler2D Texture0;\n'+
    'in highp vec4 fragmentColor;\n'+
    'in highp vec2 oTexCoord;\n'+
    'out highp vec4 outColor;\n'+
    'void main()\n'+
    '{\n'+
    ' outColor = (fragmentColor * 0.5) + (texture(Texture0, oTexCoord) * 0.5);\n'+
    '}'
  ));
  var cubeVertices = [
    VERTEX_POSITION,       VERTEX_COLOR,          VERTEX_UV0,
    Vector3f(-1,  1, -1),  Vector4f(1, 0, 1, 1),  Vector2f(-1, -1), // top
    Vector3f( 1,  1, -1),  Vector4f(0, 1, 0, 1),  Vector2f( 1, -1),
    Vector3f( 1,  1,  1),  Vector4f(0, 0, 1, 1),  Vector2f( 1,  1),
    Vector3f(-1,  1,  1),  Vector4f(1, 0, 0, 1),  Vector2f(-1,  1),
    Vector3f(-1, -1, -1),  Vector4f(0, 0, 1, 1),  Vector2f(-1, -1), // bottom
    Vector3f(-1, -1,  1),  Vector4f(0, 1, 0, 1),  Vector2f(-1,  1),
    Vector3f( 1, -1,  1),  Vector4f(1, 0, 1, 1),  Vector2f( 1,  1),
    Vector3f( 1, -1, -1),  Vector4f(1, 0, 0, 1),  Vector2f( 1, -1),
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
  var texture = Texture({
    path: 'assets/noise.jpg',
    width: 300,
    height: 300
  });

  var cube = Model({
    geometry: cubeGeometry,
    program: program,
    textures: [texture],
    position: Vector3f(0, 0, -10),
    onFrame: function(ev) {
      if (!this._start) {
        this._start = ev.now;
      }
      var secondsElapsed = (ev.now - this._start); // Seconds
      this.rotation.x = secondsElapsed;
      this.rotation.y = secondsElapsed * 0.8;
    },
  });
  Flint.scene.add(cube);
}