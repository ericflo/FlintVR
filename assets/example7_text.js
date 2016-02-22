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
  Flint.scene.background = Texture({
    path: 'assets/ggp.jpg',
    width: 7168,
    height: 3584
  });

  Flint.scene.add(Model({
    position: Vector3f(0, 0, -12),
    text: 'Hello, world!',
    textColor: Vector4f(0.1, 0.1, 0.1, 1),
    textSize: 12,
    onGazeHoverOver: function(ev) {
      this.textColor.x = 1;
    },
    onGazeHoverOut: function(ev) {
      this.textColor.x = 0.1;
    }
  }));


  /////// CURSOR //////////
  var cursorProgram = Program((
    '#version 300 es\n'+
    'in vec3 Position;\n'+
    'uniform mat4 Modelm;\n'+
    'uniform mat4 Viewm;\n'+
    'uniform mat4 Projectionm;\n'+
    'void main()\n'+
    '{\n'+
    ' gl_Position = Projectionm * (Viewm * (Modelm * vec4(Position, 1.0)));\n'+
    '}'
  ), (
    '#version 300 es\n'+
    'out lowp vec4 outColor;\n'+
    'void main()\n'+
    '{\n'+
    ' outColor = vec4(0.0, 0.0, 0.0, 1.0);\n'+
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
  var cursor = Model({
    geometry: cubeGeometry,
    program: cursorProgram,
    position: Vector3f(0, 0, 0),
    rotation: Vector3f(0, 0, 0),
    scale: Vector3f(0.02, 0.02, 0.02),
    onFrame: function(ev) {
      var pos = ev.viewPos.add(ev.viewFwd.multiply(5));
      // TODO: Make this not necessary
      this.position.x = pos.x;
      this.position.y = pos.y;
      this.position.z = pos.z;
    }
  });
  Flint.scene.add(cursor);
}