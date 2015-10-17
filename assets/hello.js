var STARFIELD_COUNT = 4000;
var STARFIELD_NEAR = 600;
var STARFIELD_FAR = 2000;

var CUBE_ROWS = 10;
var CUBE_COLS = 10;
var CUBE_LAYERS = 3;

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
    ' gl_Position = Projectionm * ( Viewm * ( Modelm * vec4( Position, 1.0 ) ) );\n'+
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

  /////// STARFIELD /////////

  var starfieldVertices = [VERTEX_POSITION, VERTEX_COLOR];
  var starfieldIndices = [];
  var offset = 0;
  for (var i = 0; i < STARFIELD_COUNT; ++i) {
    var size = Math.random() * 5;
    var distance = (Math.random() * (STARFIELD_FAR - STARFIELD_NEAR)) + STARFIELD_NEAR;
    var brightness = (Math.random() * 0.5) + 0.4;

    var transform = Matrix4f();
    transform = transform.multiply(transform.rotationZ(Math.random() * 2 * Math.PI));
    transform = transform.multiply(transform.rotationY(Math.random() * 2 * Math.PI));
    transform = transform.multiply(transform.rotationX(Math.random() * 2 * Math.PI));
    var bottomLeft = transform.transform(Vector3f(0, 0, distance));
    var topLeft = transform.transform(Vector3f(0, size, distance));
    var topRight = transform.transform(Vector3f(size, size, distance));
    var bottomRight = transform.transform(Vector3f(size, 0, distance));
    starfieldVertices.push(
      bottomLeft,  Vector4f(0, 0, 0, brightness),
      topLeft,     Vector4f(0, 0, 0, brightness),
      topRight,    Vector4f(0, 0, 0, brightness),
      bottomRight, Vector4f(0, 0, 0, brightness)
    );

    var bottomLeftIdx = 0;
    var topLeftIdx = 1;
    var topRightIdx = 2;
    var bottomRightIdx = 3;
    starfieldIndices.push(
      offset + bottomLeftIdx, offset + topLeftIdx, offset + topRightIdx,
      offset + topRightIdx, offset + bottomRightIdx, offset + bottomLeftIdx
    );

    offset += 4;
  }
  var geometry = Geometry({
    vertices: starfieldVertices,
    indices: starfieldIndices,
  });

  var starfield = Model({
    geometry: geometry,
    program: program,
    position: Vector3f(0, 0, 0),
    rotation: Vector3f(0, 0, 0),
    scale: Vector3f(1, 1, 1)
  });
  env.scene.add(starfield);

  /////// CUBES /////////

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
  for (var i = 0; i < CUBE_ROWS; ++i) {
    for (var j = 0; j < CUBE_COLS; ++j) {
      for (var k = 0; k < CUBE_LAYERS; ++k) {
        var cube = Model({
          geometry: cubeGeometry,
          program: program,
          position: Vector3f(
            (4 * i) - ((CUBE_ROWS * 4) / 2),
            (4 * j) - ((CUBE_COLS * 4) / 2),
            (4 * k) - (CUBE_LAYERS * 4) - 10
          ),
          rotation: Vector3f(0, 0, 0),
          scale: Vector3f(1, 1, 1),
          onFrame: function(ev) {
            if (!this._start) {
              this._start = ev.now;
            }
            if (this._touchdown) {
              return;
            }
            var secondsElapsed = (ev.now - this._start); // Seconds
            this.rotation.x = secondsElapsed;
            this.rotation.y = secondsElapsed * 0.8;
          },
          onHoverOver: function(ev) {
            this.scale.x = 1.4;
            this.scale.y = 1.4;
            this.scale.z = 1.4;
          },
          onHoverOut: function(ev) {
            this.scale.x = 1;
            this.scale.y = 1;
            this.scale.z = 1;
          },
          onGestureTouchDown: function(ev) {
            this._touchdown = true;
          },
          onGestureTouchUp: function(ev) {
            this._touchdown = false;
            env.scene.remove(this);
          },
          onGestureTouchCancel: function(ev) {
            this._touchdown = false;
          }
        });
        env.scene.add(cube);
      }
    }
  }

  /////// CURSOR //////////
  var darkProgram = Program((
    '#version 300 es\n'+
    'in vec3 Position;\n'+
    'uniform mat4 Modelm;\n'+
    'uniform mat4 Viewm;\n'+
    'uniform mat4 Projectionm;\n'+
    'void main()\n'+
    '{\n'+
    ' gl_Position = Projectionm * ( Viewm * ( Modelm * vec4( Position, 1.0 ) ) );\n'+
    '}'
  ), (
    '#version 300 es\n'+
    'out lowp vec4 outColor;\n'+
    'void main()\n'+
    '{\n'+
    ' outColor = vec4(0.0, 0.0, 0.0, 1.0);\n'+
    '}'
  ));
  var cursor = Model({
    geometry: cubeGeometry,
    program: darkProgram,
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
  env.scene.add(cursor);
}